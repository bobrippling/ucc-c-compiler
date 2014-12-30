// RUN: %ucc -fno-leading-underscore -S -o- %s | grep '\.local common'

static int common;
