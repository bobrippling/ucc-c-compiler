typedef struct {int hi;} Embedded;
typedef struct {Embedded;} Encapsulating;

Encapsulating poo = {.hi = 3;};
