// RUN: %check %s -E -Wundef

#if undefined_word // CHECK: /warning: undefined identifier/
never see me
#else
hi
#endif
