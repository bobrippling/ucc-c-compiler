; offset += bytecode[offset + 1]
add    0x8(%r10,%rsi,8), %esi

; jmp *bytecode[offset]
jmpq   *(%r10,%rsi,8)

; magic!
