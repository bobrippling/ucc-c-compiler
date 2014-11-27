OS = $(shell uname)

ifeq(${OS},Darwin)
LDFLAGS = -Wl,-allow_stack_execute
else
LDFLAGS = -z execstack
endif

lambda: lambda.o
	cc -o $@ $< ${LDFLAGS}

lambda.o: lambda.s
	as -o $@ $<
