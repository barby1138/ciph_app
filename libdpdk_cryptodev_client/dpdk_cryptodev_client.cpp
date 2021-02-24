
#include <stdio.h>
#include <unistd.h>

#include <rte_malloc.h>
#include <rte_mbuf_pool_ops.h>
#include <rte_random.h>
#include <rte_eal.h>
#include <rte_cryptodev.h>

#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
#include <rte_cryptodev_scheduler.h>
#endif

#include "data_vectors.h"
#include "dpdk_cryptodev_client.h"

#include "memcpy_fast.h"

// use external fridmon logger
#define _EXTERNAL_TRACER_
#ifdef _EXTERNAL_TRACER_
#include <quark/config.h>
#include <meson/tracer.h>
#include <meson/profiler.h>

#undef RTE_LOG
#define TRACE_ERR TRACE_ERROR
#define RTE_LOG(a, b, format, ...) TRACE_##a (format, ##__VA_ARGS__)
#endif
//////////////////////

rte_crypto_cipher_algorithm crypto2rte_cipher_algo_map[CRYPTO_CIPHER_ALGO_LAST];
rte_crypto_cipher_operation crypto2rte_cipher_op_map[CRYPTO_CIPHER_OP_LAST];

uint8_t algo2cdev_id_map[CRYPTO_CIPHER_ALGO_LAST];

//enum { MAX_SESS_NUM = 2 * 2 * 4000 };

const uint32_t BLOCK_LENGTH = 16;

uint8_t plaintext[1024] = {
0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b

};

uint8_t ciphertext[1024] = {
0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0x75, 0x95, 0xb3, 0x48, 0x38, 0xf9, 0xe4, 0x88, 0xec, 0xf8, 0x3b, 0x09, 0x40, 0xd4, 0xd6, 0xea,
0xf1, 0x80, 0x6d, 0xfb, 0xba, 0x9e, 0xee, 0xac, 0x6a, 0xf9, 0x8f, 0xb6, 0xe1, 0xff, 0xea, 0x19,
0x17, 0xc2, 0x77, 0x8d, 0xc2, 0x8d, 0x6c, 0x89, 0xd1, 0x5f, 0xa6, 0xf3, 0x2c, 0xa7, 0x6a, 0x7f,
0x50, 0x1b, 0xc9, 0x4d, 0xb4, 0x36, 0x64, 0x6e, 0xa6, 0xd9, 0x39, 0x8b, 0xcf, 0x8e, 0x0c, 0x55,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b,

0xff, 0xca, 0xfb, 0xf1, 0x38, 0x20, 0x2f, 0x7b, 0x24, 0x98, 0x26, 0x7d, 0x1d, 0x9f, 0xb3, 0x93,
0xd9, 0xef, 0xbd, 0xad, 0x4e, 0x40, 0xbd, 0x60, 0xe9, 0x48, 0x59, 0x90, 0x67, 0xd7, 0x2b, 0x7b,
0x8a, 0xe0, 0x4d, 0xb0, 0x70, 0x38, 0xcc, 0x48, 0x61, 0x7d, 0xee, 0xd6, 0x35, 0x49, 0xae, 0xb4,
0xaf, 0x6b, 0xdd, 0xe6, 0x21, 0xc0, 0x60, 0xce, 0x0a, 0xf4, 0x1c, 0x2e, 0x1c, 0x8d, 0xe8, 0x7b

};

uint8_t iv[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

uint8_t cipher_key[] = {
	0xE4, 0x23, 0x33, 0x8A, 0x35, 0x64, 0x61, 0xE2, 0x49, 0x03, 0xDD, 0xC6, 0xB8, 0xCA, 0x55, 0x7A
};

uint8_t test_buff[1024];

void print_buff1(uint8_t* data, int len)
{
	enum { BUFF_STR_MAX_LEN = 16 * 3 };

	char buff_str[BUFF_STR_MAX_LEN];

	int i = 0;
  	while(i < len)
	{
		int lim = (len - i > 16) ? 16 : len - i;
		
		for(int j = 0; j < lim; ++j)
	  		snprintf(buff_str + j*3, BUFF_STR_MAX_LEN, "%02X%s", data[i + j], (( j + 1 ) % 16 == 0) ? " " : " ");

		RTE_LOG(INFO, USER1, buff_str);

		i += lim;
	}
}

void print_buff_dbg(uint8_t* data, int len)
{
	enum { BUFF_STR_MAX_LEN = 16 * 3 };

	char buff_str[BUFF_STR_MAX_LEN];

	int i = 0;
  	while(i < len)
	{
		int lim = (len - i > 16) ? 16 : len - i;
		
		for(int j = 0; j < lim; ++j)
	  		snprintf(buff_str + j*3, BUFF_STR_MAX_LEN, "%02X%s", data[i + j], (( j + 1 ) % 16 == 0) ? " " : " ");

		RTE_LOG(DEBUG, USER1, buff_str);

		i += lim;
	}

	RTE_LOG(DEBUG, USER1, " ");
}

int Dpdk_cryptodev_client::fill_session_pool_socket(int32_t socket_id, uint32_t session_priv_size, uint32_t nb_sessions)
{
	char mp_name[RTE_MEMPOOL_NAMESIZE];
	rte_mempool *sess_mp;

	if (_priv_mp == NULL) 
	{
		snprintf(mp_name, RTE_MEMPOOL_NAMESIZE, "priv_sess_mp_%u", socket_id);

		sess_mp = rte_mempool_create(mp_name,
					nb_sessions,
					session_priv_size,
					0, 
					0, 
					NULL, 
					NULL, 
					NULL,
					NULL, 
					socket_id,
					0);

		if (sess_mp == NULL) 
		{
			RTE_LOG(ERR, USER1, "Cannot create pool \"%s\" on socket %d", mp_name, socket_id);
			return -ENOMEM;
		}

		RTE_LOG(INFO, USER1, "Allocated pool \"%s\" on socket %d", mp_name, socket_id);

		_priv_mp = sess_mp;
	}

	if (_sess_mp == NULL) 
	{
		snprintf(mp_name, RTE_MEMPOOL_NAMESIZE, "sess_mp_%u", socket_id);

		sess_mp = rte_cryptodev_sym_session_pool_create(mp_name,
					nb_sessions, 0, 0, 0, socket_id);

		if (sess_mp == NULL) 
		{
			RTE_LOG(ERR, USER1, "Cannot create pool \"%s\" on socket %d", mp_name, socket_id);
			return -ENOMEM;
		}

		RTE_LOG(INFO, USER1, "Allocated pool \"%s\" on socket %d", mp_name, socket_id);

		_sess_mp = sess_mp;
	}

	return 0;
}

int Dpdk_cryptodev_client::init_inner()
{
	uint8_t enabled_cdev_count = 0, nb_lcores, cdev_id;
	uint32_t sessions_needed = 0;
	unsigned int i, j;
	int ret;

	printf("init_inner\n");

	uint8_t enabled_cdevs_id[RTE_CRYPTO_MAX_DEVS];
	//enabled_cdev_count = rte_cryptodev_devices_get(_opts.device_type, _enabled_cdevs, RTE_CRYPTO_MAX_DEVS);

	uint8_t cdev_nb = rte_cryptodev_devices_get("crypto_aesni_mb", 
													enabled_cdevs_id, 
													RTE_CRYPTO_MAX_DEVS);
	if (cdev_nb == 0) 
	{
		printf("No crypto devices type crypto_aesni_mb available\n");
		return -EINVAL;
	}
	if (cdev_nb > 1)
	{
		printf("WARN!!! crypto_aesni_mb dev_nb > 1 %d - use last\n", cdev_nb);
	}
	enabled_cdev_count += 1;
	printf("enabled_cdev id: %d\n", enabled_cdevs_id[enabled_cdev_count - 1]);
	_enabled_cdevs[enabled_cdev_count - 1].cdev_id = enabled_cdevs_id[enabled_cdev_count - 1];
	_enabled_cdevs[enabled_cdev_count - 1].algo = crypto2rte_cipher_algo_map[CRYPTO_CIPHER_AES_CBC];
	algo2cdev_id_map[CRYPTO_CIPHER_AES_CBC] = enabled_cdevs_id[enabled_cdev_count - 1];

	cdev_nb = rte_cryptodev_devices_get("crypto_snow3g", 
													enabled_cdevs_id + enabled_cdev_count, 
													RTE_CRYPTO_MAX_DEVS - enabled_cdev_count);
	if (cdev_nb == 0) 
	{
		RTE_LOG(ERR, USER1, "No crypto devices type crypto_snow3g available");
		return -EINVAL;
	}
	if (cdev_nb > 1)
	{
		printf("WARN!!! crypto_snow3g dev_nb > 1 %d - use last\n", cdev_nb);
	}
	enabled_cdev_count += 1;
	printf("enabled_cdev id: %d\n", enabled_cdevs_id[enabled_cdev_count - 1]);
	_enabled_cdevs[enabled_cdev_count - 1].cdev_id = enabled_cdevs_id[enabled_cdev_count - 1];
	_enabled_cdevs[enabled_cdev_count - 1].algo = crypto2rte_cipher_algo_map[CRYPTO_CIPHER_SNOW3G_UEA2];
	algo2cdev_id_map[CRYPTO_CIPHER_SNOW3G_UEA2] = enabled_cdevs_id[enabled_cdev_count - 1];

	nb_lcores = 1;

	// Create a mempool shared by all the devices 
	uint32_t max_sess_size = 0, sess_size;

	for (cdev_id = 0; cdev_id < rte_cryptodev_count(); cdev_id++) 
	{
		sess_size = rte_cryptodev_sym_get_private_session_size(cdev_id);
		if (sess_size > max_sess_size)
			max_sess_size = sess_size;
	}

	// number of queue pairs
	_opts.nb_qps = 1;
	
	for (i = 0; i < enabled_cdev_count && i < RTE_CRYPTO_MAX_DEVS; i++) 
	{
		cdev_id = _enabled_cdevs[i].cdev_id;
#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
		/*
		 * If multi-core scheduler is used, limit the number
		 * of queue pairs to 1, as there is no way to know
		 * how many cores are being used by the PMD, and
		 * how many will be available for the application.
		 */
		if (!strcmp((const char *)_opts.device_type, "crypto_scheduler") &&
				rte_cryptodev_scheduler_mode_get(cdev_id) ==
				CDEV_SCHED_MODE_MULTICORE)
			_opts.nb_qps = 1;
#endif
		rte_cryptodev_info cdev_info;
		uint8_t socket_id = rte_cryptodev_socket_id(cdev_id);

		if (socket_id >= RTE_MAX_NUMA_NODES)
			socket_id = 0;

		rte_cryptodev_info_get(cdev_id, &cdev_info);
		if (_opts.nb_qps > cdev_info.max_nb_queue_pairs) 
		{
			RTE_LOG(ERR, USER1, "Number of needed queue pairs is higher than the maximum number of queue pairs per device.");
			RTE_LOG(ERR, USER1, "Lower the number of cores or increase the number of crypto devices");
			return -EINVAL;
		}
		rte_cryptodev_config conf;
        
		conf.nb_queue_pairs = _opts.nb_qps;
		conf.socket_id = socket_id;
		conf.ff_disable = RTE_CRYPTODEV_FF_SECURITY | RTE_CRYPTODEV_FF_ASYMMETRIC_CRYPTO;

		rte_cryptodev_qp_conf qp_conf;
		qp_conf.nb_descriptors = _opts.nb_descriptors;

		/**
		 * Device info specifies the min headroom and tailroom
		 * requirement for the crypto PMD. This need to be honoured
		 * by the application, while creating mbuf.
		 */
		if (_opts.headroom_sz < cdev_info.min_mbuf_headroom_req) 
		{
			_opts.headroom_sz = cdev_info.min_mbuf_headroom_req;
		}
		if (_opts.tailroom_sz < cdev_info.min_mbuf_tailroom_req) 
		{
			_opts.tailroom_sz = cdev_info.min_mbuf_tailroom_req;
		}

		printf("dev: %d head %d tail %d\n", cdev_id, cdev_info.min_mbuf_headroom_req, cdev_info.min_mbuf_tailroom_req);

		// Update segment size to include headroom & tailroom
		_opts.segment_sz += (_opts.headroom_sz + _opts.tailroom_sz);

		uint32_t dev_max_nb_sess = cdev_info.sym.max_nb_sessions;
		printf("dev_max_nb_sess: %u\n", dev_max_nb_sess);

		sessions_needed = MAX_SESS_NUM;
		// Done only onece - per all devices
		ret = fill_session_pool_socket(socket_id, max_sess_size, sessions_needed);
		if (ret < 0)
			return ret;

		qp_conf.mp_session = _sess_mp;
		qp_conf.mp_session_private = _priv_mp;

		ret = rte_cryptodev_configure(cdev_id, &conf);
		if (ret < 0) 
		{
			RTE_LOG(ERR, USER1, "Failed to configure cryptodev %u", cdev_id);
			return -EINVAL;
		}

		for (j = 0; j < _opts.nb_qps; j++) 
		{
			ret = rte_cryptodev_queue_pair_setup(cdev_id, j, &qp_conf, socket_id);
			if (ret < 0) 
			{
				RTE_LOG(ERR, USER1, "Failed to setup queue pair %u on cryptodev %u", j, cdev_id);
				return -EINVAL;
			}
		}

		if (alloc_common_memory(
			cdev_id, 
			0, //qp_id, 
			0 // extra_priv
			) < 0)
		{
			RTE_LOG(ERR, USER1, "Failed to alloc_common_memory");
			return -EPERM;
		}

		ret = rte_cryptodev_start(cdev_id);
		if (ret < 0) 
		{
			RTE_LOG(ERR, USER1, "Failed to start device %u: error %d", cdev_id, ret);
			return -EPERM;
		}
	}

	return enabled_cdev_count;
}

int Dpdk_cryptodev_client::verify_devices_capabilities(Dpdk_cryptodev_options *opts)
{
	rte_cryptodev_sym_capability_idx cap_idx;
	const rte_cryptodev_symmetric_capability *capability;

	uint8_t i, cdev_id;
	int ret;

	for (i = 0; i < _nb_cryptodevs; i++) 
	{
		cdev_id = _enabled_cdevs[i].cdev_id;

		cap_idx.type = RTE_CRYPTO_SYM_XFORM_CIPHER;
		cap_idx.algo.cipher = opts->cipher_algo;

		capability = rte_cryptodev_sym_capability_get(cdev_id, &cap_idx);
		if (capability == NULL)
			return -1;

		ret = rte_cryptodev_sym_capability_check_cipher(
					capability,
					opts->cipher_key_sz,
					opts->cipher_iv_sz);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int Dpdk_cryptodev_client::init(int argc, char **argv)
{
	uint8_t i;

	_print_dbg = 0;

	int ret;
	uint32_t lcore_id;

	crypto2rte_cipher_algo_map[CRYPTO_CIPHER_AES_CBC] = RTE_CRYPTO_CIPHER_AES_CBC;
	crypto2rte_cipher_algo_map[CRYPTO_CIPHER_SNOW3G_UEA2] = RTE_CRYPTO_CIPHER_SNOW3G_UEA2;

	crypto2rte_cipher_op_map[CRYPTO_CIPHER_OP_ENCRYPT] = RTE_CRYPTO_CIPHER_OP_ENCRYPT;
	crypto2rte_cipher_op_map[CRYPTO_CIPHER_OP_DECRYPT] = RTE_CRYPTO_CIPHER_OP_DECRYPT;

	// Initialise DPDK EAL 
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments!\n");

	argc -= ret;
	argv += ret;

	cperf_options_default(&_opts);

	ret = cperf_options_parse(&_opts, argc, argv);
	if (ret) 
	{
		RTE_LOG(ERR, USER1, "Parsing on or more user options failed");
		goto err;
	}

	ret = cperf_options_check(&_opts);
	if (ret) {
		RTE_LOG(ERR, USER1, "Checking on or more user options failed");
		goto err;
	}

	cperf_options_dump(&_opts);

	_nb_cryptodevs = init_inner();

	if (_nb_cryptodevs < 1) 
	{
		RTE_LOG(ERR, USER1, "Failed to initialise requested crypto device type");
		_nb_cryptodevs = 0;
		goto err;
	}
/*
	ret = verify_devices_capabilities(&_opts);
	if (ret) 
	{
		RTE_LOG(ERR, USER1, "Crypto device type does not support capabilities requested");
		goto err;
	}
*/
/*
    // per connection
	t_vecs = (Crypto_operation *)rte_malloc(NULL, _opts.max_burst_size * sizeof(Crypto_operation), 0);
	if (t_vecs == NULL)
	{
		RTE_LOG(ERR, USER1, "t_vecs alloc failed");
		goto err;
	}
*/
	memset(_active_sessions_registry, 0, sizeof(_active_sessions_registry));

	return 0;

err:
	i = 0;

	for (i = 0; i < _nb_cryptodevs && i < RTE_CRYPTO_MAX_DEVS; i++)
		rte_cryptodev_stop(_enabled_cdevs[i].cdev_id);

	// TODO
	// free connections

	return EXIT_FAILURE;
}

void  Dpdk_cryptodev_client::cleanup()
{
	int i = 0;
	
	for (i = 0; i < _nb_cryptodevs && i < RTE_CRYPTO_MAX_DEVS; i++)
		rte_cryptodev_stop(_enabled_cdevs[i].cdev_id);
	
	// TODO
	// free connections
}

int Dpdk_cryptodev_client::init_conn(int ch_id) 
{
	return 0;
}

int Dpdk_cryptodev_client::cleanup_conn(int ch_id)
{
	remove_all_sessions(ch_id);

	return 0;
}

int Dpdk_cryptodev_client::preprocess_jobs(int ch_id,
											Crypto_operation* jobs, 
											uint32_t size, 
											uint32_t* dev_ops_size_arr, 
											Dev_vecs_idxs_t* dev_vecs_idxs_arr )
{
	static int ccc = 0;

	for (uint32_t i = 0; i < size; ++i)
	{
		if (jobs[i].op.op_status == CRYPTO_OP_STATUS_FAILED)
		{
			RTE_LOG(ERR, USER1, "job %d op_type %d was set FAILED inside the agent", i, jobs[i].op.op_type);
			jobs[i].cipher_buff_list.buff_list_length = 0;

			continue;
		}

		if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CREATE)
		{
			uint32_t sess_id = -1;

			if (0 == create_session(ch_id, jobs[i], &sess_id))
			{
				jobs[i].op.op_status = CRYPTO_OP_STATUS_SUCC;
				jobs[i].op.sess_id = sess_id;
			}
			else
			{
				jobs[i].op.op_status = CRYPTO_OP_STATUS_FAILED;
				jobs[i].op.sess_id = -1;
			}
		}
		else if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CLOSE)
		{
			;
			// postpone remove sess to the end of burst
		}
		else if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING)
		{
			if (NULL == get_session(ch_id, jobs[i]))
			{
				// TODO set failed op
				jobs[i].op.op_status = CRYPTO_OP_STATUS_FAILED;
				jobs[i].cipher_buff_list.buff_list_length = 0;

				RTE_LOG(ERR, USER1, "preprocess_jobs WARN!!! Verify session == NULL - skip op");
			}
			else
			{	
				uint16_t cdev_id = (uint16_t) (jobs[i].op.sess_id >> 16);
				//uint16_t i = (uint16_t) vec.op.sess_id & 0x0000FFFF

				if( cdev_id < _nb_cryptodevs)
				{
					dev_vecs_idxs_arr[cdev_id].dev_vecs_idxs[dev_ops_size_arr[cdev_id]] = i;
					dev_ops_size_arr[cdev_id]++;

					// init with succ
					jobs[i].op.op_status = CRYPTO_OP_STATUS_SUCC;
					jobs[i].cipher_buff_list.buff_list_length = 1;

					if (_print_dbg)
					{
						RTE_LOG(DEBUG, USER1, "sess_id %d ch %d", jobs[i].op.sess_id, ch_id);
						RTE_LOG(DEBUG, USER1, "IN DATA len %d ind %d", jobs[i].cipher_buff_list.buffs[0].length, i);
						print_buff_dbg(jobs[i].cipher_buff_list.buffs[0].data, jobs[i].cipher_buff_list.buffs[0].length);
						RTE_LOG(DEBUG, USER1, "IV %d", jobs[i].cipher_iv.length);
						print_buff_dbg(jobs[i].cipher_iv.data, jobs[i].cipher_iv.length);
					}
/*
					if (jobs[i].op.cipher_op == CRYPTO_CIPHER_OP_ENCRYPT)
						if ( 0 != memcmp((jobs[i].op.cipher_op == CRYPTO_CIPHER_OP_DECRYPT) ? ciphertext : plaintext,
							jobs[i].cipher_buff_list.buffs[0].data,
							jobs[i].cipher_buff_list.buffs[0].length))
						{
    						printf("in BAD data %d %d %d\n", ccc++, jobs[i].cipher_buff_list.buffs[0].length, i);
							print_buff1(jobs[i].cipher_buff_list.buffs[0].data, jobs[i].cipher_buff_list.buffs[0].length);
						}
*/
				}
				else
				{
					jobs[i].op.op_status = CRYPTO_OP_STATUS_FAILED;
					jobs[i].cipher_buff_list.buff_list_length = 0;

					RTE_LOG(ERR, USER1, "preprocess_jobs wrong cdev sess_id %d cdev_id %d", jobs[i].op.sess_id, cdev_id);
				}		
			}
		}
		else
		{
			RTE_LOG(ERR, USER1, "preprocess_jobs WARN!!! unknown op_type %d i %d", jobs[i].op.op_type, i);
		}
	}
}

int Dpdk_cryptodev_client::postprocess_jobs(int ch_id, Crypto_operation* jobs, uint32_t size)
{	
	for (uint32_t i = 0; i < size; ++i)
	{
		if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CREATE)
		{
			;
		}
		else if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CLOSE)
		{
			if (0 == remove_session(ch_id, jobs[i]))
				jobs[i].op.op_status = CRYPTO_OP_STATUS_SUCC;
			else
				jobs[i].op.op_status = CRYPTO_OP_STATUS_FAILED;
		}
		else if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING && 
				jobs[i].op.op_status == CRYPTO_OP_STATUS_SUCC)
		{
			if (_print_dbg)
			{
				RTE_LOG(DEBUG, USER1, "sess_id %d ch %d", jobs[i].op.sess_id, ch_id);
				RTE_LOG(DEBUG, USER1, "OUT DATA len %d ind %d", jobs[i].cipher_buff_list.buffs[0].length, i);
				print_buff_dbg(jobs[i].cipher_buff_list.buffs[0].data, jobs[i].cipher_buff_list.buffs[0].length);
				RTE_LOG(DEBUG, USER1, "IV len %d", jobs[i].cipher_iv.length);
				print_buff_dbg(jobs[i].cipher_iv.data, jobs[i].cipher_iv.length);
			}
			;
		}
		else if (jobs[i].op.op_type == CRYPTO_OP_TYPE_SESS_CIPHERING && 
				jobs[i].op.op_status == CRYPTO_OP_STATUS_FAILED)
		{
			//RTE_LOG(WARNING, USER1, "postprocess_jobs WARN!!! FAILED job op_type %d i %d", jobs[i].op.op_type, i);
			;
		}
		else
		{
			RTE_LOG(WARNING, USER1, "postprocess_jobs WARN!!! unknown op_type %d i %d", jobs[i].op.op_type, i);
		}
	}
}

int Dpdk_cryptodev_client::run_jobs(int ch_id, Crypto_operation* jobs, uint32_t size)
{
	if (size == 0)
	{
		//RTE_LOG(INFO, USER1, "run_jobs size == 0");
		return 0;
	}

	uint8_t cdev_id;
	uint8_t buffer_size_idx = 0;
	uint8_t qp_id = 0; 

	uint32_t dev_ops_size_arr[RTE_CRYPTO_MAX_DEVS] = { 0 };
	Dev_vecs_idxs_t dev_vecs_idxs_arr[RTE_CRYPTO_MAX_DEVS] = { 0 };

	{
  	meson::bench_scope_low scope("preprocess_jobs");

	preprocess_jobs(ch_id, jobs, size, dev_ops_size_arr, dev_vecs_idxs_arr);
	}

	{
  	meson::bench_scope_low scope("run_jobs_inner");

	for (uint8_t cdev_index = 0; cdev_index < _nb_cryptodevs && cdev_index < RTE_CRYPTO_MAX_DEVS; cdev_index++) 
	{
		cdev_id = _enabled_cdevs[cdev_index].cdev_id;

		if (dev_ops_size_arr[cdev_index])
			run_jobs_inner(	ch_id, 
							cdev_id, 
							qp_id, 
							jobs, 
							dev_ops_size_arr[cdev_index], 
							dev_vecs_idxs_arr[cdev_index].dev_vecs_idxs);
	}
	}

	{
  	meson::bench_scope_low scope("postprocess_jobs");

	postprocess_jobs(ch_id, jobs, size);
	}

	return 0;
}

static struct rte_mbuf_ext_shared_info g_shinfo = {};

// ops
int Dpdk_cryptodev_client::set_ops_cipher(	rte_crypto_op **ops, 
											int ch_id, 
											uint8_t dev_id, 
											Crypto_operation* vecs, 
											uint32_t ops_nb, 
											uint32_t* dev_vecs_idxs)
{
	// same as in memif agent
	const uint32_t BUFF_SIZE = 1600; // aligned to BLOCK_LENGTH

	uint8_t cdev_id = 0;
	uint16_t iv_offset = sizeof(rte_crypto_op) + sizeof(rte_crypto_sym_op);

	for (uint32_t i = 0; i < ops_nb; i++) 
	{
		uint32_t j = dev_vecs_idxs[i];

		rte_crypto_sym_op *sym_op = ops[i]->sym;

		ops[i]->status = RTE_CRYPTO_OP_STATUS_NOT_PROCESSED;

		static int count = 0;
		ops[i]->sess_type =  RTE_CRYPTO_OP_WITH_SESSION; 

		rte_cryptodev_sym_session* sess = get_session(ch_id, vecs[j]);
		// [OT] No need handle not found sess == NULL - checked at input
		if(sess == NULL)
		{
			// TODO this should never happen but need to simulate
			RTE_LOG(ERROR, USER1, "set_ops_cipher sess == NULL");

			// set RTE_CRYPTO_OP_STATUS_ERROR ?
			continue;
		}

		rte_crypto_op_attach_sym_session(ops[i], sess);

		sym_op->m_src = (rte_mbuf *)((uint8_t *)ops[i] + _src_buf_offset);

		rte_mbuf *m = sym_op->m_src;

		rte_pktmbuf_attach_extbuf(m,
					  vecs[j].cipher_buff_list.buffs[0].data,
					  rte_mempool_virt2iova(vecs[j].cipher_buff_list.buffs[0].data),
					  BUFF_SIZE,
					  //vecs[j].cipher_buff_list.buffs[0].length,
					  &g_shinfo);
	
		rte_pktmbuf_append(m, vecs[j].cipher_buff_list.buffs[0].length);

		uint32_t data_len = vecs[j].cipher_buff_list.buffs[0].length;
		uint32_t pad_len = 0;
		uint8_t* padding;

        // Following algorithms are block cipher algorithms, and might need padding
		if (_enabled_cdevs[dev_id].algo == RTE_CRYPTO_CIPHER_AES_CBC)
		{
			if (data_len % BLOCK_LENGTH)
                pad_len = BLOCK_LENGTH - (data_len % BLOCK_LENGTH);

        	if (pad_len) 
			{
            	padding = (uint8_t*) rte_pktmbuf_append(m, pad_len);
            	if (NULL == padding)
				{
					// TODO this should never happen but need to simulate
					RTE_LOG(ERROR, USER1, "not enought place for pad");
					
					// set RTE_CRYPTO_OP_STATUS_ERROR ?
					continue;
				}

				vecs[j].op.reserved_1 = pad_len;
				vecs[j].cipher_buff_list.buffs[0].length += pad_len;
            	memset(padding, 0, pad_len);
        	}
		}

		sym_op->cipher.data.length = vecs[j].cipher_buff_list.buffs[0].length;
		sym_op->cipher.data.offset = 0;

		if (_enabled_cdevs[dev_id].algo == RTE_CRYPTO_CIPHER_SNOW3G_UEA2 ||
				_enabled_cdevs[dev_id].algo == RTE_CRYPTO_CIPHER_KASUMI_F8 ||
				_enabled_cdevs[dev_id].algo == RTE_CRYPTO_CIPHER_ZUC_EEA3)
		{
			// in bits
			sym_op->cipher.data.length <<= 3;
			sym_op->cipher.data.offset <<= 3;
		}

		if (vecs[j].cipher_iv.length)
		{
			uint8_t *iv_ptr = rte_crypto_op_ctod_offset(ops[i], uint8_t *, iv_offset);
			clib_memcpy_fast(iv_ptr, vecs[j].cipher_iv.data, vecs[j].cipher_iv.length);
		}
		
	}

	return 0;
}

int Dpdk_cryptodev_client::set_ops_cipher_result(	rte_crypto_op **ops, // processed
													Crypto_operation* vecs, 
													uint32_t ops_nb, // deqd
													uint32_t* dev_vecs_idxs,
													uint32_t ops_deqd_total // deqd before
													)
{
	for (uint32_t i = 0; i < ops_nb; i++) 
	{
		uint32_t j = dev_vecs_idxs[i + ops_deqd_total];

		rte_crypto_sym_op *sym_op = ops[i]->sym;

		if (ops[i]->status != RTE_CRYPTO_OP_STATUS_NOT_PROCESSED)
		{
			if (ops[i]->status != RTE_CRYPTO_OP_STATUS_SUCCESS)
				vecs[j].op.op_status = CRYPTO_OP_STATUS_FAILED;
			else
				vecs[j].op.op_status = CRYPTO_OP_STATUS_SUCC;
		}
		else
		{
			RTE_LOG(ERR, USER1, "set_ops_cipher_result some ops are still RTE_CRYPTO_OP_STATUS_NOT_PROCESSED");
		}
	}
}

int Dpdk_cryptodev_client::run_jobs_inner(
							int ch_id, 
							uint8_t dev_id, 
							uint8_t qp_id, 
							Crypto_operation *vecs, 
							uint32_t dev_vecs_size,
							uint32_t* dev_vecs_idxs)
{
	uint64_t ops_enqd = 0, ops_enqd_total = 0, ops_enqd_failed = 0;
	uint64_t ops_deqd = 0, ops_deqd_total = 0, ops_deqd_failed = 0;
	uint64_t ops_failed = 0;

	static rte_atomic16_t display_once = RTE_ATOMIC16_INIT(0);

	uint64_t i;
	uint16_t ops_unused = 0;

    uint16_t socket_id = 0;

	rte_mbuf *mbufs[_opts.max_burst_size];
	rte_crypto_op *ops[_opts.max_burst_size];
	rte_crypto_op *ops_processed[_opts.max_burst_size];

	uint32_t lcore = rte_lcore_id();

#ifdef CPERF_LINEARIZATION_ENABLE
	rte_cryptodev_info dev_info;
	int linearize = 0;

	/* Check if source mbufs require coalescing */
	if (ctx->options->segment_sz < ctx->options->max_buffer_size) {
		rte_cryptodev_info_get(ctx->dev_id, &dev_info);
		if ((dev_info.feature_flags &
				RTE_CRYPTODEV_FF_MBUF_SCATTER_GATHER) == 0)
			linearize = 1;
	}
#endif /* CPERF_LINEARIZATION_ENABLE */


	// WARN!!! _opts.max_burst_size should be >  max vecs size 
	// or we should shift vecs in set_ops_cipher
	while (ops_enqd_total < dev_vecs_size) 
	{
		uint16_t burst_size = ((ops_enqd_total + _opts.max_burst_size) <= dev_vecs_size) ?
						_opts.max_burst_size :
						dev_vecs_size - ops_enqd_total;
	
		if (0 != ops_enqd_total)
			RTE_LOG(INFO, USER1, "0 != ops_enqd_total %" PRIu64 "", ops_enqd_total);

		uint16_t ops_needed = burst_size - ops_unused;
	
		if (rte_mempool_get_bulk(_ops_mp, (void **)ops, ops_needed) != 0) 
		{
			RTE_LOG(ERR, USER1, 
				"Failed to allocate more crypto operations " 
				"from the crypto operation pool. " 
				"Consider increasing the pool size " 
				"with --pool-sz");
			return -1;
		}
		
		//populate_ops
        set_ops_cipher(ops, ch_id, dev_id, vecs, ops_needed, dev_vecs_idxs);

#ifdef CPERF_LINEARIZATION_ENABLE
		if (linearize) {
			/* PMD doesn't support scatter-gather and source buffer
			 * is segmented.
			 * We need to linearize it before enqueuing.
			 */
			for (i = 0; i < burst_size; i++)
				rte_pktmbuf_linearize(ops[i]->sym->m_src);
		}
#endif /* CPERF_LINEARIZATION_ENABLE */

		/* Enqueue burst of ops on crypto device */
		ops_enqd = rte_cryptodev_enqueue_burst(dev_id, qp_id, ops, ops_needed); //burst_size);
		if (ops_enqd < burst_size)
			ops_enqd_failed++;

		/**
		 * Calculate number of ops not enqueued (mainly for hw
		 * accelerators whose ingress queue can fill up).
		 */
		ops_unused = burst_size - ops_enqd;
		ops_enqd_total += ops_enqd;

//printf("d 1\n");

		/* Dequeue processed burst of ops from crypto device */
		ops_deqd = rte_cryptodev_dequeue_burst(dev_id, qp_id, ops_processed, ops_needed);//ctx->options->max_burst_size);
		if (ops_deqd == 0) 
		{
//printf("d 1.5\n");

			/**
			 * Count dequeue polls which didn't return any
			 * processed operations. This statistic is mainly
			 * relevant to hw accelerators.
			 */
			ops_deqd_failed++;
			//continue;
		}
		else
		{
//printf("d 1.6\n");

			set_ops_cipher_result(ops_processed, vecs, ops_deqd, dev_vecs_idxs, ops_deqd_total);

			/* Free crypto ops so they can be reused. */
			rte_mempool_put_bulk(_ops_mp, (void **)ops_processed, ops_deqd);
		
			ops_deqd_total += ops_deqd;
		}
//printf("d 2\n");

	}

	/* Dequeue any operations still in the crypto device */

	while (ops_deqd_total < dev_vecs_size) 
	{
		/* Sending 0 length burst to flush sw crypto device */
		rte_cryptodev_enqueue_burst(dev_id, qp_id, NULL, 0);

//printf("d 3\n");

		/* dequeue burst */
		ops_deqd = rte_cryptodev_dequeue_burst(dev_id, qp_id, ops_processed, _opts.max_burst_size);
		if (ops_deqd == 0) 
		{
			ops_deqd_failed++;
			continue;
		}
//printf("d 4\n");

		set_ops_cipher_result(ops_processed, vecs, ops_deqd, dev_vecs_idxs, ops_deqd_total);

		/* Free crypto ops so they can be reused. */
		rte_mempool_put_bulk(_ops_mp, (void **)ops_processed, ops_deqd);
		
		ops_deqd_total += ops_deqd;
	}

	return 0;
}

int Dpdk_cryptodev_client::create_session(int ch_id, const Crypto_operation& vec, uint32_t* sess_id)
{
	uint16_t iv_offset = sizeof(rte_crypto_op) + sizeof(rte_crypto_sym_op);

	rte_crypto_sym_xform cipher_xform;
	rte_cryptodev_sym_session *s = NULL;

	if (vec.op.cipher_algo >= CRYPTO_CIPHER_ALGO_LAST)
	{
		RTE_LOG(ERR, USER1, "create_session invalid algo %d", vec.op.cipher_algo);
		return -1;
	}

	s = rte_cryptodev_sym_session_create(_sess_mp);
	if (s == NULL)
	{
		RTE_LOG(ERR, USER1, "create_session rte_cryptodev_sym_session_create FAILED");
		return -1;
	}

	cipher_xform.type = RTE_CRYPTO_SYM_XFORM_CIPHER;
	cipher_xform.next = NULL;

	cipher_xform.cipher.algo = crypto2rte_cipher_algo_map[vec.op.cipher_algo];
	cipher_xform.cipher.op = crypto2rte_cipher_op_map[vec.op.cipher_op];
	cipher_xform.cipher.iv.offset = iv_offset;

	uint8_t cdev_id = algo2cdev_id_map[vec.op.cipher_algo];

	RTE_LOG(INFO, USER1, "create_session ch %d, alg:%d op:%s iv_offset:%d iv len %d key len %d", 
			ch_id,
			cipher_xform.cipher.algo, 
			(cipher_xform.cipher.op == RTE_CRYPTO_CIPHER_OP_ENCRYPT) ? "ENC" : "DEC", 
			cipher_xform.cipher.iv.offset, 
			16,
			vec.cipher_key.length);

	print_buff1(vec.cipher_key.data, vec.cipher_key.length);

	if (cipher_xform.cipher.algo != RTE_CRYPTO_CIPHER_NULL) 
	{
		cipher_xform.cipher.key.data = vec.cipher_key.data;
		cipher_xform.cipher.key.length = vec.cipher_key.length;
		cipher_xform.cipher.iv.length = 16;
	} 
	else 
	{
		cipher_xform.cipher.key.data = NULL;
		cipher_xform.cipher.key.length = 0;
		cipher_xform.cipher.iv.length = 0;
	}

    if (0 == rte_cryptodev_sym_session_init(cdev_id, s, &cipher_xform, _priv_mp))
	{
		for (uint16_t i = 0; i < MAX_SESS_NUM; i++)
		{
			if (_active_sessions_registry[ch_id][cdev_id][i] == NULL)
			{
				// find empty slot / assign id
				_active_sessions_registry[ch_id][cdev_id][i] = s;

				*sess_id = ( 0xFFFF0000 & ( (uint16_t) cdev_id << 16 ) ) | i;

				RTE_LOG(INFO, USER1, "create_session SUCC id %d", *sess_id);

				return 0;
			}
		}
	}
	else
	{
		RTE_LOG(ERR, USER1, "rte_cryptodev_sym_session_init FAILED");
	}

	RTE_LOG(ERR, USER1, "create_session NO FREE sessions");

	// free sess
	if (0 != rte_cryptodev_sym_session_clear(cdev_id, s))
		RTE_LOG(ERR, USER1, "create_session rte_cryptodev_sym_session_clear failed");
	if (0 != rte_cryptodev_sym_session_free(s))
		RTE_LOG(ERR, USER1, "create_session rte_cryptodev_sym_session_free failed");
		
	return -1;
}

int Dpdk_cryptodev_client::remove_session(int ch_id, const Crypto_operation& vec)
{
	uint16_t cdev_id = (uint16_t) (vec.op.sess_id >> 16);
	uint16_t i = (uint16_t) (vec.op.sess_id & 0x0000FFFF);
	if ( !(cdev_id < _nb_cryptodevs && i < MAX_SESS_NUM) )
	{
		RTE_LOG(ERR, USER1, "remove_session ivalid dev or sess id ch_id %d, sess_id 0x%x (%d %d)", ch_id, vec.op.sess_id, cdev_id, i);
		return -1;
	}

	rte_cryptodev_sym_session* s = _active_sessions_registry[ch_id][cdev_id][i];
	if (NULL == s)
	{
		RTE_LOG(ERR, USER1, "remove_session s == NULL ch_id %d, sess_id %d (%d %d)", ch_id, vec.op.sess_id, cdev_id, i);
		return -1;
	}

	if (0 != rte_cryptodev_sym_session_clear(cdev_id, s))
		RTE_LOG(ERR, USER1, "remove_session rte_cryptodev_sym_session_clear failed");
	if (0 != rte_cryptodev_sym_session_free(s))
		RTE_LOG(ERR, USER1, "remove_session rte_cryptodev_sym_session_free failed");
		
	RTE_LOG(INFO, USER1, "remove_session SUCC ch %d, id %d", ch_id, vec.op.sess_id);

	_active_sessions_registry[ch_id][cdev_id][i] = NULL;
	
	return 0;
}

rte_cryptodev_sym_session* Dpdk_cryptodev_client::get_session(int ch_id, const Crypto_operation& vec)
{
	uint16_t cdev_id = (uint16_t) (vec.op.sess_id >> 16);
	uint16_t i = (uint16_t) (vec.op.sess_id & 0x0000FFFF);
	if ( !(cdev_id < _nb_cryptodevs && i < MAX_SESS_NUM) )
	{
		RTE_LOG(ERR, USER1, "get_session ivalid dev or sess id ch_id %d, sess_id 0x%x (%d %d)", ch_id, vec.op.sess_id, cdev_id, i);
		return NULL;
	}
	
	rte_cryptodev_sym_session* s = _active_sessions_registry[ch_id][cdev_id][i];
	if (NULL == s)
	{
		RTE_LOG(ERR, USER1, "get_session s == NULL ch_id %d, sess_id 0x%x (%d %d)", ch_id, vec.op.sess_id, cdev_id, i);
		return NULL;
	}

	return s;
}

int Dpdk_cryptodev_client::remove_all_sessions(int ch_id)
{
	uint32_t cleaned_sess_cnt = 0;

	for (uint16_t cdev_id = 0; cdev_id < _nb_cryptodevs; cdev_id++)
	{
		for (uint16_t i = 0; i < MAX_SESS_NUM; i++)
		{
			rte_cryptodev_sym_session* s = _active_sessions_registry[ch_id][cdev_id][i];
			if (NULL == s)
				continue;

			if (0 != rte_cryptodev_sym_session_clear(cdev_id, s))
				RTE_LOG(ERR, USER1, "remove_all_sessions rte_cryptodev_sym_session_clear failed");
			if (0 != rte_cryptodev_sym_session_free(s))
				RTE_LOG(ERR, USER1, "remove_all_sessions rte_cryptodev_sym_session_free failed");
		
			_active_sessions_registry[ch_id][cdev_id][i] = NULL;

			cleaned_sess_cnt++;
		}
	}

	RTE_LOG(INFO, USER1, "remove_all_sessions cleaned %d", cleaned_sess_cnt);
		
	return 0;
}

// TODO to class
/// memory common
struct obj_params {
	uint32_t src_buf_offset;
	uint32_t dst_buf_offset;
/*
	uint16_t segment_sz;
	uint16_t headroom_sz;
	uint16_t data_len;
	uint16_t segments_nb;
*/
};

/*
void fill_single_seg_mbuf(struct rte_mbuf *m, struct rte_mempool *mp,
		void *obj, uint32_t mbuf_offset, uint16_t segment_sz,
		uint16_t headroom, uint16_t data_len)
{
	uint32_t mbuf_hdr_size = sizeof(struct rte_mbuf);

	// start of buffer is after mbuf structure and priv data 
	m->priv_size = 0;
	m->buf_addr = (char *)m + mbuf_hdr_size;
	m->buf_iova = rte_mempool_virt2iova(obj) + mbuf_offset + mbuf_hdr_size;
	m->buf_len = segment_sz;
	m->data_len = data_len;
	m->pkt_len = data_len;

	printf("fill_single_seg_mbuf %d, %d\n", m, m->data_len);

	// Use headroom specified for the buffer 
	m->data_off = headroom;

	// init some constant fields 
	m->pool = mp;
	m->nb_segs = 1;
	m->port = 0xff;
	rte_mbuf_refcnt_set(m, 1);
	m->next = NULL;
}

void fill_multi_seg_mbuf(struct rte_mbuf *m, struct rte_mempool *mp,
		void *obj, uint32_t mbuf_offset, uint16_t segment_sz,
		uint16_t headroom, uint16_t data_len, uint16_t segments_nb)
{
	uint16_t mbuf_hdr_size = sizeof(struct rte_mbuf);
	uint16_t remaining_segments = segments_nb;
	struct rte_mbuf *next_mbuf;
	rte_iova_t next_seg_phys_addr = rte_mempool_virt2iova(obj) + mbuf_offset + mbuf_hdr_size;

	do {
		// start of buffer is after mbuf structure and priv data 
		m->priv_size = 0;
		m->buf_addr = (char *)m + mbuf_hdr_size;
		m->buf_iova = next_seg_phys_addr;
		next_seg_phys_addr += mbuf_hdr_size + segment_sz;
		m->buf_len = segment_sz;
		m->data_len = data_len;
		printf("fill_multi_seg_mbuf %d, %d\n", m, m->data_len);

		// Use headroom specified for the buffer 
		m->data_off = headroom;

		// init some constant fields 
		m->pool = mp;
		m->nb_segs = segments_nb;
		m->port = 0xff;
		rte_mbuf_refcnt_set(m, 1);
		next_mbuf = (struct rte_mbuf *) ((uint8_t *) m + mbuf_hdr_size + segment_sz);
		m->next = next_mbuf;
		m = next_mbuf;
		remaining_segments--;

	} while (remaining_segments > 0);

	m->next = NULL;
}
*/

void mempool_obj_init(rte_mempool *mp,
		    void *opaque_arg,
		    void *obj,
		    __attribute__((unused)) unsigned int i)
{
	obj_params *params = (obj_params *) opaque_arg;
	rte_crypto_op *op = (rte_crypto_op *) obj;
	rte_mbuf *m = (rte_mbuf *) ((uint8_t *) obj + params->src_buf_offset);
	/* Set crypto operation */
	op->type = RTE_CRYPTO_OP_TYPE_SYMMETRIC;
	op->status = RTE_CRYPTO_OP_STATUS_NOT_PROCESSED;
	op->sess_type =  RTE_CRYPTO_OP_WITH_SESSION;
	op->phys_addr = rte_mem_virt2iova(obj);
	op->mempool = mp;

	/* Set source buffer */
	op->sym->m_src = m;
	/*
	if (params->segments_nb == 1)
		fill_single_seg_mbuf(m, mp, obj, params->src_buf_offset,
				params->segment_sz, params->headroom_sz,
				params->data_len);
	else
		fill_multi_seg_mbuf(m, mp, obj, params->src_buf_offset,
				params->segment_sz, params->headroom_sz,
				params->data_len, params->segments_nb);

	// Set destination buffer 
	if (params->dst_buf_offset) {
		m = (struct rte_mbuf *) ((uint8_t *) obj + params->dst_buf_offset);
		fill_single_seg_mbuf(m, mp, obj, params->dst_buf_offset,
				params->segment_sz, params->headroom_sz,
				params->data_len);
		op->sym->m_dst = m;
	} else
		op->sym->m_dst = NULL;
	*/
}

int Dpdk_cryptodev_client::alloc_common_memory(
			uint8_t dev_id, 
			uint16_t qp_id,
			size_t extra_op_priv_size)
{
	const char *mp_ops_name;
	char pool_name[32] = "";
	int ret;

	// Calculate the object size 
	uint16_t crypto_op_size = sizeof(rte_crypto_op) + sizeof(rte_crypto_sym_op);
	/*
	 * If doing AES-CCM, IV field needs to be 16 bytes long,
	 * and AAD field needs to be long enough to have 18 bytes,
	 * plus the length of the AAD, and all rounded to a
	 * multiple of 16 bytes.
	 */
	uint32_t cipher_iv_length = 16;
	uint16_t crypto_op_private_size = extra_op_priv_size + cipher_iv_length;
//	if (options->aead_algo == RTE_CRYPTO_AEAD_AES_CCM) {
//		crypto_op_private_size = extra_op_priv_size +
//			cipher_iv_length; 
			/*+
			test_vector->auth_iv.length +
			RTE_ALIGN_CEIL(test_vector->aead_iv.length, 16) +
			RTE_ALIGN_CEIL(options->aead_aad_sz + 18, 16);
			*/
//	} else {
//		crypto_op_private_size = extra_op_priv_size +
//			cipher_iv_length;
			/*+
			test_vector->auth_iv.length +
			test_vector->aead_iv.length +
			options->aead_aad_sz;
			*/
//	}

	uint16_t crypto_op_total_size = crypto_op_size + crypto_op_private_size;
	uint16_t crypto_op_total_size_padded = RTE_CACHE_LINE_ROUNDUP(crypto_op_total_size);
/*
	uint32_t mbuf_size = sizeof(rte_mbuf) + _opts.segment_sz;
	uint32_t max_size = _opts.max_buffer_size + _opts.digest_sz;
	uint16_t segments_nb = (max_size % _opts.segment_sz) ?
			(max_size / _opts.segment_sz) + 1 :
			max_size / _opts.segment_sz;
	printf("segments_nb %d\n", segments_nb);
*/
	//uint32_t obj_size = crypto_op_total_size_padded + (mbuf_size * segments_nb);
	// MAX - 25 mbufs
	uint32_t obj_size = crypto_op_total_size_padded + (sizeof(rte_mbuf));

	snprintf(pool_name, sizeof(pool_name), "pool_cdev_%u_qp_%u", dev_id, qp_id);

	_src_buf_offset = crypto_op_total_size_padded;

	obj_params params;
	params.src_buf_offset = crypto_op_total_size_padded;
	params.dst_buf_offset = 0;
/*
	struct obj_params params;
	params.segment_sz = _opts.segment_sz;
	params.headroom_sz = _opts.headroom_sz;
	// Data len = segment size - (headroom + tailroom) 
	params.data_len = _opts.segment_sz - _opts.headroom_sz - _opts.tailroom_sz;
	params.segments_nb = segments_nb;
	params.src_buf_offset = crypto_op_total_size_padded;
	params.dst_buf_offset = 0;
*/
	if (_opts.out_of_place) {
		//_dst_buf_offset = _src_buf_offset + (mbuf_size * segments_nb);
		//params.dst_buf_offset = _dst_buf_offset;

		/* Destination buffer will be one segment only */
		//obj_size += max_size;
	}

	_ops_mp = rte_mempool_create_empty(pool_name,
			_opts.pool_sz, 
            obj_size, 
            512, 
            0,
			rte_socket_id(), 
            0);
	if (_ops_mp == NULL) {
		RTE_LOG(ERR, USER1, "Cannot allocate mempool for device %u", dev_id);
		return -1;
	}
                                                                                                                                                                                                                                                                                                                                                    
	mp_ops_name = rte_mbuf_best_mempool_ops();

	ret = rte_mempool_set_ops_byname(_ops_mp, mp_ops_name, NULL);
	if (ret != 0) {
		RTE_LOG(ERR, USER1, "Error setting mempool handler for device %u", dev_id);
		return -1;
	}

	ret = rte_mempool_populate_default(_ops_mp);
	if (ret < 0) {
		RTE_LOG(ERR, USER1, "Error populating mempool for device %u", dev_id);
		return -1;
	}

	rte_mempool_obj_iter(_ops_mp, mempool_obj_init, (void *)&params);

	return 0;
}

////////////////////////////////////////////
// test

double get_delta_usec(struct timespec start, struct timespec end)
{
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
        
	double tmp = t1 * 1000 * 1000;
    tmp += t2 / 1000.0;

	return tmp;
}

int32_t g_setup_sess_id;

int32_t Dpdk_cryptodev_client::test_create_session(long cid, 
													uint64_t seq, 
													Crypto_cipher_algorithm algo, 
													Crypto_cipher_operation op_type)
{
	int32_t res;

    Crypto_operation op_sess;
    memset(&op_sess, 0, sizeof(Crypto_operation));
	op_sess.op.seq = seq;
	// sess
    op_sess.op.op_type = CRYPTO_OP_TYPE_SESS_CREATE;
    op_sess.op.cipher_algo = algo;
    op_sess.op.cipher_op = op_type;
    op_sess.cipher_key.data = cipher_key;
    op_sess.cipher_key.length = 16;

    run_jobs(0, &op_sess, 1);

	if (op_sess.op.op_status != CRYPTO_OP_STATUS_SUCC)
	{
		TRACE_ERROR ("test_create_session FAILED");
		return -1;
	}

	g_setup_sess_id = op_sess.op.sess_id;
	TRACE_INFO ("test_create_session SUCC %d", g_setup_sess_id);

	return 0;
}

int32_t Dpdk_cryptodev_client::test_cipher(long cid, uint64_t seq, uint64_t sess_id, Crypto_cipher_operation op_type)
{
	int32_t res;

    Crypto_operation op;
    memset(&op, 0, sizeof(Crypto_operation));
	op.op.seq = seq;
    // sess
    op.op.sess_id = sess_id;
    op.op.op_type = CRYPTO_OP_TYPE_SESS_CIPHERING;
	op.op.cipher_op = op_type;
	
	op.op.op_ctx_ptr = NULL;

	uint32_t BUFFER_TOTAL_LEN = 1 + rand() % 300 ; //1 + rand() % 264;
	uint32_t BUFFER_SEGMENT_NUM = 1 ;//1 + rand() % 15;
	BUFFER_SEGMENT_NUM = (BUFFER_SEGMENT_NUM > BUFFER_TOTAL_LEN) ? 
												BUFFER_TOTAL_LEN : 
												BUFFER_SEGMENT_NUM;
	op.cipher_buff_list.buff_list_length = BUFFER_SEGMENT_NUM;

	uint32_t total_len = 0;
	uint32_t step = BUFFER_TOTAL_LEN / BUFFER_SEGMENT_NUM;

	/*
	for (uint32_t i = 0; i < op.cipher_buff_list.buff_list_length; ++i)
	{
    	//op.cipher_buff_list.buffs[i].data = ((op_type == CRYPTO_CIPHER_OP_ENCRYPT) ? plaintext : ciphertext) + i*step;

		op.cipher_buff_list.buffs[i].data = ((op_type == CRYPTO_CIPHER_OP_ENCRYPT) ? plaintext : ciphertext) + i*step;

		op.cipher_buff_list.buffs[i].length = step;
		total_len += step;
		// last segment
		if (i == op.cipher_buff_list.buff_list_length - 1)
			op.cipher_buff_list.buffs[i].length += (BUFFER_TOTAL_LEN - total_len);
	}
	*/

	void * pt = (op_type == CRYPTO_CIPHER_OP_ENCRYPT) ? plaintext : ciphertext;

	memcpy(test_buff, pt, BUFFER_TOTAL_LEN);
	op.cipher_buff_list.buffs[0].data = test_buff;
	op.cipher_buff_list.buffs[0].length = BUFFER_TOTAL_LEN;

	//printf ("BUFFER_TOTAL_LEN %d BUFFER_SEGMENT_NUM %d\n", BUFFER_TOTAL_LEN, BUFFER_SEGMENT_NUM);

    op.cipher_iv.data = iv;
    op.cipher_iv.length = 16;
	// outbuf
	op.op.outbuff_ptr = NULL;
	op.op.outbuff_len = 0;

    run_jobs(0, &op, 1);

	if (op.op.op_status != CRYPTO_OP_STATUS_SUCC)
	{
		TRACE_ERROR ("cipher FAILED");
	}

	return 0;
}

int Dpdk_cryptodev_client::test(Crypto_cipher_algorithm a, Crypto_cipher_operation ot)
{
	uint32_t cc = 0;
	uint8_t res;
	uint64_t seq = 0;
	uint32_t i;
	Crypto_cipher_operation op_type = ot;
	Crypto_cipher_algorithm algo = a;
	struct timespec start;
	struct timespec end;

	uint32_t retries;

  	uint32_t num_pck = 10000000;
  	uint32_t num_pck_per_batch = 32;
  	uint32_t pck_size = 200;

	TRACE_INFO("a: %s ot: %s", (algo == CRYPTO_CIPHER_SNOW3G_UEA2)? "SNOW" : "AES",
								(op_type == CRYPTO_CIPHER_OP_ENCRYPT)? "ENC" : "DEC");
    
	// create 3 sessions
	res = test_create_session(0, seq, algo, op_type);
	if (0 != res) 
		return -1;

	// warmup
    for (i = 0; i < num_pck_per_batch; ++i)
    {
		test_cipher(0, ++seq, g_setup_sess_id, op_type);
    }
    
    timespec_get (&start, TIME_UTC);

    uint32_t num_batch = num_pck / num_pck_per_batch;

	uint32_t c;
    for(c = 0; c < num_batch; ++c)
    {
      	for (i = 0; i < num_pck_per_batch; ++i)
      	{
			test_cipher(0, ++seq, g_setup_sess_id, op_type);
      	}
/*
		cc++;
		// control pps
		if (cc > 1000) 
		{
			cc = 0;
      		timespec_get (&end, TIME_UTC);

      		double tmp = 1000.0 * seq / get_delta_usec(start, end);
	  		printf ("Curr pps %ld %f\n", cid, tmp);
		}
*/
    	//printf ("num %ld %ld ...\n", cid, send_to_usec);
    }

    timespec_get (&end, TIME_UTC);
    TRACE_INFO ("Pakcet sequence finished!");
    TRACE_INFO ("Seq len: %lu", seq + 1);

    double tmp = 1000.0 * seq / get_delta_usec(start, end);
    TRACE_INFO ("Average Kpps: %f", tmp);
    TRACE_INFO ("Average TP: %f Gb/s", (tmp * pck_size) * 8.0 / 1000000.0);
	
	return 0;	
}

/////////////
int Dpdk_cryptodev_client::run_jobs_test(int ch_id, Crypto_operation* jobs, uint32_t size)
{
	struct timespec start;
	struct timespec end;

  	uint32_t num_pck = 10000000;
  	uint32_t num_pck_per_batch = size;
  	uint32_t pck_size = 200;
	
	timespec_get (&start, TIME_UTC);

    uint32_t num_batch = num_pck / num_pck_per_batch;

	//printf ("size %d\n", size);

	uint32_t c;
    for(c = 0; c < num_batch; ++c)
	{
		if (size == 0)	
		{
			//RTE_LOG(INFO, USER1, "run_jobs size == 0");
			return 0;
		}

		uint8_t cdev_id;
		uint8_t buffer_size_idx = 0;
		uint8_t qp_id = 0; 

		uint32_t dev_ops_size_arr[RTE_CRYPTO_MAX_DEVS] = { 0 };
		Dev_vecs_idxs_t dev_vecs_idxs_arr[RTE_CRYPTO_MAX_DEVS] = { 0 };

//printf("1\n");

		preprocess_jobs(ch_id, jobs, size, dev_ops_size_arr, dev_vecs_idxs_arr);

//printf("2\n");

		for (uint8_t cdev_index = 0; cdev_index < _nb_cryptodevs && cdev_index < RTE_CRYPTO_MAX_DEVS; cdev_index++) 
		{
			cdev_id = _enabled_cdevs[cdev_index].cdev_id;

//printf("2.5 %d %d\n", dev_ops_size_arr[cdev_index], size);

			if (dev_ops_size_arr[cdev_index])
				run_jobs_inner(	ch_id, 
							cdev_id, 
							qp_id, 
							jobs, 
							dev_ops_size_arr[cdev_index], 
							dev_vecs_idxs_arr[cdev_index].dev_vecs_idxs);
		}

//printf("3\n");

		postprocess_jobs(ch_id, jobs, size);
	}
//printf("4\n");

    timespec_get (&end, TIME_UTC);
    TRACE_INFO ("Pakcet sequence finished!");
    TRACE_INFO ("Seq len: %lu", num_pck);

    double tmp = 1000.0 * num_pck / get_delta_usec(start, end);
    TRACE_INFO ("Average Kpps: %f", tmp);
    TRACE_INFO ("Average TP: %f Gb/s", (tmp * pck_size) * 8.0 / 1000000.0);

	return 0;
}

// to remove
/*
		uint32_t mbuf_hdr_size = sizeof(rte_mbuf);

		// start of buffer is after mbuf structure and priv data 
		m->priv_size = 0;
		m->buf_addr = vecs[j].cipher_buff_list.buffs[0].data;
		m->buf_iova = RTE_BAD_IOVA; //rte_mempool_virt2iova(vecs[j].cipher_buff_list.buffs[0].data);

		m->buf_len = vecs[j].cipher_buff_list.buffs[0].length;
		m->data_len = vecs[j].cipher_buff_list.buffs[0].length;
		m->pkt_len = vecs[j].cipher_buff_list.buffs[0].length;

		//printf("fill_single_seg_mbuf %d, %d\n", m, m->data_len);

		m->data_off = _opts.headroom_sz;

		// init some constant fields 
		m->pool = _ops_mp;
		m->nb_segs = 1;
		m->port = 0xff;
		rte_mbuf_refcnt_set(m, 1);
		m->next = NULL;
*/
		/*
		do {
			// start of buffer is after mbuf structure and priv data 
			m->priv_size = 0;
			m->buf_addr = (char *)m + mbuf_hdr_size;
			m->buf_iova = next_seg_phys_addr;
			next_seg_phys_addr += mbuf_hdr_size + segment_sz;
			m->buf_len = segment_sz;
			m->data_len = data_len;
			printf("fill_multi_seg_mbuf %d, %d\n", m, m->data_len);

			// Use headroom specified for the buffer 
			m->data_off = headroom;

			// init some constant fields 
			m->pool = mp;
			m->nb_segs = segments_nb;
			m->port = 0xff;
			rte_mbuf_refcnt_set(m, 1);
			next_mbuf = (struct rte_mbuf *) ((uint8_t *) m + mbuf_hdr_size + segment_sz);
			m->next = next_mbuf;
			m = next_mbuf;
			remaining_segments--;
		} while (remaining_segments > 0);
		*/
		//////////////////////////////////////