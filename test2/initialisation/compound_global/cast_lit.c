// RUN: %ucc -S -o- %s | grep -E '\.long +5'

int i = (int)(int){5};
