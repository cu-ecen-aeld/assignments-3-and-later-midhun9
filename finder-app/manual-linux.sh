#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
   
    # “deep clean” the kernel build tree - removing the .config file with any existing configurations
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    # Configure for our “virt” arm dev board we will simulate in QEMU
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    # Build a kernel image for booting with QEMU
    make -j8 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # Build any kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    # Build the devicetree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi 

echo "Adding the Image in outdir"

cp -a ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories

# Create rootfs directory
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

# Create directory tree
mkdir -p bin dev etc home lib proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
else
    cd busybox
fi

# TODO: Make and install busybox

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs

export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
mkdir lib64

#cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib
#cp -a $SYSROOT/lib64/libm.so.6 lib64
#cp -a $SYSROOT/lib64/libresolv.so.2 lib64
#cp -a $SYSROOT/lib64/libc.so.6 lib64

cp -L $SYSROOT/lib/ld-linux-aarch64.* lib
cp -L $SYSROOT/lib64/libm.so.* lib64
cp -L $SYSROOT/lib64/libresolv.so.* lib64
cp -L $SYSROOT/lib64/libc.so.* lib64


# TODO: Make device nodes

sudo mknod -m 666 dev/null c 1 3

sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
#cd ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

# used absolute path
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/finder-test.sh ${OUTDIR}/rootfs/home
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/finder.sh ${OUTDIR}/rootfs/home
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/writer.sh ${OUTDIR}/rootfs/home
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/writer.c ${OUTDIR}/rootfs/home
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/writer ${OUTDIR}/rootfs/home
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/Makefile ${OUTDIR}/rootfs/home
#cp -r ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/conf/ ${OUTDIR}/rootfs/home
#cp  ~/Desktop/aesd/assignments-3-and-later-midhun9/finder-app/autorun-qemu.sh ${OUTDIR}/rootfs/home

cp  ./finder-test.sh ${OUTDIR}/rootfs/home
cp  ./finder.sh ${OUTDIR}/rootfs/home
cp  ./writer.sh ${OUTDIR}/rootfs/home
cp  ./writer.c ${OUTDIR}/rootfs/home
cp  ./writer ${OUTDIR}/rootfs/home
cp  ./Makefile ${OUTDIR}/rootfs/home
cp -r ./conf/ ${OUTDIR}/rootfs/home
cp  ./autorun-qemu.sh ${OUTDIR}/rootfs/home


# TODO: Chown the root directory

cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz

find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio

