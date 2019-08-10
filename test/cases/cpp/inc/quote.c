// RUN: %ucc -I"$(dirname "%s")" -P -E %s | %stdoutcheck %s
// STDOUT: A
// STDOUT: B
// STDOUT: C

#include <">
#include "\""
#include <\>>
