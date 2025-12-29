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
#define KERNEL_ENTRY(x) x##_0_mix_aiv   // 动态生成函数名 KERNEL_ENTRY(my_kernel) -> my_kernel_0_mix_aiv
#else
#define KERNEL_ENTRY(x) x##_0_mix_aic
#endif

struct Handshake {
    volatile uint32_t aicpu_ready;
    volatile uint32_t aicore_done;
};
/**
 * Minimal kernel entry point
 *
 * This function is called by the runtime when kernel is launched.
 */
// const uint32_t MAX_WAIT = 1000000;
extern "C" __global__ __aicore__ void KERNEL_ENTRY(aicore_kernel)(__gm__ struct Handshake* hank) {

    while (hank->aicpu_ready == 0) {
        dcci(hank, ENTIRE_DATA_CACHE, CACHELINE_OUT);
    }
#ifdef __AIV__
    hank->aicore_done = 1;
#else
    hank->aicore_done = 2;
#endif
}
