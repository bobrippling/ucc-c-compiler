// RUN: %ucc -target x86_64-linux -S -o %t %s
// RUN: grep -F '.section initialdata,"aw",@progbits' %t
// RUN: grep -F '.section initialfunc,"ax",@progbits' %t
// RUN: grep -F '.section initialdataro,"a",@progbits' %t

int initialdata __attribute((section("initialdata")));

__attribute((section("initialdata")))
void initialdata2()
{
}

// ---

__attribute((section("initialfunc")))
void initialfunc()
{
}

int initialfunc2 __attribute((section("initialfunc")));

// ---

const int initialdataro __attribute((section("initialdataro")));
