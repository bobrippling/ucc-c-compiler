// RUN: %ucc -fno-leading-underscore -mno-align-is-p2 -S -o- %s | grep -F .comm | %stdoutcheck --prefix=nop2 %s
// STDOUT-nop2: .comm i,1,8
// STDOUT-NEXT-nop2: .comm j,1,4
// STDOUT-NEXT-nop2: .comm k,1,2
//
// RUN: %ucc -fno-leading-underscore    -malign-is-p2 -S -o- %s | grep -F .comm | %stdoutcheck --prefix=p2 %s
// STDOUT-p2: .comm i,1,3
// STDOUT-NEXT-p2: .comm j,1,2
// STDOUT-NEXT-p2: .comm k,1,1

char i __attribute((aligned(8)));
_Alignas(int) char j;
_Alignas(2) char k;
