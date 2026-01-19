/**
 * 04-aicpu-basic - AICPU Kernel Execution
 *
 * This example demonstrates:
 * - Loading .so binaries to device HBM
 * - AICPU backend server pattern (two-phase launch)
 * - Custom argument passing to AICPU kernels
 * - Coordination between host CPU and AICPU cores
 *
 * Hardware concepts taught:
 * - AICPU: ARM Cortex-A55 cores with full C++ support
 * - Backend server pattern: System kernel intermediary layer
 * - HBM access: AICPU directly accesses same memory as AICore
 * - Kernel dispatch: Init phase + execution phase
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <runtime/rt.h>
#include <vector>

/**
 * Device arguments structure
 * Hardware layout: Must match expected offsets for backend server pattern.
 * The system kernel (libaicpu_extend_kernels.so) expects aicpuSoBin/Len
 * at specific offsets. We append customArgsPtr to pass our kernel args
 * to the device through the DeviceArgs indirection.
 */
struct DeviceArgs {
    uint64_t unused[12] = {0};     // Hardware requirement: padding for system kernel
    uint64_t aicpuSoBin{0};        // HBM pointer to .so binary
    uint64_t aicpuSoLen{0};        // Size in bytes
    uint64_t customArgsPtr{0};     // HBM pointer to ScaleArgs (our workaround)
};

/**
 * Kernel arguments structure
 * Hardware dispatch: Contains pointer to DeviceArgs which lives in HBM.
 * The runtime passes this to the system kernel during launch.
 */
struct KernelArgs {
    uint64_t unused[5] = {0};
    int64_t *deviceArgs{nullptr};  // HBM pointer to DeviceArgs

    int InitDeviceArgs(const DeviceArgs &hostDeviceArgs) {
        /* Hardware allocation: Reserve HBM space for DeviceArgs structure.
         * This will be read by AICPU cores during kernel initialization. */
        if (deviceArgs == nullptr) {
            void *deviceArgsDev = nullptr;
            uint64_t deviceArgsSize = sizeof(DeviceArgs);
            int rc = rtMalloc(&deviceArgsDev, deviceArgsSize, RT_MEMORY_HBM, 0);
            if (rc != 0) {
                std::cerr << "rtMalloc for deviceArgs failed: " << rc << '\n';
                return rc;
            }
            deviceArgs = reinterpret_cast<int64_t *>(deviceArgsDev);
        }

        /* Hardware transfer: Copy structure from host DRAM to device HBM
         * via PCIe. AICPU cores will read this directly from HBM. */
        int rc = rtMemcpy(deviceArgs, sizeof(DeviceArgs), &hostDeviceArgs,
                          sizeof(DeviceArgs), RT_MEMCPY_HOST_TO_DEVICE);
        if (rc != 0) {
            std::cerr << "rtMemcpy deviceArgs failed: " << rc << '\n';
            rtFree(deviceArgs);
            deviceArgs = nullptr;
            return rc;
        }
        return 0;
    }

    int FinalizeDeviceArgs() {
        /* Hardware cleanup: Free HBM allocation */
        if (deviceArgs != nullptr) {
            int rc = rtFree(deviceArgs);
            deviceArgs = nullptr;
            return rc;
        }
        return 0;
    }
};

/**
 * Kernel-specific arguments
 * Hardware pointers: input/output must be HBM addresses accessible by AICPU.
 * This structure lives in HBM and is accessed directly by AICPU cores.
 */
struct ScaleArgs {
    void* input;       // HBM pointer (from rtMalloc)
    void* output;      // HBM pointer (from rtMalloc)
    int32_t count;     // Element count
    float scale;       // Scale factor
};

/**
 * AICPU .so loader
 * Hardware loading: Transfers compiled ARM64 .so binary from host filesystem
 * to device HBM where AICPU cores can execute it.
 */
struct AicpuSoInfo {
    uint64_t aicpuSoBin{0};  // HBM address of .so
    uint64_t aicpuSoLen{0};  // Size in bytes

    int Init(const std::string &soPath) {
        // Read .so file from host filesystem
        std::ifstream file(soPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Cannot open " << soPath << '\n';
            return -1;
        }

        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);

        /* Hardware allocation: Reserve HBM for .so binary.
         * AICPU backend server will load this as a shared library. */
        void *dAicpuData = nullptr;
        int rc = rtMalloc(&dAicpuData, fileSize, RT_MEMORY_HBM, 0);
        if (rc != 0) {
            std::cerr << "rtMalloc for .so failed: " << rc << '\n';
            return rc;
        }

        /* Hardware transfer: Copy .so binary to HBM via PCIe */
        rc = rtMemcpy(dAicpuData, fileSize, buffer.data(), fileSize,
                      RT_MEMCPY_HOST_TO_DEVICE);
        if (rc != 0) {
            std::cerr << "rtMemcpy .so failed: " << rc << '\n';
            rtFree(dAicpuData);
            return rc;
        }

        aicpuSoBin = reinterpret_cast<uint64_t>(dAicpuData);
        aicpuSoLen = fileSize;
        return 0;
    }

    int Finalize() {
        /* Hardware cleanup: Free HBM allocation */
        if (aicpuSoBin != 0) {
            int rc = rtFree(reinterpret_cast<void *>(aicpuSoBin));
            aicpuSoBin = 0;
            return rc;
        }
        return 0;
    }
};

/**
 * Launch AICPU kernel using backend server pattern
 * Hardware dispatch: Sends kernel launch command to AICPU task scheduler.
 * The system kernel (AST_DYN_AICPU) acts as intermediary, loading our
 * backend server .so and calling the specified entry point on AICPU cores.
 */
int LaunchAiCpuKernel(rtStream_t stream, KernelArgs *kArgs,
                      const char *kernelName, int aicpuNum) {
    /* Hardware args structure: Pack kernel metadata and arguments.
     * The system kernel expects this exact layout. */
    struct Args {
        KernelArgs kArgs;
        char kernelName[32];                        // Entry function name
        const char soName[32] = {"libaicpu_extend_kernels.so"};  // System kernel
        const char opName[32] = {""};
    } args;

    args.kArgs = *kArgs;
    std::strncpy(args.kernelName, kernelName, sizeof(args.kernelName) - 1);
    args.kernelName[sizeof(args.kernelName) - 1] = '\0';

    /* Hardware launch: Configure extended AICPU launch arguments.
     * Offsets tell runtime where to find kernel/so names in args structure. */
    rtAicpuArgsEx_t rtArgs;
    std::memset(&rtArgs, 0, sizeof(rtArgs));
    rtArgs.args = &args;
    rtArgs.argsSize = sizeof(args);
    rtArgs.kernelNameAddrOffset = offsetof(struct Args, kernelName);
    rtArgs.soNameAddrOffset = offsetof(struct Args, soName);

    /* Hardware dispatch: Submit to AICPU scheduler on specified stream.
     * KERNEL_TYPE_AICPU_KFC = kernel fusion control (backend server pattern).
     * AST_DYN_AICPU = system kernel that loads our backend server. */
    return rtAicpuKernelLaunchExWithArgs(rtKernelType_t::KERNEL_TYPE_AICPU_KFC,
                                          "AST_DYN_AICPU", aicpuNum, &rtArgs,
                                          nullptr, stream, 0);
}

int main() {
    std::cout << "=== AICPU Kernel Example ===\n\n";

    /* Step 1: Set device
     * Hardware initialization: Select NPU device 0 and establish host→device
     * communication. This configures PCIe routes for memory transfers and
     * kernel launches. */
    int rc = rtSetDevice(0);
    if (rc != 0) {
        std::cerr << "rtSetDevice failed: " << rc << '\n';
        return rc;
    }

    /* Step 2: Create stream
     * Hardware queue: Allocate command queue for async AICPU operations.
     * Operations submitted to this stream execute in FIFO order. */
    rtStream_t stream = nullptr;
    rc = rtStreamCreate(&stream, 0);
    if (rc != 0) {
        std::cerr << "rtStreamCreate failed: " << rc << '\n';
        rtDeviceReset(0);
        return rc;
    }

    /* Step 3: Prepare host data
     * Host DRAM: Initialize input array on CPU side before transfer */
    const int N = 1024;
    float* host_input = new float[N];
    float* host_output = new float[N];

    for (int i = 0; i < N; i++) {
        host_input[i] = static_cast<float>(i);
    }

    /* Step 4: Allocate device memory
     * Hardware allocation: Reserve HBM (High Bandwidth Memory) for input/output.
     * AICPU cores access this directly - no separate AICPU memory space. */
    void* dev_input = nullptr;
    void* dev_output = nullptr;

    rc = rtMalloc(&dev_input, N * sizeof(float), RT_MEMORY_HBM, 0);
    if (rc != 0) {
        std::cerr << "rtMalloc input failed: " << rc << '\n';
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    rc = rtMalloc(&dev_output, N * sizeof(float), RT_MEMORY_HBM, 0);
    if (rc != 0) {
        std::cerr << "rtMalloc output failed: " << rc << '\n';
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 5: Copy input to device
     * Hardware transfer: DMA transfer from host DRAM to device HBM via PCIe.
     * Synchronous operation - host waits for completion. */
    rc = rtMemcpy(dev_input, N * sizeof(float), host_input, N * sizeof(float),
                  RT_MEMCPY_HOST_TO_DEVICE);
    if (rc != 0) {
        std::cerr << "rtMemcpy H2D failed: " << rc << '\n';
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 6: Load AICPU kernel .so
     * Hardware loading: Transfer compiled ARM64 AICPU kernel to HBM.
     * AICPU cores will load this as a shared library during kernel init. */
    AicpuSoInfo soInfo{};
    rc = soInfo.Init("./kernel/libscale_aicpu_kernel.so");
    if (rc != 0) {
        std::cerr << "Failed to load kernel .so\n";
        std::cerr << "Build kernel first: cd kernel && mkdir build && cd build && cmake .. && make\n";
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 7: Allocate and prepare kernel arguments
     * Hardware argument passing: Our ScaleArgs must live in HBM for AICPU
     * to access. We pass pointer to it through DeviceArgs.customArgsPtr. */
    ScaleArgs* dev_scale_args = nullptr;
    rc = rtMalloc(reinterpret_cast<void**>(&dev_scale_args), sizeof(ScaleArgs),
                  RT_MEMORY_HBM, 0);
    if (rc != 0) {
        std::cerr << "rtMalloc for ScaleArgs failed: " << rc << '\n';
        soInfo.Finalize();
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    ScaleArgs scale_args;
    scale_args.input = dev_input;
    scale_args.output = dev_output;
    scale_args.count = N;
    scale_args.scale = 2.5f;

    /* Hardware transfer: Copy ScaleArgs to HBM */
    rc = rtMemcpy(dev_scale_args, sizeof(ScaleArgs), &scale_args,
                  sizeof(ScaleArgs), RT_MEMCPY_HOST_TO_DEVICE);
    if (rc != 0) {
        std::cerr << "rtMemcpy ScaleArgs failed: " << rc << '\n';
        rtFree(dev_scale_args);
        soInfo.Finalize();
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 8: Configure DeviceArgs
     * Hardware indirection: DeviceArgs contains .so location and our custom
     * args pointer. System kernel reads this to locate backend server. */
    KernelArgs kernelArgs{};
    DeviceArgs deviceArgs{};
    deviceArgs.aicpuSoBin = soInfo.aicpuSoBin;
    deviceArgs.aicpuSoLen = soInfo.aicpuSoLen;
    deviceArgs.customArgsPtr = reinterpret_cast<uint64_t>(dev_scale_args);

    rc = kernelArgs.InitDeviceArgs(deviceArgs);
    if (rc != 0) {
        std::cerr << "InitDeviceArgs failed: " << rc << '\n';
        rtFree(dev_scale_args);
        soInfo.Finalize();
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 9: Launch init kernel
     * Hardware dispatch phase 1: AICPU executes DynTileFwkKernelServerInit
     * to load backend server .so and extract custom args pointer. This runs on
     * one AICPU core (aicpuNum=1). */
    rc = LaunchAiCpuKernel(stream, &kernelArgs, "DynTileFwkKernelServerInit", 1);
    if (rc != 0) {
        std::cerr << "Launch init kernel failed: " << rc << '\n';
        rtFree(dev_scale_args);
        kernelArgs.FinalizeDeviceArgs();
        soInfo.Finalize();
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 10: Launch main kernel
     * Hardware dispatch phase 2: AICPU executes DynTileFwkKernelServer
     * which performs actual computation. Uses custom args extracted during init.
     * Runs on one AICPU core (aicpuNum=1). */
    rc = LaunchAiCpuKernel(stream, &kernelArgs, "DynTileFwkKernelServer", 1);
    if (rc != 0) {
        std::cerr << "Launch main kernel failed: " << rc << '\n';
        rtFree(dev_scale_args);
        kernelArgs.FinalizeDeviceArgs();
        soInfo.Finalize();
        rtFree(dev_output);
        rtFree(dev_input);
        delete[] host_input;
        delete[] host_output;
        rtStreamDestroy(stream);
        rtDeviceReset(0);
        return rc;
    }

    /* Step 11: Synchronize stream
     * Hardware synchronization: Block host CPU until AICPU completes both
     * kernel launches. All operations in the stream execute in order. */
    rc = rtStreamSynchronize(stream);
    if (rc != 0) {
        std::cerr << "rtStreamSynchronize failed: " << rc << '\n';
    }

    /* Step 12: Copy results back
     * Hardware transfer: DMA from device HBM to host DRAM via PCIe */
    rc = rtMemcpy(host_output, N * sizeof(float), dev_output, N * sizeof(float),
                  RT_MEMCPY_DEVICE_TO_HOST);
    if (rc != 0) {
        std::cerr << "rtMemcpy D2H failed: " << rc << '\n';
    }

    /* Step 13: Verify results */
    int errors = 0;
    for (int i = 0; i < N; i++) {
        float expected = host_input[i] * scale_args.scale;
        if (host_output[i] != expected) {
            if (errors < 3) {
                std::cout << "Mismatch at " << i << ": expected " << expected
                         << ", got " << host_output[i] << '\n';
            }
            errors++;
        }
    }

    if (errors == 0) {
        std::cout << "PASS: All " << N << " elements correct (scale factor: "
                 << scale_args.scale << ")\n";
        std::cout << "  Input[0]  = " << host_input[0]
                 << "  →  Output[0]  = " << host_output[0] << '\n';
        std::cout << "  Input[N-1] = " << host_input[N-1]
                 << "  →  Output[N-1] = " << host_output[N-1] << '\n';
    } else {
        std::cout << "FAIL: " << errors << " errors\n";
    }

    /* Step 14: Cleanup */
    rtFree(dev_scale_args);
    kernelArgs.FinalizeDeviceArgs();
    soInfo.Finalize();
    rtFree(dev_output);
    rtFree(dev_input);
    delete[] host_input;
    delete[] host_output;
    rtStreamDestroy(stream);
    rtDeviceReset(0);

    std::cout << "\n=== End of AICPU Example ===\n";
    return errors > 0 ? 1 : 0;
}
