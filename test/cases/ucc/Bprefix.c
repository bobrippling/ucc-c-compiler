// RUN: %ucc -'###' -Bthe-prefix %s >%t 2>&1
// RUN: grep -E '^the-prefix/+cpp .*-isystem the-prefix/+include' %t
// RUN: grep -E '^the-prefix/+cc1' %t
// don't test this - linux only: grep -E '^ld .* the-prefix/+../rt/dsohandle.o' %t
