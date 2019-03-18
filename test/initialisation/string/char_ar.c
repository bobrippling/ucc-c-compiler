// RUN: %ucc -c %s -DPRE= -DPOST=
// RUN: %ucc -c %s -DPRE='main(){' -DPOST='}'

PRE
char x[] = "hello";
char y[] = { 'a', 'b' };
char *z = "yo";

int q[sizeof x]; // 6
POST
