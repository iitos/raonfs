#!/usr/bin/python3

import os
import io
import sys
import stat
import argparse

import struct
import json

from collections import defaultdict

INODE_INLINE_DATA_FLAG = 0

ENDIAN_TYPE = os.getenv("ENDIAN_TYPE", "little")

endian_formats = {}
endian_formats["little"] = "<"
endian_formats["big"] = ">"

dentry_types = {}
dentry_types["none"] = 0
dentry_types["dir"] = 1
dentry_types["file"] = 2
dentry_types["link"] = 3
dentry_types["bdev"] = 4
dentry_types["cdev"] = 5
dentry_types["fifo"] = 6
dentry_types["sock"] = 7

def get_steps(bytesize, unitsize):
    return int(((bytesize - 1) / unitsize) + 1) * unitsize

def write_packdata(td, fmt, *args):
    return td.write(struct.pack("{}{}".format(endian_formats.get(ENDIAN_TYPE, ""), fmt), *args))

def read_packdata(td, fmt):
    data = struct.unpack("{}{}".format(endian_formats.get(ENDIAN_TYPE, ""), fmt), td.read(struct.calcsize(fmt)))
    if len(fmt) == 1:
        return data[0]
    return data

def get_supersize():
    return struct.calcsize("BBBBIIQ32s")

def write_superblock(td, fsinfo):
    td.seek(0)
    write_packdata(td, "BBBB", *bytearray(fsinfo["magics"], "utf-8"))
    write_packdata(td, "I", fsinfo["blocksize"])
    write_packdata(td, "I", fsinfo["rootid"]["ioffset"])
    write_packdata(td, "Q", fsinfo["fssize"])
    write_packdata(td, "32s", fsinfo["fsname"].encode())

def update_fssize(fsinfo, td):
    fssize = td.tell()
    if fsinfo["fssize"] < fssize:
        fsinfo["fssize"] = fssize

def get_inodesize():
    return struct.calcsize("IIIHHHIIIIQQ")

def write_inodes(td, fsinfo, fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        td.seek(node["ioffset"])
        write_packdata(td, "I", node["size"])
        write_packdata(td, "I", node["msize"])
        write_packdata(td, "I", node["rdev"])
        write_packdata(td, "H", node["mode"])
        write_packdata(td, "H", node["uid"])
        write_packdata(td, "H", node["gid"])
        write_packdata(td, "I", node["ctime"])
        write_packdata(td, "I", node["mtime"])
        write_packdata(td, "I", node["atime"])
        write_packdata(td, "I", node["flags"])
        if "doffset" in node:
            write_packdata(td, "Q", node["doffset"])
        else:
            write_packdata(td, "Q", 0)
        if "moffset" in node:
            write_packdata(td, "Q", node["moffset"])
        else:
            write_packdata(td, "Q", 0)
        update_fssize(fsinfo, td)

def get_dentsize():
    return struct.calcsize("IHHI")

def write_dentries(td, fsinfo, fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["type"] == "dir":
            children = node["children"]
            td.seek(node["doffset"])
            textoffs = 0
            for childname in sorted(children):
                child = fstree[children[childname]]
                write_packdata(td, "I", textoffs)
                write_packdata(td, "H", len(childname))
                write_packdata(td, "H", dentry_types[child["type"]])
                write_packdata(td, "I", child["ioffset"])
                textoffs += len(childname)
            td.seek(node["moffset"])
            for childname in sorted(children):
                child = fstree[children[childname]]
                write_packdata(td, "{}s".format(len(childname)), childname.encode())
            update_fssize(fsinfo, td)

def write_blocks(td, fsinfo, fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["type"] == "file":
            td.seek(node["doffset"])
            with open(node["path"], "rb") as od:
                td.write(od.read())
            update_fssize(fsinfo, td)
        elif node["type"] == "link":
            td.seek(node["doffset"])
            td.write(node["link"].encode())
            update_fssize(fsinfo, td)

def get_fsnode(fstree, nodepath):
    nodestat = os.lstat(nodepath)
    nodeid = nodestat.st_ino
    if nodeid not in fstree:
        fstree[nodeid] = node = {}
        node["id"] = nodeid
        node["type"] = "none"
        node["size"] = 0
        node["msize"] = 0
        node["ioffset"] = 0
        node["doffset"] = 0
        node["moffset"] = 0
        node["path"] = nodepath
        node["rdev"] = nodestat.st_rdev
        node["mode"] = nodestat.st_mode
        node["uid"] = nodestat.st_uid
        node["gid"] = nodestat.st_gid
        node["ctime"] = int(nodestat.st_ctime)
        node["mtime"] = int(nodestat.st_mtime)
        node["atime"] = int(nodestat.st_atime)
        node["flags"] = 0
        if os.path.islink(nodepath):
            node["type"] = "link"
            node["link"] = os.readlink(nodepath)
        elif os.path.isdir(nodepath):
            node["type"] = "dir"
        elif os.path.isfile(nodepath):
            node["type"] = "file"
        else:
            nodemode = nodestat.st_mode
            if stat.S_ISCHR(nodemode):
                noderdev = nodestat.st_rdev
                node["type"] = "cdev"
                node["major"] = os.major(noderdev)
                node["minor"] = os.minor(noderdev)
            elif stat.S_ISBLK(nodemode):
                noderdev = nodestat.st_rdev
                node["type"] = "bdev"
                node["major"] = os.major(noderdev)
                node["minor"] = os.minor(noderdev)
            elif stat.S_ISFIFO(nodemode):
                node["type"] = "fifo"
            elif stat.S_ISSOCK(nodemode):
                node["type"] = "sock"
    return fstree[nodeid]

def build_fstree(path):
    fstree = defaultdict(lambda: {})
    for dirpath, dirnames, filenames in os.walk(path, followlinks = False):
        children = {}
        nodepath = os.path.abspath(dirpath)
        dirnode = get_fsnode(fstree, nodepath)
        for dirname in dirnames:
            childpath = os.path.join(nodepath, dirname)
            childnode = get_fsnode(fstree, childpath)
            children[dirname] = childnode['id']
        for filename in filenames:
            childpath = os.path.join(nodepath, filename)
            childnode = get_fsnode(fstree, childpath)
            children[filename] = childnode['id']
        dirnode["children"] = children
    return fstree

def update_sizes(fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["type"] == "file":
            node["size"] = os.path.getsize(node["path"])
        elif node["type"] == "dir":
            node["msize"] = sum(list(map(lambda x: len(x), node["children"])))
            node["size"] = get_dentsize() * len(node["children"]) + node["msize"]
        elif node["type"] == "link":
            node["size"] = len(node["link"])

def update_inlines(fstree, baseoffset, maxsize):
    isize = get_inodesize()
    sizes = 0
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["ioffset"] != 0:
            continue
        if node["size"] != 0 and node["size"] > maxsize - isize:
            continue
        node["ioffset"] = baseoffset + sizes
        node["doffset"] = baseoffset + sizes + isize
        node["moffset"] = baseoffset + sizes + isize + (node["size"] - node["msize"])
        node["flags"] = node["flags"] | (1 << INODE_INLINE_DATA_FLAG)
        sizes += maxsize
    return sizes

def update_extents(fstree, baseoffset, blocksize):
    isize = get_inodesize()
    sizes = 0
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["ioffset"] != 0:
            continue
        node["ioffset"] = baseoffset + sizes
        sizes += isize
    baseoffset = baseoffset + get_steps(sizes, blocksize)
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["doffset"] != 0:
            continue
        if node["size"] == 0:
            continue
        node["doffset"] = baseoffset
        node["moffset"] = baseoffset + (node["size"] - node["msize"])
        baseoffset = baseoffset + get_steps(node["size"], blocksize)
    return baseoffset

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--source", help = "source directory path", default = ".")
    parser.add_argument("-t", "--target", help = "target image file")
    parser.add_argument("-b", "--blocksize", help = "block size", type = int, default = 4096)
    parser.add_argument("-l", "--inlinestep", help = "inline step", type = int, default = 4)
    parser.add_argument("-m", "--magics", help = "filesystem magic code", default = "RAON")
    parser.add_argument("-n", "--name", help = "filesystem name", default = "RAON-FS")
    parser.add_argument("-o", "--output", help = "print filesystem tree as json format")
    args = parser.parse_args()

    fstree = build_fstree(args.source)
    fsinfo = {}
    fsinfo["magics"] = args.magics
    fsinfo["blocksize"] = args.blocksize
    fsinfo["rootid"] = get_fsnode(fstree, args.source)
    fsinfo["fssize"] = 0
    fsinfo["fsname"] = args.name
    fsinfo["nodebase"] = args.blocksize

    update_sizes(fstree)

    offsets = fsinfo["nodebase"]
    for step in range(args.inlinestep, 0, -1):
        offsets += update_inlines(fstree, offsets, int(args.blocksize / pow(2, step)))
        offsets = get_steps(offsets, args.blocksize)

    update_extents(fstree, offsets, args.blocksize)

    if args.target:
        td = open(args.target, "wb")
        write_inodes(td, fsinfo, fstree)
        write_dentries(td, fsinfo, fstree)
        write_blocks(td, fsinfo, fstree)
        write_superblock(td, fsinfo)
        td.close()
    if args.output:
        od = open(args.output, "w")
        print(json.dumps(fsinfo, indent = 2), file = od)
        print(json.dumps(fstree, indent = 2), file = od)
        od.close()
