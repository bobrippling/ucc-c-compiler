typedef int ifun();

//ifun main, a, b, c, d, e, g, h, i, j, k;
int a(){return 1;}
int b(){return 2;}
int c(){return 3;}
int d(){return 4;}
int e(){return 5;}
int g(){return 6;}
int h(){return 7;}
int i(){return 8;}
int j(){return 9;}
int k(){return 10;}

int f(
    int a, int b, int c, int d,
    int e, int g, int h, int i, int j, int k)
{
  printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
      a, b, c, d, e, g, h, i, j, k);
}

main(){
  return f(
    a(),
    b(),
    c(),
    d(),
    e(),
    g(),
    h(),
    i(),
    j(),
    k()
  );
}
