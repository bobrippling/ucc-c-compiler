// RUN: %ucc -E %s -o /dev/null

#if 0 != (0 && (1 / 0))
#  error x
#endif

#if 1 != (-52 || (1 / 0))
#  error x
#endif

#if 5 != (3 ? 5 : (1 / 0))
#  error x
#endif
