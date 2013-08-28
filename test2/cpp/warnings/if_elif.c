// RUN: %check %s -E

#if undefined_word // CHECK: /warning: undefined identifier/
never see me
#else
hi
#endif
