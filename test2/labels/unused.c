// RUN: %check %s

main()
{
a:; // CHECK: /warning: unused label 'a'/
}
