// RUN: %ucc -fsyntax-only %s

int main()
{
    /* comment with stray handling *\
/
       /* this is a valid *\/ comment */
       /* this is a valid comment *\*/
    //  this is a valid\
comment

    return 0;
}

