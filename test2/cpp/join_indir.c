// RUN: %ucc -E %s | grep 'return 42'

#define J(a,b) a##b
#define FB 42
main(){return J(F, B);}
