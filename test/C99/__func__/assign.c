// RUN: %check -e %s

hello()
{
	__func__[2] = 'a'; // CHECK: /error: can't modify const expression/
	__FUNCTION__[2] = 'a'; // CHECK: /error: can't modify const expression/
}
