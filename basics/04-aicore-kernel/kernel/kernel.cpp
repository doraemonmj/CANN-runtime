/**
 * Minimal AICore Kernel
 */

#include <cstdint>

#ifndef __gm__
#define __gm__
#endif

#ifndef __global__
#define __global__
#endif

#ifndef __aicore__
#define __aicore__ [aicore]
#endif

#ifdef __AIV__
#define KERNEL_ENTRY(x) x##_0_mix_aiv
#else
#define KERNEL_ENTRY(x) x##_0_mix_aic
#endif

#ifdef __AIV__
#define blockIdx blockIdx_aiv
#else
#define blockIdx blockIdx_aic
#endif

[[block_local]] int blockIdx;

/**
 * Minimal kernel entry point
 *
 * This function is called by the runtime when kernel is launched.
 */
extern "C" __global__ __aicore__ void KERNEL_ENTRY(aicore_kernel)(__gm__ uint8_t *Out, int64_t Stride) {
#ifdef __AIV__
    blockIdx = get_block_idx() * get_subblockdim() + get_subblockid() + get_block_num();
#else
    blockIdx = get_block_idx();
#endif
    Out[blockIdx * Stride] = static_cast<uint8_t>(blockIdx);
}