#!/bin/bash

set -e

SOURCE=$1
TARGET=.img
MOUNTO=.mnt
LDEV=$(losetup -f)

MKRAONFS=../tools/mkraonfs.py

$MKRAONFS -s $SOURCE -t $TARGET
mkdir -p $MOUNTO
losetup $LDEV $TARGET
mount -t raonfs $LDEV $MOUNTO

diff -r $SOURCE $MOUNTO

umount $MOUNTO
losetup -d $LDEV
rm -rf $MOUNTO
rm -f $TARGET
