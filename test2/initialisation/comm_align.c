// RUN: %ucc -fno-leading-underscore -S -o- %s | grep -F .comm | %output_check '.comm i,1,8' '.comm j,1,4' '.comm k,1,2'

char i __attribute((aligned(8)));
_Alignas(int) char j;
_Alignas(2) char k;
