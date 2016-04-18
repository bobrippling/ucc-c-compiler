// RUN: %check --only -e %s -Wno-empty-struct -Wno-anon-untagged-c11-struct

union u {
	union u { // CHECK: error: redefinition of union in scope
	};
};
