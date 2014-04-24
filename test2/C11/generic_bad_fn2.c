// RUN: %ocheck 0 %s

f();

i =  _Generic(f, int (*)(): 0);

main(){ return i; }
