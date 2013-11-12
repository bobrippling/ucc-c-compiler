// RUN: %layout_check %s -fplan9-extensions
typedef struct {int hi;} Embedded;
typedef struct {Embedded;} Encapsulating;

Encapsulating abc = {.hi = 3};
