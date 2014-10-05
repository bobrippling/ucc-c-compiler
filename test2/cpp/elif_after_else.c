// RUN: %check -e %s
#if 0
#else
hello
#elif exp // CHECK: error: #else already encountered (now at #elif)
#endif
