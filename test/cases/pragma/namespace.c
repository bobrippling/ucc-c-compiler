// RUN: %check %s

#pragma ucc namespace yo_

static void private() // CHECK: !/warn/
{
}

void yo_hi() // CHECK: !/warn/
{
	private();
}

static int get_x() // CHECK: !/warn/
{
	return 3;
}

int hi() // CHECK: warning: non-static function not in "yo_" namespace
{
	return get_x();
}
