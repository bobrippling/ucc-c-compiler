__attribute__((__gnu_inline__, __nothrow__, __pure__))
extern __inline__
bool
_BitScanForward(uint32_t* index, uint32_t value)
   {
     bool zf;
     __asm__ (
       "{ bsfl %2, %1 | bsf %1, %2 }"
       : "=@ccz"(zf), "=r"(*index) : "rm"(value)
     );
     return !zf;
   }
