// RUN: %check %s

enum A { X, Y } f() // CHECK: !/warn/
{
	return 5; // CHECK: !/warn/
}
