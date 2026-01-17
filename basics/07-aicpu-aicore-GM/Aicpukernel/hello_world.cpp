#include "device_log.h"
#include <cstdint>
#include <cstdio> 
#include <sched.h>


struct Handshake {
    volatile uint32_t aicpu_ready;
    volatile uint32_t aicore_done;
};

struct KernelArgs {
    uint64_t unused[5] = {0};
    int64_t *deviceArgs{nullptr};
    int64_t *hankArgs{nullptr};
};

int HankAiCore(void *arg) {
    auto kargs = (KernelArgs *)arg;
    Handshake* hank =(Handshake*)kargs->hankArgs;
    hank->aicpu_ready = 1;

    DEV_INFO("AICPU: hank addr = 0x%lx", (uint64_t)hank);

    while (hank->aicore_done == 0) {

    };
    DEV_INFO("success hank->aicore_done = %u", (uint64_t)hank->aicore_done);
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
