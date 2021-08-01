// undefine a builtin macro
// RUN: %ucc -E -U__STDC__ %s | grep 'got A'

got __STDC__
