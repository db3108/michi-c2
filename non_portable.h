// non_portable.h -- non portable parts of the code (for efficiency)

/* ===========================================================================
 * If an operaion can be implemented very efficiently on a given architecture
 * it must be placed in this file. A portable version of the function must be
 * provided.
 *
 * At the moment, only a few operations available on the Intel processors are
 * provided. In the future, more operations using SSE intrinsics or the 
 * auto vectorization capabilities of the compilers will be defined.
 * When/if users of other operating systems/compilers want to contribute they
 * can add another case in this file
 *
 * Will certainly need to be refined and improved in the next future
 * ===========================================================================
 */
#ifndef _MSC_VER    // for gcc
    #define __INLINE__ static inline
#else               // for Microsoft Visual Studio
    #define __INLINE__ static __inline
#endif

// Random Number generator (32 bits Linear Congruential Generator)
// Ref: Numerical Recipes in C (W.H. Press & al), 2nd Ed page 284
extern unsigned int idum;
__INLINE__ unsigned int qdrandom(void) {idum=(1664525*idum)+1013904223; return idum;}
#ifndef MICHI_M32
__INLINE__ unsigned int random_int(int n) /* random int between 0 and n-1 */ \
           {unsigned long r=qdrandom(); return (r*n)>>32;}
#else
// The above version seems not to work with Microsoft Studio on 32 bits systems 
__INLINE__ unsigned int random_int(int n) /* random int between 0 and n-1 */
{
    static double i2_32 = 1.0/(((double)(1<<16))*((double)(1<<16)));
    unsigned int r=qdrandom();
    double t = r*i2_32;
    r = t*n;
    return r;
}
#endif

// ------------------------- Portable definitions -----------------------------
// of the efficient operations coded below for some (compiler/processor).
#ifdef PORTABLE
  __INLINE__ int popcnt_u32(unsigned int m32)
  // Compute the number of bits set (i.e. equal to 1) in the 32 bits word m32
  // This is a very naive implementation. Much better solutions exist.
  {
      int sum=0;
      for (int i=0 ; i<32 ; i++)
          if (m32 & (1<<i)) sum++;
      return sum;
  }

  __INLINE__ int bsf_u32(unsigned int m32)
  // Bit Scan Forward -- Search for the least significant bit set in m32
  // If m32 is 0, the result is undefined
  // This is a very naive implementation. Much better solutions exist.
  {
      for (int i=0 ; i<32 ; i++)
          if (m32 & (1<<i)) return i;
      return 32;
  }
#else
  // --------------------- Efficient/Non portable definitions -----------------
  #ifdef __APPLE__
    __INLINE__ int popcnt_u32(unsigned int m32) { return __builtin_popcount(m32);}
    __INLINE__ int bsf_u32(unsigned int m32) { return  __builtin_ctz(m32);}
  #elif __GNUC__
    #include <x86intrin.h>       // for INTEL SSE intrinsics
    __INLINE__ int popcnt_u32(unsigned int m32) { return __builtin_popcount(m32);}
    __INLINE__ int bsf_u32(unsigned int m32) { return  __builtin_ctz(m32);} 
  #elif  xxxxxxx
    // If useful : add here new implementation for another architecture
  #endif // End of case of architectures (compiler/processor)
#endif   // PORTABLE
