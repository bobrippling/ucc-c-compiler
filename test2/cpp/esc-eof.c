// RUN: %ucc -E %s | grep -vF '\\'
// RUN: %ucc -E %s 2>&1 >/dev/null | grep 'backslash-escape at eof'

hi
there\
