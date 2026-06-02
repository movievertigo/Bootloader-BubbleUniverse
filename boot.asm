[bits 16]
[org 0x7c00]
    jmp start

    times 3 - ($-$$)   db 0x90
    OEMname:           db "mkfs.fat"
    bytesPerSector:    dw 0x0200
    sectPerCluster:    db 0x01
    reservedSectors:   dw 0x0001
    numFAT:            db 0x02
    numRootDirEntries: dw 0x00E0
    numSectors:        dw 0x0B40
    mediaType:         db 0xF0
    numFATsectors:     dw 0x0009
    sectorsPerTrack:   dw 0x0012
    numHeads:          dw 0x0002
    numHiddenSectors:  dd 0x00000000
    numSectorsHuge:    dd 0x00000000
    driveNum:          db 0x00
    reserved:          db 0x00
    signature:         db 0x29
    volumeID:          dd 0x12345678
    volumeLabel:       db "NO NAME    "
    fileSysType:       db "FAT12   "

start:
    cli                     ; disable interrupts

    xor ax, ax              ; zero register
    mov ds, ax              ; zero register
    mov es, ax              ; zero register
    mov ss, ax              ; zero register
    mov sp, 0x7C00          ; setup stack

    mov ah, 0x02            ; read sectors function
    mov ch, 0x00            ; cylinder 0
    mov dh, 0x00            ; head 0
    mov cl, 0x02            ; start from sector 2
    mov al, 0x08            ; sectors to read
    mov bx, 0x1000          ; buffer to load into
    int 0x13                ; call bios function

    mov ax, 0x4F02          ; VBE function: Set VBE mode
    mov bx, 0x4101           ; Mode 101h: 640x480x256 colors
    int 0x10                ; call bios function

    mov ax, 0x4F01          ; VBE function 0x4F01: Get Mode Info
    mov cx, 0x4101           ; Mode 0x101 = 640x480x256
    mov di, 0x8000          ; Put Mode Info Block at 0000:8000
    int 0x10

    lgdt [gdt_descriptor]   ; load GDT descriptor
    mov eax, cr0
    or eax, 0x1             ; enable protected mode
    mov cr0, eax
    jmp CODE_SEG:init32bit  ; jump to 32-bit code

[bits 32]
init32bit:
    mov ax, DATA_SEG        ; update segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x90000        ; setup stack
    mov ebp, esp

    mov eax, cr0
    and eax, ~(1<<2)        ; CR0.EM = 0  → do not trap FPU/SSE insns
    or eax,  (1<<1)|(1<<5)  ; CR0.MP = 1  → monitor FPU present
    mov cr0, eax
    finit                   ; x87 FPU init (alias FNINIT + FWAIT)

    call 0x1000             ; give control to the kernel
    jmp $                   ; loop in case kernel returns

gdt_start:
    dq 0
gdt_code:
    dq 0x00CF9A000000FFFF
gdt_data:
    dq 0x00CF92000000FFFF   
gdt_descriptor:
    dw gdt_descriptor - gdt_start - 1 ; size (16 bit)
    dd gdt_start ; address (32 bit)
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start



times 510 - ($-$$) db 0     ; padding
dw 0xaa55                   ; magic number
