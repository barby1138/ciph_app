#ifndef _MEMCPYFAST_H_
#define _MEMCPYFAST_H_

#include "types.h"

#ifndef __COVERITY__
#if __AVX512F__ //__AVX512BITALG__
#include "memcpy_avx512.h"
#pragma message "__AVX512F__"
#elif __AVX2__
#include "memcpy_avx2.h"
#pragma message "__AVX2__"
/*
#elif __SSSE3__
#include "memcpy_sse3.h"
#pragma message "__SSSE3__"
*/
#else
#define clib_memcpy_fast(a,b,c) memcpy(a,b,c)
#pragma message "default"
#endif
#else /* __COVERITY__ */
#define clib_memcpy_fast(a,b,c) memcpy(a,b,c)
#endif

#endif // _MEMCPYFAST_H_
