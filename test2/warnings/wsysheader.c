// RUN: %check --prefix=no_flags %s -x cpp-output -Wimplicit
// RUN: %check --prefix=wnosysh  %s -x cpp-output -Wimplicit -Wno-system-headers
// RUN: %check --prefix=wsysh    %s -x cpp-output -Wimplicit -Wsystem-headers

// flag "3" means system header:
# 7 "stdio.h" 1 3
// ^ the above 5 is volatile to change

f(); // CHECK-no_flags: !/warning/
// CHECK-wnosysh: ^ !/warning/
// CHECK-wsysh: ^^ /warning/

# 2 "src.c" 1

int main(void)
{
	return 0;
}
