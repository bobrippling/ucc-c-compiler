// RUN: %ucc -fsyntax-only %s 2>&1 | grep 'this is the filename:336:9: warning: implicit declaration of function "g"'

#line 333 "this is the filename"

void f(void)
{
        g("FOO");
}
