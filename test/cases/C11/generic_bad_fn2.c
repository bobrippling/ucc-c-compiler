// RUN: %check -e %s

f();

i =  _Generic(f, int (*)(): 0); // CHECK: error: no type satisfying int ()

main(){ return i; }
