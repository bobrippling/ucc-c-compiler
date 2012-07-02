#!/usr/bin/perl
use warnings;

assert_output("addr", "-\n");
assert_output("alphabet", "\"abcdefghijklmnopqrstuvwxyz\"\n");

assert_success("assign_order");

assert_output_args("atoi", "3", "3\n");

assert_output("break", "while, i = 0
while, i = 1
while, i = 2
while, i = 3
while, i = 4
while, i = 5
for, i = 5
for, i = 6
for, i = 7
for, i = 8
for, i = 9
do, i = 10
do, i = 11
do, i = 12
do, i = 13
do, i = 14
do, i = 15
");

assert_exit_code("brk", 5);


assert_io("cat", "hi\n", "hi\n");

assert_output("late_decl", "strs[0] = hi
strs[1] = there
strs[2] = tim
");


