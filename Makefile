C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h)
ASM_SOURCES = cpu/interrupt.asm

OBJ = ${C_SOURCES:.c=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

CC = i686-elf-gcc 
GDB = i686-elf-gdb
CFLAGS = -g -ffreestanding -O0 -fno-inline
DISASM = i686-elf-objdump

hda.img: boot/bootsect.bin boot/stage2.bin kernel.bin
    # Create a 2GB empty disk image
	dd if=/dev/zero of=hda.img bs=1M count=2048
    # Write bootsector (LBA 0)
	dd if=boot/bootsect.bin of=hda.img conv=notrunc
    # Write stage2 (LBA 1-2)
	dd if=boot/stage2.bin of=hda.img seek=1 conv=notrunc
    # Write kernel (LBA 3-108)
	dd if=kernel.bin of=hda.img seek=3 conv=notrunc
    # Format remaining space as FAT12 (optional)
    # mkfs.fat -F12 -s 1 -R 32 hda.img
	dd if=/dev/zero of=hda.img seek=200 bs=512 count=1 conv=notrunc
	dd if=test.txt of=hda.img seek=200 bs=512 conv=notrunc
	dd if=hda.img skip=200 bs=512 count=1 | hexdump -C

#os-image.bin: boot/bootsect.bin boot/stage2.bin kernel.bin
#	cat $^ > os-image.bin

kernel.bin: boot/kernel_entry.o ${OBJ} ${ASM_OBJ}
	i686-elf-ld -o $@ -T kernel.ld $^ --oformat binary -Map kernel.map

kernel.elf: boot/kernel_entry.o ${OBJ} ${ASM_OBJ}  # <-- ADDED ${ASM_OBJ}
	i686-elf-ld -T kernel.ld -o $@ $^ 

#qemu-system-i386 -hda os-image.bin

run: hda.img
	qemu-system-i386 -hda hda.img

kernel_disassembly.txt: kernel.elf
	$(DISASM) -d $< > $@

debug: os-image.bin kernel.elf
	qemu-system-i386 -hda os-image.bin -d guest_errors,int & 
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image.bin *.elf
	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o cpu/*.o


# C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c libc/*.c)
# HEADERS = $(wildcard kernel/*.h drivers/*.h cpu/*.h libc/*.h)
# ASM_SOURCES = cpu/interrupt.asm

# OBJ = ${C_SOURCES:.c=.o}
# ASM_OBJ = ${ASM_SOURCES:.asm=.o}

# CC = i686-elf-gcc 
# GDB = i686-elf-gdb
# CFLAGS = -ffreestanding -g -O2 -Wall -Wextra
# DISASM = i686-elf-objdump
# LD = i686-elf-ld

# # -----------------------------------------------------------------------------
# # Final image combines boot sectors + kernel binary
# # -----------------------------------------------------------------------------
# os-image.bin: boot/bootsect.bin boot/stage2.bin kernel.bin
# 	cat $^ > $@

# # -----------------------------------------------------------------------------
# # Kernel binary (flat image, no symbols)
# # -----------------------------------------------------------------------------
# kernel.bin: boot/kernel_entry.o ${OBJ} ${ASM_OBJ}
# 	$(LD) -T kernel.ld -o $@ $^ --oformat binary -Map kernel.map

# # -----------------------------------------------------------------------------
# # Kernel ELF (with symbols, for GDB debugging)
# # -----------------------------------------------------------------------------
# kernel.elf: boot/kernel_entry.o ${OBJ} ${ASM_OBJ}
# 	$(LD) -T kernel.ld -o $@ $^

# # -----------------------------------------------------------------------------
# # Run normally
# # -----------------------------------------------------------------------------
# run: os-image.bin
# 	qemu-system-i386 -hda os-image.bin

# # -----------------------------------------------------------------------------
# # Run with GDB debugging
# # -----------------------------------------------------------------------------
# debug: os-image.bin kernel.elf
# 	qemu-system-i386 -hda os-image.bin -s -S -nographic -d guest_errors,int & \
# 	$(GDB) -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

# # -----------------------------------------------------------------------------
# # Disassemble for inspection
# # -----------------------------------------------------------------------------
# kernel_disassembly.txt: kernel.elf
# 	$(DISASM) -d $< > $@

# # -----------------------------------------------------------------------------
# # Compilation rules
# # -----------------------------------------------------------------------------
# %.o: %.c ${HEADERS}
# 	${CC} ${CFLAGS} -c $< -o $@

# %.o: %.asm
# 	nasm $< -f elf -o $@

# %.bin: %.asm
# 	nasm $< -f bin -o $@

# # -----------------------------------------------------------------------------
# # Clean build artifacts
# # -----------------------------------------------------------------------------
# clean:
# 	rm -rf *.bin *.dis *.o os-image.bin *.elf kernel.map
# 	rm -rf kernel/*.o boot/*.bin drivers/*.o boot/*.o cpu/*.o libc/*.o
