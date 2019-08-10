// RUN: %check %s

f() // CHECK: warning: control reaches end of non-void function f
{
	goto end;
end:;
}

g() // CHECK: !/warn.*control/
{
end:
	goto end;
}

get_3()
{
	return 3;
}

main() // CHECK: !/warn.*control/
{
	get_3(); // set up %eax to return 3

	goto end;
end:;
}
