// RUN: %ucc -S -o %t %s
// RUN: grep 'globl.*internal4' %t; [ $? -ne 0 ]
// RUN: grep 'globl.*internal5' %t; [ $? -ne 0 ]
// RUN: grep 'globl.*internal6' %t; [ $? -ne 0 ]

typedef void func(void);

// declared static to begin with - later nothing- and extern-defs have no effect

static func internal4;
func internal4;
extern func internal4;

void internal4(void)
{
}


// declared static to begin with - later static- and extern-defs have no effect

static func internal5;
func internal5;
extern func internal5;
static func internal5;

void internal5(void)
{
}


// declared static to begin with - later static- and extern-defs have no effect

static func internal6;
func internal6;
extern func internal6;
static func internal6;

static void internal6(void)
{
}
