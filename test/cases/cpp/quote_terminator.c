// RUN: %ucc -P -E %s | %stdoutcheck %s

#define abc 5

'"' abc
"'" abc

// STDOUT: '"' 5
// STDOUT-NEXT: "'" 5
