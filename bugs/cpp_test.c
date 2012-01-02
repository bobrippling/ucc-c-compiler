// vim: ft=c
#define a
//#define b
#define c

#ifdef a
#  ifdef b
#    ifdef c
all defined
#    else
a_and_b defined
#    endif
#  else
#    ifdef c
a_and_c defined
#    else
just_a defined
#    endif
#  endif
#else
#  ifdef b
#    ifdef c
b_and_c defined
#    else
just_b defined
#    endif
#  else
#    ifdef c
just_c defined
#    else
nothing defined
#    endif
#  endif
#endif

#undef a
#ifdef a
should never see this
#endif
