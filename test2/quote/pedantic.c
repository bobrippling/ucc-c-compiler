#define MACRO instance

\"MACRO yo \" MACRO abc // CHECK: /warning: no terminating/
// ^ start, no end

\"MACRO yo "\" MACRO abc // CHECK: /warning: no terminating/
// ^ start,  ^end
//             ^ start, no end

"\"" MACRO // CHECK: !/warning/
//^ start
//   ^ end
//	   ^expanded

'" MACRO'   " MACRO // CHECK: /warning: no terminating/
// ^start     ^end
//              ^expanded

"\\" MACRO // CHECK: !/warning/
//^s ^e ^expanded
