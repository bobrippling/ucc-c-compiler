// RUN: %ucc -S -o- %s | grep '\(common\|space\)' | grep -vF .globl | %output_check 'nocommon:' '.space 4 # object space' '.comm common,4,4'

int nocommon = 0;
int common;
