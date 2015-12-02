// RUN: %check --prefix=sysh %s -Weverything -Wc,-I/system
// RUN: %check --prefix=norm %s -Weverything -Wc,-I/system -Wno-system-headers

# 5 "/system/sys.h"
// ^ the above 5 is volatile to change

f(); // CHECK-sysh: /warning/
// CHECK-norm: ^ !/warning/

# 2 "src.c"

int main(void)
{
	return 0;
}
