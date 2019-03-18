// RUN: %ucc -fno-leading-underscore -mno-align-is-p2 -S -o- %s | grep '\(common\|space\)' | grep -vE '\.(globl|type|size)' | %output_check 'nocommon:' '.space 4 # object space' '.comm common,4,4'

int nocommon = 0;
int common;
