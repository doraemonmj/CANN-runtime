/**
 * AICPU Kernel Launcher Example
 * This program demonstrates how to launch an AICPU kernel using CANN runtime
 * APIs
 */

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <runtime/rt.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include "regs.h"
#include "acl/acl.h"
/**
 * @brief Kernel arguments for the AICPU kernel
 * @details This structure is used to pass arguments to the AICPU kernel.
 * It contains a pointer to the device arguments.
 * The thing important is
 *   1. the offset from KernelArgs to deviceArgs pointer
 *   2. the offset from DeviceArgs to aicpuSoBin
 *   3. the offset from DeviceArgs to aicpuSoLen
 * which are a hardcoded in the AICPU kernel("libaicpu_extend_kernels.so").
 * There are also hardcoded three function names(see hello_world.cpp):
 *   1. StaticTileFwkBackendKernelServer
 *   2. DynTileFwkBackendKernelServerInit
 *   3. DynTileFwkBackendKernelServer
 * which will called when you launch the following kernels of
 * libtilefwk_backend_server.so:
 *   1. StaticTileFwkKernelServer
 *   2. DynTileFwkKernelServerInit
 *   3. DynTileFwkKernelServer
 */

struct DeviceArgs {
    uint64_t unused[12] = {0};
    uint64_t aicpuSoBin{0};
    uint64_t aicpuSoLen{0};
    volatile uint64_t regs{0};
    uint64_t devId{0};
    uint64_t coreNum{0};
    volatile bool fastPath{false};
};

static int ReadFile(const std::string &path, std::vector<uint8_t> &buf) {//以二进制方式完整读取一个文件的内容到buf
    std::ifstream fs(path, std::ios::binary | std::ios::ate);
    if (!fs.is_open()) {
        std::cerr << "无法打开内核文件: " << path << '\n';
        return -1;
    }
    std::streamsize size = fs.tellg();
    fs.seekg(0, std::ios::beg);
    buf.resize(static_cast<size_t>(size));
    if (!fs.read(reinterpret_cast<char *>(buf.data()), size)) {
        std::cerr << "读取内核文件失败: " << path << '\n';
        return -1;
    }
    return 0;
}

struct KernelArgs {
    uint64_t unused[5] = {0};
    int64_t *deviceArgs{nullptr};

    int InitDeviceArgs(const DeviceArgs &hostDeviceArgs) {
        // Allocate device memory for deviceArgs
        if (deviceArgs == nullptr) {
            void *deviceArgsDev = nullptr;
            uint64_t deviceArgsSize = sizeof(DeviceArgs);
            int allocRc = rtMalloc(&deviceArgsDev, deviceArgsSize, RT_MEMORY_HBM, 0);
            if (allocRc != 0) {
                std::cerr << "Error: rtMalloc for deviceArgs failed: " << allocRc << '\n';
                return allocRc;
            }
            deviceArgs = reinterpret_cast<int64_t *>(deviceArgsDev);
        }
        // Copy hostDeviceArgs to device memory via deviceArgs
        int rc =
            rtMemcpy(deviceArgs, sizeof(DeviceArgs), &hostDeviceArgs, sizeof(DeviceArgs), RT_MEMCPY_HOST_TO_DEVICE);
        if (rc != 0) {
            std::cerr << "Error: rtMemcpy failed: " << rc << '\n';
            rtFree(deviceArgs);
            deviceArgs = nullptr;
            return rc;
        }
        return 0;
    }

    int FinalizeDeviceArgs() {
        if (deviceArgs != nullptr) {
            int rc = rtFree(deviceArgs);
            deviceArgs = nullptr;
            return rc;
        }
        return 0;
    }
};

struct AicpuSoInfo {
    uint64_t aicpuSoBin{0};
    uint64_t aicpuSoLen{0};

    int Init(const std::string &soPath) {
        std::ifstream file(soPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open " << soPath << '\n';
            return -1;
        }

        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);

        void *dAicpuData = nullptr;
        // Don't know why use 0 for module id, but it works.
        int rc = rtMalloc(&dAicpuData, fileSize, RT_MEMORY_HBM, 0);
        if (rc != 0) {
            std::cerr << "Error: rtMalloc failed: " << rc << '\n';
            return rc;
        }
        rc = rtMemcpy(dAicpuData, fileSize, buffer.data(), fileSize, RT_MEMCPY_HOST_TO_DEVICE);
        if (rc != 0) {
            std::cerr << "Error: rtMemcpy failed: " << rc << '\n';
            rtFree(dAicpuData);
            dAicpuData = nullptr;
            return rc;
        }

        aicpuSoBin = reinterpret_cast<uint64_t>(dAicpuData);
        aicpuSoLen = fileSize;
        return 0;
    }

    int Finalize() {
        if (aicpuSoBin != 0) {
            int rc = rtFree(reinterpret_cast<void *>(aicpuSoBin));
            aicpuSoBin = 0;
            return rc;
        }
        return 0;
    }
};

class DeviceRunner {
  public:
    static DeviceRunner &Get() {
        static DeviceRunner runner;
        return runner;
    }

    int LaunchAiCpuKernel(rtStream_t stream, KernelArgs *kArgs, const char *kernelName, int aicpuNum) {
        struct Args {
            KernelArgs kArgs;
            char kernelName[32];
            const char soName[32] = {"libaicpu_extend_kernels.so"};
            const char opName[32] = {""};
        } args;

        args.kArgs = *kArgs;
        std::strncpy(args.kernelName, kernelName, sizeof(args.kernelName) - 1);
        args.kernelName[sizeof(args.kernelName) - 1] = '\0';

        rtAicpuArgsEx_t rtArgs;
        std::memset(&rtArgs, 0, sizeof(rtArgs));
        rtArgs.args = &args;
        rtArgs.argsSize = sizeof(args);
        rtArgs.kernelNameAddrOffset = offsetof(struct Args, kernelName);
        rtArgs.soNameAddrOffset = offsetof(struct Args, soName);

        return rtAicpuKernelLaunchExWithArgs(rtKernelType_t::KERNEL_TYPE_AICPU_KFC, "AST_DYN_AICPU", aicpuNum, &rtArgs,
                                             nullptr, stream, 0);
    }

    int LauncherAicoreKernel(rtStream_t stream, KernelArgs *kernelArgs) {
        const std::string binPath = "./Aicorekernel/kernel.o";
        std::vector<uint8_t> bin;
        if (ReadFile(binPath, bin) != 0) {
            return -1;
        }

        size_t binSize = bin.size();
        const void *binData = bin.data();

        rtDevBinary_t binary;
        std::memset(&binary, 0, sizeof(binary));
        binary.magic = RT_DEV_BINARY_MAGIC_ELF;
        binary.version = 0;
        binary.data = binData;
        binary.length = binSize;
        void *binHandle = nullptr;
        int rc = rtRegisterAllKernel(&binary, &binHandle);
        if (rc != RT_ERROR_NONE) {
            std::cerr << "rtRegisterAllKernel失败: " << rc << '\n';
            return rc;
        }

        struct Args {
            void *deviceArgs;
        };
        Args args = {kernelArgs->deviceArgs};
        rtArgsEx_t rtArgs;
        std::memset(&rtArgs, 0, sizeof(rtArgs));
        rtArgs.args = &args;
        rtArgs.argsSize = sizeof(args);

        rtTaskCfgInfo_t cfg = {};
        cfg.schemMode = RT_SCHEM_MODE_BATCH;

        rc = rtKernelLaunchWithHandleV2(binHandle, 0, 1, &rtArgs, nullptr, stream, &cfg);
        if (rc != RT_ERROR_NONE) {
            std::cerr << "rtKernelLaunchWithHandleV2失败: " << rc << '\n';
            return rc;
        }

        return rc;
    }

    int Run(rtStream_t streamAicpu, rtStream_t streamAicore, KernelArgs *kernelArgs, int launchAicpuNum = 5) {
        if (kernelArgs == nullptr) {
            std::cerr << "Error: kernelArgs is null" << '\n';
            return -1;
        }
        // auto rc = LauncherAicoreKernel(streamAicore, kernelArgs);
        // Launch init which save the Aicpu So to device and bind the function names
        auto rc = LaunchAiCpuKernel(streamAicpu, kernelArgs, "DynTileFwkKernelServerInit", 1);
        if (rc != 0) {
            std::cerr << "Error: LaunchAiCpuKernel (init) failed: " << rc << '\n';
            return rc;
        }

        // // Launch main kernel
        rc = LaunchAiCpuKernel(streamAicpu, kernelArgs, "DynTileFwkKernelServer", launchAicpuNum);
        if (rc != 0) {
            std::cerr << "Error: LaunchAiCpuKernel (main) failed: " << rc << '\n';
            return rc;
        }

        rc = LauncherAicoreKernel(streamAicore, kernelArgs);
        if (rc != 0) {
            std::cerr << "Error: LaunchAiCcoreKernel failed: " << rc << '\n';
            return rc;
        }

        return rc;
    }

  private:
    DeviceRunner() {}
};

void CopyToHost(KernelArgs *kernelArgs) {
    auto deviceArgs = kernelArgs->deviceArgs;

    std::vector<DeviceArgs> output(1);
    int rc = rtMemcpy(output.data(), sizeof(DeviceArgs), deviceArgs, sizeof(DeviceArgs), RT_MEMCPY_DEVICE_TO_HOST);
    if (rc !=0) {
        std::cout << " fail" << std::endl;
    }

    std::cout << "jieguo: " << output[0].devId << "  " <<output[0].fastPath << std::endl;
}

void InitDeviceRegsAddr(DeviceArgs& arg) {
    std::vector<int64_t> host_regs;
    GetAicoreRegs(host_regs, arg.devId);

    size_t size = host_regs.size() * sizeof(uint64_t);
    void* reg_ptr = nullptr;

    // 1. 分配 Device 内存
    int rc = rtMalloc(&reg_ptr, size, RT_MEMORY_HBM, 0);
    if (rc != RT_ERROR_NONE) {
        // 处理 rtMalloc 失败
        return;
    }

    // 2. 拷贝 Host 地址值（注意：这些值在 Device 上不可用作指针！）
    rc = rtMemcpy(reg_ptr, size, host_regs.data(), size, RT_MEMCPY_HOST_TO_DEVICE);
    if (rc != RT_ERROR_NONE) {
        rtFree(reg_ptr);
        return;
    }

    // 3. 保存 Device 内存地址（不是寄存器地址！）
    arg.regs = reinterpret_cast<uint64_t>(reg_ptr);
    arg.coreNum = host_regs.size();

}
// Example usage
int main(int argc, char **argv) {
    std::cout << "=== Launching Empty AICPU Kernel ===" << '\n';
    // Parse device id from main's argument (expected range: 0-15)
    int deviceId = 9;

    if (argc > 1) {
        try {
            deviceId = std::stoi(argv[1]);
            if (deviceId < 0 || deviceId > 15) {
                std::cerr << "Error: deviceId (" << deviceId << ") out of range [0, 15]" << '\n';
                return -1;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error: invalid deviceId argument: " << argv[1] << '\n';
            return -1;
        }
    }

    int devRc = rtSetDevice(deviceId);

    if (devRc != 0) {
        std::cerr << "Error: rtSetDevice(" << deviceId << ") failed: " << devRc << '\n';
        return devRc;
    }
    rtStream_t streamAicpu = nullptr;
    rtStream_t streamAicore = nullptr;
    int rc = rtStreamCreate(&streamAicpu, 0);
    if (rc != 0) {
        std::cerr << "Error: rtStreamCreate failed: " << rc << '\n';
        return rc;
    }

    rc = rtStreamCreate(&streamAicore, 0);
    if (rc != 0) {
        std::cerr << "Error: rtStreamCreate failed: " << rc << '\n';
        return rc;
    }
    std::string soPath = "./Aicpukernel/libtilefwk_backend_server.so";
    AicpuSoInfo soInfo{};
    rc = soInfo.Init(soPath);
    if (rc != 0) {
        std::cerr << "Error: AicpuSoInfo::Init failed: " << rc << '\n';
        rtStreamDestroy(streamAicpu);
        rtStreamDestroy(streamAicore);
        streamAicpu = nullptr;
        streamAicore = nullptr;
        return rc;
    }
    KernelArgs kernelArgs{};
    DeviceArgs deviceArgs{};
    deviceArgs.aicpuSoBin = soInfo.aicpuSoBin;
    deviceArgs.aicpuSoLen = soInfo.aicpuSoLen;
    deviceArgs.devId = deviceId;
    InitDeviceRegsAddr(deviceArgs);

    rc = kernelArgs.InitDeviceArgs(deviceArgs);
    if (rc != 0) {
        std::cerr << "Error: KernelArgs::InitDeviceArgs failed: " << rc << '\n';
        soInfo.Finalize();
        rtStreamDestroy(streamAicpu);
        rtStreamDestroy(streamAicore);
        streamAicpu = nullptr;
        streamAicore = nullptr;
        return rc;
    }

    DeviceRunner &runner = DeviceRunner::Get();
    int launchAicpuNum = 1;
    rc = runner.Run(streamAicpu, streamAicore, &kernelArgs, launchAicpuNum);

    rc = rtStreamSynchronize(streamAicpu);
    if (rc != 0) {
        std::cerr << "Error: rtStreamSynchronize failed: " << rc << '\n';
        return rc;
    }

    rc = rtStreamSynchronize(streamAicore);
    if (rc != 0) {
        std::cerr << "Error: rtStreamSynchronize failed: " << rc << '\n';
        return rc;
    }

    CopyToHost(&kernelArgs);

    kernelArgs.FinalizeDeviceArgs();
    soInfo.Finalize();
    rtStreamDestroy(streamAicpu);
    rtStreamDestroy(streamAicore);
    streamAicpu = nullptr;
    streamAicore = nullptr;

    if (rc != 0) {
        std::cerr << "=== Launch Failed ===" << '\n';
    } else {
        std::cout << "=== Launch Success ===" << '\n';
    }

    return rc;
}
