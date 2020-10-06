
#include <stdlib.h>
#include <stdint.h>

#include "memif_client.h"

///////////////////
// memcpy
//
 // the vector unit memcpy variants confuse coverity
 // so don't let it anywhere near them.
//
/*
#include "types.h"

#ifndef __COVERITY__
#if __AVX512BITALG__
#include "memcpy_avx512.h"
#pragma message "__AVX512BITALG__"
#elif __AVX2__
#include <memcpy_avx2.h>
#pragma message "__AVX2__"
#else
#define clib_memcpy_fast(a,b,c) memcpy(a,b,c)
#pragma message "default"
#endif
#else // __COVERITY__ 
#define clib_memcpy_fast(a,b,c) memcpy(a,b,c)
#endif

int test_memcpy (int a_pck_size)
{
#if __AVX512BITALG__
printf("memcpy __AVX512BITALG__\n");
#elif __AVX2__
printf("memcpy __AVX2__\n");
#elif __SSSE3__
printf("memcpy __SSSE3__\n");
#else
printf("memcpy default\n");
#endif

  char dst_buff[2048];
  char src_buff[2048];
  int len;
  int i;

  int seq_len = 100000000;
  int pck_size = a_pck_size;
  uint8_t ip_addr[4];
  uint8_t hw_daddr[6];

  ip_addr[0] = 10;
  ip_addr[1] = 0;
  ip_addr[2] = 0;
  ip_addr[3] = 1;

  hw_daddr[0] = 1;
  hw_daddr[1] = 1;
  hw_daddr[2] = 1;
  hw_daddr[3] = 1;
  hw_daddr[4] = 1;
  hw_daddr[5] = 1;

// TODO fill pck

  struct timespec start, end;
  memset (&start, 0, sizeof (start));
  memset (&end, 0, sizeof (end));

  timespec_get (&start, TIME_UTC);
  for (i = 0; i < seq_len; i++)
  {
    clib_memcpy_fast(dst_buff, src_buff, len);
  }
  timespec_get (&end, TIME_UTC);
  printf ("\n\n");
  printf ("Pakcet sequence copied!");
  printf ("Seq len: %u", seq_len);
  printf ("Pck size: %u", pck_size);
  uint64_t t1 = end.tv_sec - start.tv_sec;
  uint64_t t2;
  if (end.tv_nsec > start.tv_nsec)
    {
      t2 = end.tv_nsec - start.tv_nsec;
    }
  else
    {
      t2 = start.tv_nsec - end.tv_nsec;
      t1--;
    }
  printf ("Seq time: %lus %luns\n", t1, t2);

  return 0;
}
///////////////////////////
*/

int main ()
{
  Memif_client c;

  c.init();

  while (1)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}
