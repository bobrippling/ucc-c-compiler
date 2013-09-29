// RUN: %ucc -E %s | grep '^foo 1 a foo 1 2 b$'

#define foo bar a baz b
#define bar foo 1
#define baz bar 2
foo
