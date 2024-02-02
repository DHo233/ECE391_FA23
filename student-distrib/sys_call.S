#define ASM     1

.align 4
sys_call_table:
    .long 0x0               # no system_call 0
    .long halt
    .long execute
    .long read
    .long write
    .long open
    .long close
    .long getargs
    .long vidmap
    .long set_handler
    .long sigreturn
    .long cp
    .long rm

.globl SYS_CALL_link

.align 4
SYS_CALL_link:
    # save registers except %eax
    pushl   %ebp
    pushl   %edi
    pushl   %esi
    pushl   %edx
    pushl   %ecx
    pushl   %ebx

    # check validity of call number
    cmpl    $0, %eax
    jle     invalid_syscall
    cmpl    $12,%eax
    jg      invalid_syscall

    # set args and call func
    pushl   %edx
    pushl   %ecx
    pushl   %ebx
    call    *sys_call_table(, %eax, 4)
    addl    $12, %esp
    jmp     sys_call_return

invalid_syscall:
    movl    $-1, %eax

sys_call_return:
    popl    %ebx
    popl    %ecx
    popl    %edx
    popl    %esi
    popl    %edi
    popl    %ebp
    iret
