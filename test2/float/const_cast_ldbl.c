// RUN: %ucc -o %t %s
// RUN: %t | %output_check '-1.0' -10
// RUN: %check %s

float f = -1; // CHECK: !/warning:.*standard/
int i = -10.0f; // CHECK: !/warning:.*standard/

main()
{
	printf("%.1f\n", f);
	printf("%d\n", i);
}
