// (disabled) RUN: %ucc -S -o- -target x86_64-linux %s -fno-common -fdata-sections | %stdoutcheck %s
// RUN: true

// no way to trigger this - .bss is defaulted, any custom section can't assume @nobits

__thread int zero_init;
// STDOUT: .section .bss,"awT",@nobits
//                             ^~~~~~~
// STDOUT: zero_init:
