// RUN: %check --only %s

int a1(unsigned q){ return q <= 0; }
int b1(unsigned q){ return q <  0; } // CHECK: warning: comparison of unsigned expression < 0 is always false
int c1(unsigned q){ return q >  0; }
int d1(unsigned q){ return q >= 0; } // CHECK: warning: comparison of unsigned expression >= 0 is always true
int e1(unsigned q){ return q == 0; }
int f1(unsigned q){ return q != 0; }

int a2(unsigned q){ return 0 >  q; } // CHECK: warning: comparison of 0 > unsigned expression is always false
int b2(unsigned q){ return 0 >= q; }
int c2(unsigned q){ return 0 <= q; } // CHECK: warning: comparison of 0 <= unsigned expression is always true
int d2(unsigned q){ return 0 <  q; }
int e2(unsigned q){ return 0 == q; }
int f2(unsigned q){ return 0 != q; }

int a3(unsigned q){ return q <= 0u; }
int b3(unsigned q){ return q <  0u; } // CHECK: warning: comparison of unsigned expression < 0 is always false
int c3(unsigned q){ return q >  0u; }
int d3(unsigned q){ return q >= 0u; } // CHECK: warning: comparison of unsigned expression >= 0 is always true
int e3(unsigned q){ return q == 0u; }
int f3(unsigned q){ return q != 0u; }

int a4(unsigned q){ return 0u >  q; } // CHECK: warning: comparison of 0 > unsigned expression is always false
int b4(unsigned q){ return 0u >= q; }
int c4(unsigned q){ return 0u <= q; } // CHECK: warning: comparison of 0 <= unsigned expression is always true
int d4(unsigned q){ return 0u <  q; }
int e4(unsigned q){ return 0u == q; }
int f4(unsigned q){ return 0u != q; }

