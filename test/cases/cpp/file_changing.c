// RUN: %ucc -std=c89 -fsyntax-only %s 2>&1 | grep 'this is the filename:336:9: warning: implicit declaration of function "g"'
// C89 for implicit function decl

#line 333 "this is the filename"

void f(void)
{
        g("FOO");
}
