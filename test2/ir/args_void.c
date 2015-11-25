// RUN: %ircheck %s
any_args();
no_args(void);
int_then_any(int i, ...);

// ir:
$_any_args = i4(...)
$_no_args = i4()
$_int_then_any = i4(i4 $i, ...)
