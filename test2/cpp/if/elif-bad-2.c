// RUN: %ucc -E %s; [ $? -ne 0 ]
#if 0
#elif
#endif
