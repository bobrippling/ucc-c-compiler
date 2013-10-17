// fix: -fplan9-extensions
// RUN: %layout_check %s
typedef struct {int hi;} Embedded;
typedef struct {Embedded;} Encapsulating;

Encapsulating abc = {.hi = 3};
