// RUN: %ucc -fsyntax-only %s
// RUN: %check -e %s -DERROR


enum E1 : _Bool { X1 = 0 };
enum E2 : _Bool { X2 = 1 };

enum E3 : int { X3 = 0 };
enum E4 : unsigned char { X4 = 255 };

#ifdef ERROR
enum Echar : signed char { X5_1 = -7, X5 = 256 }; // CHECK: /error: underlying type 'signed char' cannot represent all enumerator values/
enum EBool : _Bool { X6_1 = 0, X6_2 = 1, X6_3 = 2 }; // CHECK: /error: underlying type '_Bool' cannot represent all enumerator values/
#endif
