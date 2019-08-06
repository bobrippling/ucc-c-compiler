// RUN: %ucc -P -E %s | %stdoutcheck %s
//      STDOUT: cpp/base_file.c
// STDOUT-NEXT: cpp/base_file.c
// STDOUT-NEXT: cpp/base_file.c
// STDOUT-NEXT: cpp/base_file.h

__BASE_FILE__
__FILE__

#include "base_file.h"
