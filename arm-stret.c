typedef struct A {
  unsigned short a, b;
} A;

typedef struct B {
  int a, b, c;
} B;

A ret_sm(){
  return (A){1,2};
}

int get_sm(){
  __auto_type a = ret_sm();
  return a.b;
}

B ret_lg()
#if defined(IMPL) || defined(BOTH)
{
  return (B){ 1, 2, 3 };
}
#else
;
#endif

int get_lg(){
  __auto_type a = ret_lg();
  return a.b;
}

#if !defined(IMPL) || defined(BOTH)
int printf(const char *, ...);
int main(){
  __auto_type x = ret_lg();
  printf("{ %d, %d, %d }\n", x.a, x.b, x.c);
}
#endif
