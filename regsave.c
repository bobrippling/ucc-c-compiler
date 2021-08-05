typedef int ifun();

ifun main, a, b, c, d, e, g, h, i, j, k;

int f(int, ...);

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
