// RUN: %ucc -fno-leading-underscore -mno-align-is-p2 -S -o- %s | grep 'common\|space' | grep -vE '\.(globl|type|size)' | %stdoutcheck %s
// STDOUT: nocommon:
// STDOUT-NEXT: .space 4 # object space
// STDOUT-NEXT: .comm common,4,4

int nocommon = 0;
int common;
