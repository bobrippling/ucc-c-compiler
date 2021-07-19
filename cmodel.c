// -mcmodel=large/medium/small/etc
typedef void fn(int);

fn a, b;

void f(int x) {
  (x ? a : b)(3);
}
