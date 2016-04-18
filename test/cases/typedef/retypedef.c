// RUN: %check %s -std=c99
// RUN: %ucc -std=c11 %s | grep 'redefinition'; [ $? -ne 0 ]

typedef int int_t; // CHECK: /note:/
typedef int int_t; // CHECK: /warning: typedef 'int_t' redefinition/
typedef int_t int_t; // CHECK: /warning: typedef 'int_t' redefinition/

main()
{
	typedef int int_t; // CHECK: /note:/
	typedef int int_t; // CHECK: /warning: typedef 'int_t' redefinition/

	int_t x;
}
