// RUN: ! %ucc %s -fsyntax-only -Wabc -Werror
// RUN: ! %ucc %s -fsyntax-only -Werror -Wabc
// RUN: ! %ucc %s -fsyntax-only -Werror=unknown-warning-option -Wabc -Wabc -Wxyz
// RUN:   %ucc %s -fsyntax-only -Wabc
// RUN:   %ucc %s -fsyntax-only -Wabc -Wabc -Wxyz
