// RUN: %check %s

main()
{
a: __attribute((unused)); // CHECK: !/warning/
}
