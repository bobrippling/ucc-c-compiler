// RUN: %check -e %s

short (*p)[*] = (short [2][f()]){ }; // CHECK: error: static-duration variably modified type
