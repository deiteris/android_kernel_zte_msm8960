#!/bin/bash
cp ~/bootbuild/zte/frosty/arch/arm/boot/zImage ~/bootbuild
~/bootbuild/mkbootimg --kernel ~/bootbuild/zImage --ramdisk ~/bootbuild/initramfs.cpio.gz --cmdline 'androidboot.hardware=qcom user_debug=31 loglevel=7 kgsl.mmutype=gpummu' --base 0x80200000 --pagesize 2048 --ramdiskaddr 0x81500000 -o ~/bootbuild/boot.img
find ~/bootbuild/zte/frosty -name '*.ko' -exec cp {} ~/bootbuild/modules  \;
