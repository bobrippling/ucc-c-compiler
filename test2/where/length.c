// we don't want length overrunning, clip at 12
// RUN: [ `%ucc -fsyntax-only %s 2>&1| perl -e 'my $l = join("", <>); $n = ($l =~ s/~//g); print "$n\n"'` -le 12 ]

f();

main()
{
int lots_of_spacing_here = 0;
f(lots_of_spacing_here + lots_of_spacing_here + longly_worded_void_pointer_fn_name());
return 0;
}
