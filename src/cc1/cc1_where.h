#ifndef CC1_WHERE_H
#define CC1_WHERE_H

#include "../util/where.h"

struct where *where_cc1_current(struct where *w);
void where_cc1_adj_identifier(where *w, const char *sp);

int where_in_sysheader(const where *w) ucc_nonnull();

#endif
