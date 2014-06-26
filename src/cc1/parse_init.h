#ifndef PARSE_INIT_H
#define PARSE_INIT_H

#include "decl_init.h"

/* expr or {{...}} */
decl_init *parse_init(symtable *, int static_ctx);

#endif
