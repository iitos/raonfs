obj-m += raonfs.o

raonfs-y := super.o inode.o iomisc.o dbmisc.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
