// RUN: %check -e %s -DSTMT=if
// RUN: %check -e %s -DSTMT=while
// RUN: %check -e %s -DSTMT=switch

main()
{
	STMT(int i = 5);
	return i; // CHECK: /error: undeclared identifier "i"/
}
