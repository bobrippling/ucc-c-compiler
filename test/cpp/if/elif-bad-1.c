// RUN: %ucc -E %s; [ $? -ne 0 ]
#elif 1 > 0
#endif
