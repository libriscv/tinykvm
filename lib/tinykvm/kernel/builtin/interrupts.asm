[BITS 64]
global vm64_exception

%macro CPU_EXCEPT 1
ALIGN 0x10
	;; exception trap
	mov ax, %1
	mov dx, 0xFFFF
	out dx, ax
	o64 iret
%endmacro
%macro CPU_EXCEPT_CODE 1
ALIGN 0x10
	;; exception trap
	mov ax, %1
	mov dx, 0xFFFF
	out dx, ax
	pop rax
	o64 iret
%endmacro

org 0x2000
dw .vm64_syscall
dw .vm64_gettimeofday
dw .vm64_exception
dw .vm64_dso

.vm64_syscall:
	cmp eax, 158 ;; PRCTL
	je .vm64_prctl
	add eax, 0xffffa000
	mov DWORD [eax], 0
	o64 sysret

.vm64_prctl:
	push rsi
	push rcx
	push rdx
	cmp rdi, 0x1002 ;; PRCTL
	jne .vm64_prctl_trap
	mov rcx, 0xC0000100  ;; FSBASE
	mov eax, esi ;; low-32 FS base
	shr rsi, 32
	mov edx, esi ;; high-32 FS base
	wrmsr
	xor rax, rax ;; return 0
.vm64_prctl_end:
	pop rdx
	pop rcx
	pop rsi
	o64 sysret
.vm64_prctl_trap:
	out 158, ax
	jmp .vm64_prctl_end

.vm64_gettimeofday:
	out 96, ax
	ret

.vm64_dso:
	mov rax, .vm64_gettimeofday
	ret

ALIGN 0x10
.vm64_exception:
	CPU_EXCEPT 0
	CPU_EXCEPT 1
	CPU_EXCEPT 2
	CPU_EXCEPT 3
	CPU_EXCEPT 4
	CPU_EXCEPT 5
	CPU_EXCEPT 6
	CPU_EXCEPT 7
	CPU_EXCEPT_CODE 8
	CPU_EXCEPT 9
	CPU_EXCEPT_CODE 10
	CPU_EXCEPT_CODE 11
	CPU_EXCEPT_CODE 12
	CPU_EXCEPT_CODE 13
	CPU_EXCEPT_CODE 14
	CPU_EXCEPT 15
	CPU_EXCEPT 16
	CPU_EXCEPT_CODE 17
	CPU_EXCEPT 18
	CPU_EXCEPT 19
	CPU_EXCEPT 20
