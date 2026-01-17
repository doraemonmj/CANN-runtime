/**
 * Minimal AICore Kernel
 */

#include <cstdint>
#include <array>

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

const uint64_t AICORE_SAY_HELLO = 0x80000000;

struct DeviceArgs {
    uint64_t unused[12] = {0};
    uint64_t aicpuSoBin{0};
    uint64_t aicpuSoLen{0};
    volatile uint64_t regs{0};
    uint64_t devId{0};
    uint64_t coreNum{0};
    volatile bool fastPath{false};
};


extern "C" __global__ __aicore__ void KERNEL_ENTRY(aicore_kernel)(__gm__ uint8_t *args) {
    auto devArgs = (__gm__ DeviceArgs *)args;
    uint64_t count = 0;
    uint64_t maxcount = 1000000;
    while (devArgs->fastPath == false) { // 等待AICPU获取寄存器地址
        dcci(devArgs, ENTIRE_DATA_CACHE, CACHELINE_OUT);
        count += 1;
        if (count > maxcount) {
            break;
        }
    }

    // dcci(devArgs, ENTIRE_DATA_CACHE, CACHELINE_OUT);
    // if (devArgs->fastPath) { //有问题，一遇到就卡死，应该是DATA_MAIN_BASE是专门在ascpp里特定编译的
    //     uint64_t regValue1;
    //     __asm__ volatile("MOV %0, DATA_MAIN_BASE\n" : "=l"(regValue1));
    //     devArgs->devId = regValue1;
    // }
    if (devArgs->fastPath == true) {
        uint64_t regValue1 = get_data_main_base();
        devArgs->devId = regValue1;
    }


#ifdef __AIV__
    blockIdx = get_block_idx() * get_subblockdim() + get_subblockid() + get_block_num();
    int my_core_id = get_coreid(); 
#else
    blockIdx = get_block_idx();
    int my_core_id = get_coreid();  
#endif
    uint64_t handshake_msg = ((uint64_t)blockIdx << 48) | ((uint64_t)my_core_id << 32) | AICORE_SAY_HELLO;  // 魔术字: AICORE_SAY_HELLO
    set_cond(handshake_msg); //往REG_SPR_COND寄存器写
    devArgs->fastPath = false;
}
