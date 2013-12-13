// RUN: %ucc -c -o /dev/null %s
// RUN: [ `%ucc -S -o /dev/null %s 2>&1 | wc -l` -eq 0 ]
extern a();
int x = 0 && a(); // no warning
