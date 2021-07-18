// RUN: %ucc %s -fsyntax-only -std=c99 -DEXPECTED_FEAT_ALIGNAS=0
// RUN: %ucc %s -fsyntax-only -std=c11 -DEXPECTED_FEAT_ALIGNAS=1

_Static_assert(__has_feature(c_alignas) == EXPECTED_FEAT_ALIGNAS, "should have c_alignas in C11 mode");
_Static_assert(__has_extension(c_alignas), "should always have c_alignas extension");
