// RUN: %ucc -fno-common -S -o- %s -mno-align-is-p2 | grep -i '\.align 4'
_Alignas(0) int i;
