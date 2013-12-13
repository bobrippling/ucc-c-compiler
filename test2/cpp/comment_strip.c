// RUN: %ucc -E -C %s  | grep -vF 'ucc -E' | grep >/dev/null 'yo comment'; [ $? -ne 0 ]
// RUN: %ucc -E -CC %s | grep -vF 'ucc -E' | grep >/dev/null 'yo comment'
// RUN: %ucc -E -CC %s | grep -vF 'ucc -E' | grep >/dev/null 'block'
// RUN: %ucc -E -C  %s | grep -vF 'ucc -E' | grep >/dev/null 'block'
// RUN: %ucc -E -CC %s | grep -vF 'ucc -E' | grep >/dev/null 'c++'
// RUN: %ucc -E -C  %s | grep -vF 'ucc -E' | grep >/dev/null 'c++'

#define YO // yo comment

/* block */
YO
// c++
