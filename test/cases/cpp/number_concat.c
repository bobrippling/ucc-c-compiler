// RUN: %ucc -E %s | grep '^((!!((0x0U) & 0xF)));$'

#define SQUISH(n) (!!((n) & 0xF))
#define HEX2BIN(n) (SQUISH(n))
#define FLAGS(f) HEX2BIN(0x##f##U)

FLAGS(0);
