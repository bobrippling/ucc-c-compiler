// RUN: %ucc %s; [ $? -ne 0 ]

int foo() {
	static bar();
	static (*pbar)() = bar;

}

static bar() {
	return 1;
}

static (*pbar)() = 0;
