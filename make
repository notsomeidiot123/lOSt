echo -e "\e[31;1;47mStarting Compilation!!!\e[0m"
nasm -f bin bootloader/mbr/main.s -o bin/mbr.o
cat bin/mbr.o > bin/lOSt.bin
qemu-img resize bin/lOSt.bin 1M

nasm -f bin bootloader/boot/main.s -o bin/bootloader.o
nasm -f elf kernel/kentry.s -o bin/kentry.o
nasm -f elf kernel/cpu/idt.s -o bin/idt.o
nasm -f elf kernel/sizetest.s -o bin/sztest.o
nasm -f elf kernel/proc/scheduler.s -o bin/scheduler.o

CFLAGS="-g -c -m32 -ffreestanding -fno-pie -mno-sse -O0 -Wno-int-to-pointer-cast -Wno-incompatible-pointer-types -fno-stack-protector -o"
cd kernel/
gcc main.c $CFLAGS obj/main.o
for d in ./*/; do
    if [ "$d" != "./obj/" ]; then
        echo -e "\033[1;32mCompiling files in $d\033[0m"
        for f in $d*.c; do
            echo -e "\033[1;36mfile: $f\033[0m"
            SUF=".c"
            SUF_REM=${f%"$SUF"}
            END=${SUF_REM#"$d"}
            # doing this, '#' removes prefix, and '%' removes suffix
            gcc $f $CFLAGS obj/$END.o
        done
        for f in $d*.cpp; do
            # I give up. it won't compile anything either way if it's an invalid file, so i guess im safe... for now
            SUF=".cpp"
            SUF_REM=${f%"$SUF"}
            END=${SUF_REM#"$d"}
            g++ $f $CFLAGS obj/$END.o
        done
    fi
done
echo
cd obj
ld ../../bin/kentry.o *.o ../../bin/idt.o ../../bin/sztest.o ../../bin/scheduler.o -Ttext 0x10000 --oformat binary -melf_i386 -o ../../bin/base.o
ls *.o
cd ../../
cat bin/bootloader.o bin/base.o >> bin/lOSt.bin

echo -e "\e[31m STATING KERNEL\e[0m"

stat bin/lOSt.bin

qemu-img resize bin/lOSt.bin 256M

echo "formatting:"


qemu-img resize --shrink drives/data.hd 768M 
# mkfs.fat -F 32 drives/data.hd 
mkfs.fat -F 32 drives/testhd.hd

cat drives/data.hd >> bin/lOSt.bin
qemu-img resize --shrink bin/lOSt.bin 1G
echo "partitioning"
parted bin/lOSt.bin mkpart primary 0% 25%\
        set 1 boot on
# parted bin/lOSt.bin mkpart primary fat32 25% 100%
# parted drives/testhd.hd primary fat32 0% 100%
echo -e "\nfinished"

./run

# hexdump -C bin/lOSt.bin > dump.hd
echo -e "\e[31mEnd\e[0m"