#ifndef OUT_COMMENT
#define OUT_COMMENT

#include "../../util/platform.h"
#include "../../util/compiler.h"

static ucc_inline
const char *out_asm_comment(void)
{
  switch(platform_type()){
    case ARCH_x86_64:
    case ARCH_i386:
      return "#";
    case ARCH_arm:
      return "@";
  }
  ucc_unreach(NULL);
}

#endif
