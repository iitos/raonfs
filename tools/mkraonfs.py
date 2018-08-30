#!/usr/bin/python3

import os
import io
import sys
import stat
import argparse

import struct
import json

from collections import defaultdict

ENDIAN_TYPE = os.getenv("ENDIAN_TYPE", "little")

endian_formats = {}
endian_formats["little"] = "<"
endian_formats["big"] = ">"

def get_steps(bytesize, unitsize):
    if type(bytesize) is io.BytesIO:
        bytesize = sys.getsizeof(bytesize)
    return int(((bytesize - 1) / unitsize) + 1) * unitsize

def write_packdata(td, fmt, *args):
    return td.write(struct.pack("{}{}".format(endian_formats.get(ENDIAN_TYPE, ""), fmt), *args))

def read_packdata(td, fmt):
    data = struct.unpack("{}{}".format(endian_formats.get(ENDIAN_TYPE, ""), fmt), td.read(struct.calcsize(fmt)))
    if len(fmt) == 1:
        return data[0]
    return data

def get_supersize():
    return struct.calcsize("BBBBIIIQII")

def write_superblock(td, fsinfo, textree):
    td.seek(0)
    write_packdata(td, "BBBB", *bytearray(fsinfo["magics"], "utf-8"))
    write_packdata(td, "I", fsinfo["textbase"])
    write_packdata(td, "I", fsinfo["textsize"])
    write_packdata(td, "I", search_text(textree, fsinfo["fsname"]))
    write_packdata(td, "Q", fsinfo["fssize"])
    write_packdata(td, "I", fsinfo["blocksize"])
    write_packdata(td, "I", fsinfo["rootid"]["ioffset"])

def update_fssize(fsinfo, td):
    fssize = td.tell()
    if fsinfo["fssize"] < fssize:
        fsinfo["fssize"] = fssize

def get_textsize(textio):
    return sys.getsizeof(textio)

def write_texts(td, fsinfo, textree, textio):
    textio.seek(0)
    td.seek(fsinfo["textbase"])
    td.write(textio.read())
    update_fssize(fsinfo, td)

def insert_text(textree, token, textio):
    if token not in textree:
        textree[token] = textio.tell()
        textio.write(token.encode("utf-8"))
        textio.write(b'\0')

def search_text(textree, token):
    return textree[token]

def get_inodesize():
    return struct.calcsize("IHHHIIIIQ")

def write_inodes(td, fsinfo, textree, fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        td.seek(node["ioffset"])
        write_packdata(td, "I", node["size"])
        write_packdata(td, "H", node["mode"])
        write_packdata(td, "H", node["uid"])
        write_packdata(td, "H", node["gid"])
        if "link" in node:
            write_packdata(td, "I", search_text(textree, node["link"]))
        else:
            write_packdata(td, "I", 0)
        write_packdata(td, "I", node["ctime"])
        write_packdata(td, "I", node["mtime"])
        write_packdata(td, "I", node["atime"])
        if "doffset" in node:
            write_packdata(td, "Q", node["doffset"])
        else:
            write_packdata(td, "Q", 0)
        update_fssize(fsinfo, td)

def get_dentsize():
    return struct.calcsize("II")

def write_dentries(td, fsinfo, textree, fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["type"] == "dir":
            td.seek(node["doffset"])
            children = node["children"]
            for childname in sorted(children):
                write_packdata(td, "I", search_text(textree, childname))
                write_packdata(td, "I", fstree[children[childname]]["ioffset"])
            update_fssize(fsinfo, td)

def write_blocks(td, fsinfo, textree, fstree):
    for nodeid in sorted(fstree):
        node = fstree[nodeid]
        if node["type"] == "file":
            td.seek(node["doffset"])
            with open(node["path"], "rb") as od:
                td.write(od.read())
            update_fssize(fsinfo, td)

def get_fsnode(fstree, nodepath):
    nodestat = os.stat(nodepath)
    nodeid = nodestat.st_ino
    if nodeid not in fstree:
        fstree[nodeid] = node = {}
        node["id"] = nodeid
        node["size"] = 0
        node["ioffset"] = 0
        node["doffset"] = 0
        node["path"] = nodepath
        node["mode"] = nodestat.st_mode
        node["uid"] = nodestat.st_uid
        node["gid"] = nodestat.st_gid
        node["ctime"] = int(nodestat.st_ctime)
        node["mtime"] = int(nodestat.st_mtime)
        node["atime"] = int(nodestat.st_atime)
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
            node["size"] = get_dentsize() * len(node["children"])

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

    textree = {}
    textio = io.BytesIO()
    textio.write(b'#')

    insert_text(textree, args.name, textio)

    for key, node in fstree.items():
        if "children" in node:
            for childname, chlidid in node["children"].items():
                insert_text(textree, childname, textio)
        if "link" in node:
            insert_text(textree, node["link"], textio)

    fsinfo = {}
    fsinfo["magics"] = args.magics
    fsinfo["rootid"] = get_fsnode(fstree, args.source)
    fsinfo["textbase"] = args.blocksize
    fsinfo["textsize"] = get_textsize(textio)
    fsinfo["fsname"] = args.name
    fsinfo["fssize"] = 0
    fsinfo["blocksize"] = args.blocksize
    fsinfo["nodebase"] = fsinfo["textbase"] + get_steps(fsinfo["textsize"], args.blocksize)

    update_sizes(fstree)

    offsets = fsinfo["nodebase"]
    for step in range(args.inlinestep, 1, -1):
        offsets += update_inlines(fstree, offsets, int(args.blocksize / pow(2, step)))
        offsets = get_steps(offsets, args.blocksize)

    update_extents(fstree, offsets, args.blocksize)

    if args.target:
        td = open(args.target, "wb")
        write_texts(td, fsinfo, textree, textio)
        write_inodes(td, fsinfo, textree, fstree)
        write_dentries(td, fsinfo, textree, fstree)
        write_blocks(td, fsinfo, textree, fstree)
        write_superblock(td, fsinfo, textree)
        td.close()
    if args.output:
        od = open(args.output, "w")
        print(json.dumps(textree, indent = 2), file = od)
        print(json.dumps(fsinfo, indent = 2), file = od)
        print(json.dumps(fstree, indent = 2), file = od)
        od.close()
