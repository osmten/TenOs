# Directories
BOOT_DIR    = boot
KERNEL_DIR  = kernel
CPU_DIR     = cpu
DRIVERS_DIR = drivers
FS_DIR      = fs
MM_DIR      = mm
LIB_DIR     = lib
USR_DIR		= user
PROC_DIR	= process

# Source files
C_SOURCES = $(wildcard $(KERNEL_DIR)/*.c) \
            $(wildcard $(DRIVERS_DIR)/*.c) \
            $(wildcard $(CPU_DIR)/*.c) \
            $(wildcard $(LIB_DIR)/*.c) \
            $(wildcard $(MM_DIR)/*.c) \
            $(wildcard $(FS_DIR)/*.c) \
			$(wildcard $(USR_DIR)/*.c) \
			$(wildcard $(PROC_DIR)/*.c)

HEADERS   = $(wildcard $(KERNEL_DIR)/*.h) \
            $(wildcard $(DRIVERS_DIR)/*.h) \
            $(wildcard $(CPU_DIR)/*.h) \
            $(wildcard $(LIB_DIR)/*.h) \
            $(wildcard $(MM_DIR)/*.h) \
            $(wildcard $(FS_DIR)/*.h) \
			$(wildcard $(USR_DIR)/*.h) \
			$(wildcard $(PROC_DIR)/*.h)

ASM_SOURCES = $(CPU_DIR)/interrupt.asm \
				$(CPU_DIR)/kernel_gdt.asm \
				$(CPU_DIR)/syscall_handler.asm \
				$(PROC_DIR)/jump_usermode.asm

OBJ = ${C_SOURCES:.c=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

CC = i686-elf-gcc 
GDB = i686-elf-gdb
CFLAGS = -g -ffreestanding -O0 -fno-inline -I.
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
	# Write test file (LBA 200)
	dd if=/dev/zero of=hda.img seek=200 bs=512 count=1 conv=notrunc
	dd if=hda.img skip=200 bs=512 count=1 | hexdump -C

kernel.bin: boot/kernel_entry.o ${OBJ} ${ASM_OBJ}
	i686-elf-ld -o $@ -T kernel.ld $^ --oformat binary -Map kernel.map

kernel.elf: boot/kernel_entry.o ${OBJ} ${ASM_OBJ}
	i686-elf-ld -T kernel.ld -o $@ $^ 

run: hda.img
	qemu-system-i386 -hda hda.img

kernel_disassembly.txt: kernel.elf
	$(DISASM) -d $< > $@

# debug: hda.img kernel.elf
# 	qemu-system-i386 -hda hda.img -s &
# 	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

debug: hda.img kernel.elf
	qemu-system-i386 -hda hda.img -d guest_errors,int & 
	${GDB} -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o *.elf hda.img kernel.map
	rm -rf $(KERNEL_DIR)/*.o $(BOOT_DIR)/*.bin $(BOOT_DIR)/*.o
	rm -rf $(DRIVERS_DIR)/*.o $(CPU_DIR)/*.o $(LIB_DIR)/*.o
	rm -rf $(MM_DIR)/*.o $(FS_DIR)/*.o $(USR_DIR)/*.o