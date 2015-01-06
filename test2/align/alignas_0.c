// RUN: %ucc -fno-common -S -o- %s | grep -i '\.align \+4'
_Alignas(0) int i;
