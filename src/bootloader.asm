        SZ_2MB equ 0x200000
        SZ_QW equ 8

        ; Segment Descriptor constants

        ; Access byte format:
        ; 8: Segment is present (1)
        ; 7-6: ring (0)
        ; 5: Descriptor (1 for code or data segments, 0 for task state segments)
        ; 4: Executable (1 for code, 0 for data)
        ; 3: Direction for data (0 grows up, 1 grows down) / conforming for code (0 this ring only, 1 this or lower)
        ; 2: Readable (for code) / writable (for data)
        ; 1: CPU access (leave clear)

        ; Access byte is bits 40-47, so bit shift from 40 to 47 for bits 0-7 of access byte
        SD_PRESENT equ 1<<47
        SD_RING0 equ 0
        SD_RING1 equ 1<<45
        SD_RING2 equ 2<<45
        SD_RING3 equ 3<<45
        SD_NONTSS equ 1<<44
        SD_TSS equ 0
        SD_CODESEG equ 1<<43
        SD_DATASEG equ 0
        SD_DATADOWN equ 1<<42
        SD_DATAUP equ 0
        SD_CONFORMING equ 1<<42
        SD_READABLE equ 1<<41
        SD_WRITABLE equ 1<<41

        ; Flags nibble format:
        ; 4: Granularity (0 for 1-byte blocks; 1 for 4-KiB blocks)
        ; 3: Size (0 for 16-bit protected-mode segment; 1 for 32-bit protected-mode segment)
        ; 2: Long mode (1 for 64-bit protected mode segment: clear bit 3 if turning this bit on; 64 is not 32 or 16)
        ; 1: Reserved

        ; Flags nibble is bits 52-55, so bit shift from 52 to 55 for bits 0-3 of access byte
        SD_GRAN4K equ 1<<55
        SD_GRANBYTE equ 0
        SD_MODE16 equ 0
        SD_MODE32 equ 1<<54
        SD_MODE64 equ 1<<53

        page_table_l4 equ 0x1000
        page_table_l3 equ 0x2000
        page_table_l2 equ 0x3000
        int_15_mem_table equ 0x4000
        page_tables_l2 equ 0x100000
        stack_top equ 0x7bff
        idt equ 0               ; 0-0x1000 available in long mode

        ; Page Table constants
        PT_PRESENT equ 1
        PT_WRITABLE equ 1<<1
        PT_HUGE equ 1<<7

        CR4_PAE equ 1<<5
        CR0_PROTECTION equ 1
        CR0_PAGING equ 1 << 31

        MSR_IA32_EFER equ 0xC0000080 ; Extended Feature Enable Register
        EFER_LONG_MODE_ENABLE equ 1<<8

        ; Let's load 960 sectors, 120 at a time (128 is max at a time, 961 total is max in safe area)
        ; Well, VirtualBox, and apparently some other BIOSes, can only handle reading one sector at a time...
        SECT_PER_LOAD equ 1
        LOAD_COUNT equ 960

        INT_0x10_TELETYPE equ 0x0e
        INT_0x13_LBA_READ equ 0x42

section .boot
bits 16
        jmp 0:start16           ; Make sure cs is set to 0
start16:
        mov ax, cs
        mov ds, ax
        mov es, ax
        mov ss, ax

        mov esp, stack_top

        mov ax, 3
        int 0x10

        ; Enable A20 bit
        mov ax, 0x2401
        int 0x15

        mov ch, 0
        mov cl, 2
        mov dh, 0
        mov bx, sect2
        mov ah, 2
        mov al, 1
        int 0x13

        cli

        mov eax, page_table_l3 | PT_PRESENT | PT_WRITABLE
        mov [page_table_l4], eax

        mov eax, page_table_l2 | PT_PRESENT | PT_WRITABLE
        mov [page_table_l3], eax

        mov eax, PT_PRESENT | PT_WRITABLE | PT_HUGE
        mov ebx, page_table_l2
        mov ecx, 512
l2_loop:
        mov [ebx], eax
        add eax, SZ_2MB       ; Huge page bit makes for 2MB pages, so each page is this far apart
        add ebx, SZ_QW
        loop l2_loop

        mov eax, page_table_l4
        mov cr3, eax

        mov eax, cr4
        or eax, CR4_PAE
        mov cr4, eax

        mov ecx, MSR_IA32_EFER
        rdmsr
        or eax, EFER_LONG_MODE_ENABLE
        wrmsr

        lgdt [gdtr]

        mov eax, cr0
        or eax, CR0_PAGING | CR0_PROTECTION
        mov cr0, eax

        jmp CODE_SEG:start64

bits 64

        ; Note on my note: I haven't ever tried 32-bit protected mode with paging enabled, and I'd forgotten that
        ;   I wrote that Intel says we specifically need that on and then back off...

        PTABLE_PRESENT equ 1
        PTABLE_WRITABLE equ 1<<1
        PTABLE_HUGE equ 1<<7

        PIC_PRIMARY_CMD equ 0x20
        PIC_PRIMARY_DATA equ 0x21
        PIC_SECONDARY_CMD equ 0xa0
        PIC_SECONDARY_DATA equ 0xa1

        ICW1 equ 1<<4
        ICW1_ICW4_NEEDED equ 1

        CODE_SEG equ gdt.code - gdt
gdt:
        dq 0
.code:
        dq SD_PRESENT | SD_NONTSS | SD_CODESEG | SD_READABLE | SD_GRAN4K | SD_MODE64
gdtr:
        dw $ - gdt - 1          ; Length in bytes minus 1
        dq gdt                  ; Address

idtr:
        dw 4095                 ; Size of IDT minus 1
        dq idt                  ; Address

start64:
        xor ax, ax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        mov ss, ax

        mov ebx, idt
        mov ecx, 256
loop_idt2:
        mov rax, keyboard_gate
        mov [ebx], ax
        mov [ebx + 4], rax
        mov word [ebx + 2], CODE_SEG
        mov word [ebx + 4], 1 << 15 | 0b1110 << 8
        add ebx, 16
        loop loop_idt2

        mov al, ICW1 | ICW1_ICW4_NEEDED
        out PIC_PRIMARY_CMD, al

        mov al, 0x20
        out PIC_PRIMARY_DATA, al        ; Map primary PIC to 0x20 - 0x27

        mov al, 0x04
        out PIC_PRIMARY_DATA, al

        mov al, 0x01
        out PIC_PRIMARY_DATA, al

        mov al, 0xfd
        out PIC_PRIMARY_DATA, al

        lidt [idtr]

        sti

        jmp sect2

        BOOTABLE equ 1<<7
        times 440 - ($-$$) db 0
        dd 0
        dw 0
        db BOOTABLE
        db 0           ; Head of first sector of partition
        db 5           ; Sector of first sector of partition
        db 0           ; Cylinder of first sector of parition
        db 1           ; FAT12
        db 18          ; Head of last sector of partition
        db 18          ; Sector of last sector of partition
        db 0           ; Cylinder of last sector of parition
        dd 2           ; LBA of partition start
        dd 0x3c2                ; Number of sectors in partition
        times 510 - ($-$$) db 0
        dw 0xaa55

sect2:
        jmp sect2code

keyboard_gate:
        push rax

        in al, 0x60

        mov [0xb8000 + 160], al
        mov byte [0xb8000 + 160 + 1], 0x0d

        mov al, 0x20
        out PIC_PRIMARY_CMD, al

        pop rax

        iretq

sect2code:
        mov byte [0xb8000], '@'

halt_loop:
        hlt
        jmp halt_loop

