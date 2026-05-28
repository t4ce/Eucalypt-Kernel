global rdrand64
rdrand64:
    mov ecx, 10
.retry:
    rdrand rax
    jc .done
    loop .retry
    xor eax, eax
    ret
.done:
    mov qword [rdi], rax
    mov eax, 1
    ret