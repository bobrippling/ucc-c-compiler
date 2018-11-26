// RUN: %ucc -fno-leading-underscore -S -o- %s | if uname|grep Darwin >/dev/null; then grep '\.zerofill .*common'; else grep '\.local common'; fi

static int common;
