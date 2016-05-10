// RUN: %ucc -P -E %s | %stdoutcheck %s
#define QUOTE(x) #x

QUOTE("yo")
QUOTE("\"hi\there\"")

// STDOUT: "\"yo\""
// STDOUT-NEXT: "\"\\\"hi\\there\\\"\""
