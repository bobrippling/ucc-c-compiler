// RUN: %ucc -MG -MM %s | grep 'deps_MG.o: .*deps_MG.c a/doesnt_exist.h'

#include "a/doesnt_exist.h"
