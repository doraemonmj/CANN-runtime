#include "device_log.h"
#include <cstdint>
#include <cstdio> 
#include <sched.h>
#include <vector>

constexpr int ADDR_MAP_TYPE_REG_AIC_CTRL = 2;
constexpr int ADDR_MAP_TYPE_REG_AIC_PMU_CTRL = 3;

const uint32_t REG_SPR_FAST_PATH_ENABLE = 0x18;
const uint64_t REG_SPR_FAST_PATH_OPEN = 0xE;
const uint64_t REG_SPR_FAST_PATH_CLOSE = 0xF;

const uint32_t REG_SPR_DATA_MAIN_BASE = 0xA0;
const uint32_t REG_SPR_COND = 0x4C8;

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

struct KernelArgs {
    uint64_t unused[5] = {0};
    int64_t *deviceArgs{nullptr};
};

void AicoreToAicpu(int64_t* regAddrs, int numCores) {
    // int success = 0;
    // int count = 0, maxcount = 100000;
    // while(success < 1) {
    for (int i = 0; i < numCores; i++) {
        // uint64_t base = (uint64_t)reg_addrs[i];
        void *base = reinterpret_cast<void *>(regAddrs[i]);
    
        if (base == 0) {
            DEV_ERROR("regAddrs[%d] = nulptr", i);
            break;
        }

        volatile uint64_t* cond_reg = reinterpret_cast<volatile uint64_t *>(static_cast<uint8_t *>(base) + REG_SPR_COND);

        uint64_t val = *cond_reg;

        uint32_t low32 = val & 0xFFFFFFFF;           // 低32位: AICORE_SAY_HELLO (0x80000000)
        uint32_t high32 = (val >> 32) & 0xFFFFFFFF;   // 高32位: blockIdx | core_id
        uint32_t core_id = (val >> 32) & 0xFFFF;       // 位[47:32]: 物理核心ID
        uint32_t block_idx = (val >> 48) & 0xFFFF;       // 位[63:48]: 逻辑块索引
        DEV_INFO("%lx,   %lx,  %d ,  %d", low32, high32, core_id, block_idx);
        if (low32 == AICORE_SAY_HELLO) {
            // DEV_INFO("%lx,   %lx,  %d ,  %d", low32, high32, core_id, block_idx);
            DEV_INFO("%s", "  Status: Valid handshake!");
            // success += 1;
        }

            // count += 1;
            // if (count > maxcount) {
            //     DEV_ERROR("%s", "[timeout] AicoreToAicpu");
            //     break;
            // }
        // }
    }
}


void AicpuToAicore(int64_t* regAddrs, int numCores, int offset, uint32_t val) {
    for (int i = 0; i < numCores; i++) {
        *(reinterpret_cast<volatile uint32_t*>(regAddrs[i] + REG_SPR_FAST_PATH_ENABLE)) = REG_SPR_FAST_PATH_OPEN; // 暂定，开启是为了aicore读取寄存器
        __sync_synchronize();
        void *base = reinterpret_cast<void *>(regAddrs[i]);
        if (base != 0) {
            *(reinterpret_cast<volatile uint32_t *>(regAddrs[i] + offset)) = val; //能写进REG_SPR_DATA_MAIN_BASE寄存器，可以不开启REG_SPR_FAST_PATH_OPEN
            DEV_ERROR("[AICPU->AICORE] write %lx to %lx", val, REG_SPR_FAST_PATH_OPEN);
        }
        __sync_synchronize();
    }
}

// 寄存器是有状态的，上一次运行的结果会留到下一次
int register_handshake(DeviceArgs* devArg) {

    auto regs = (int64_t* )devArg->regs;
    int numCores = devArg->coreNum;
    
    AicpuToAicore(regs, numCores, REG_SPR_DATA_MAIN_BASE, 1234);//REG_SPR_DATA_MAIN_BASE
    devArg->fastPath = true;    //设定已经写入和打开了fast path
    DEV_INFO("first devArg->fastPath = true","%s");
    // 轮询所有寄存器
    AicoreToAicpu(regs, numCores); // REG_SPR_COND是AICPU只读，aicore写

    DEV_INFO("devArg->fastPath = %s", devArg->fastPath ? "true" : "false");
    int count = 0, maxcount = 100000;

    while (devArg->fastPath != false) {
        count += 1;
        if (count > maxcount) {
            break;
        }
    }

    DEV_INFO("devArg->fastPath = %s", devArg->fastPath ? "true" : "false");
    for (int i = 0; i < numCores; i++) {
        *(reinterpret_cast<volatile uint32_t*>(regs[i] + REG_SPR_FAST_PATH_ENABLE)) = REG_SPR_FAST_PATH_CLOSE;
    }
    __sync_synchronize();

    return 0;
}


int HankAiCore(void *arg) {
    auto kargs = (KernelArgs *)arg;
    DeviceArgs* devArg =(DeviceArgs*)kargs->deviceArgs;
    int ret = register_handshake(devArg);
    if (ret != 0) {
        DEV_ERROR("Register handshake failed","%s");
        return -1;
    }
    return 0;
}


extern "C" __attribute__((visibility("default"))) int StaticTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }

    return 0;
}

extern "C" __attribute__((visibility("default"))) int DynTileFwkBackendKernelServerInit(void *arg) {
    InitLogSwitch();
    if (arg == nullptr) {
        DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }

    DEV_INFO("%s", "Hello World Kernel Init: Initializing AICPU kernel");
    return 0;
}

extern "C" __attribute__((visibility("default"))) int DynTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        DEV_ERROR("%s", "Invalid kernel arguments: null pointer");
        return -1;
    }
    DEV_INFO("%s", "Hello World from AICPU Kernel!");

    auto rc = HankAiCore(arg);
    if (rc !=0) {
        return -1;
    }
    DEV_INFO("%s", "Kernel execution completed successfully");
    return 0;
}
