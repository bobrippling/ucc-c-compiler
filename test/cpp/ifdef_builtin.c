// RUN: %ucc -E %s | grep 'date defined'

#ifdef __DATE__
date defined
#endif
