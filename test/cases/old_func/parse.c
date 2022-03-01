// RUN: %check -e %s -w
datatype( // CHECK: error: function returning a function
    BinaryTree,
    (Leaf, int), // CHECK: error: expecting token identifier, got '('
		// CHECK: ^error: expecting token ')', got '('
		// CHECK: ^^error: expecting token identifier, got int
		// CHECK: ^^^error: expecting token ')', got int
		// CHECK: ^^^^error: expecting token ';', got ')'
    (Node, BinaryTree *, int, BinaryTree *)
);
