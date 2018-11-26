// RUN: %layout_check --sections %s -fcommon

typedef unsigned long size_t;

const struct {const int b[1]; } ro[] = {{{}}};
struct {const int b[1]; } rw[] = {{{}}};

const struct {const int b[1]; } ro_common[1];
struct {const int b[1]; } rw_common[1];
