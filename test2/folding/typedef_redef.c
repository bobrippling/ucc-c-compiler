// RUN: %check -e %s

typedef enum abc {
    No,
    One,
    Two
} abc;

typedef enum abc abc; // CHECK: /error: redefinition of typedef from:/
