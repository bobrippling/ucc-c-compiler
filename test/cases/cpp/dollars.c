// RUN: %ucc -E %s | grep '^2$'

#define X_$$_Y 2

X_$$_Y
