#define __MATHCALL(function,suffix, args)  \
  __MATHDECL (_Mdouble_,function,suffix, args)
#define __MATHDECL(type, function,suffix, args) \
  __MATHDECL_1(type, function,suffix, args); \
  __MATHDECL_1(type, __CONCAT(__,function),suffix, args)
#define __MATHCALLX(function,suffix, args, attrib)  \
  __MATHDECLX (_Mdouble_,function,suffix, args, attrib)
#define __MATHDECLX(type, function,suffix, args, attrib) \
  __MATHDECL_1(type, function,suffix, args) __attribute__ (attrib); \
  __MATHDECL_1(type, __CONCAT(__,function),suffix, args) __attribute__ (attrib)
#define __MATHDECL_1(type, function,suffix, args) \
  extern type __MATH_PRECNAME(function,suffix) args


#define __CONCAT(a, b) a ## b

#define _Mdouble_     double
#define __MATH_PRECNAME(name,r)  __CONCAT(name,r)

__MATHCALL (acos,, (_Mdouble_ __x));

#undef  _Mdouble_
#undef  __MATH_PRECNAME
