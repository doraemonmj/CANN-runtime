/**
 * Host launcher loading kernel bin from file with rtRegisterAllKernel.
 */

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <runtime/rt.h>
#include <string>
#include <vector>

#define DAV_C220_CACHELINE_SIZE 64

struct DataPerCore {
    int8_t coreId;
} __attribute__((aligned(DAV_C220_CACHELINE_SIZE)));

static int ReadFile(const std::string &path, std::vector<uint8_t> &buf) {
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

int main(int argc, char **argv) {
    std::cout << "=== Launching AICORE kernel ===" << '\n';

    int deviceId = 0;
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

    // read aicore kernel from file
    const std::string binPath = "./kernel/kernel.o";
    std::vector<uint8_t> bin;
    if (ReadFile(binPath, bin) != 0) {
        return -1;
    }
    size_t binSize = bin.size();
    const void *binData = bin.data();

    int rc = rtSetDevice(deviceId);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtSetDevice失败: " << rc << '\n';
        return rc;
    }

    rtStream_t stream = nullptr;
    rc = rtStreamCreate(&stream, 0);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtStreamCreate失败: " << rc << '\n';
        return rc;
    }

    // register aicore kernel
    rtDevBinary_t binary;
    std::memset(&binary, 0, sizeof(binary));
    binary.magic = RT_DEV_BINARY_MAGIC_ELF;
    binary.version = 0;
    binary.data = binData;
    binary.length = binSize;
    void *binHandle = nullptr;
    rc = rtRegisterAllKernel(&binary, &binHandle);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtRegisterAllKernel失败: " << rc << '\n';
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    // the number of 1c2v aicore, 4 means 4 cube cores and 8 vec cores
    constexpr uint32_t blocks = 4;
    constexpr uint32_t coresPerBlock = 3;
    constexpr size_t stride = sizeof(DataPerCore);
    void *dOut = nullptr;
    size_t bytes = static_cast<size_t>(blocks) * coresPerBlock * stride;
    rc = rtMalloc(&dOut, bytes, RT_MEMORY_HBM, 0);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMalloc失败: " << rc << '\n';
        dOut = nullptr;
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }
    std::vector<uint8_t> zero(bytes, 255);
    rc = rtMemcpy(dOut, bytes, zero.data(), bytes, RT_MEMCPY_HOST_TO_DEVICE);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMemcpy H2D失败: " << rc << '\n';
        rtFree(dOut);
        dOut = nullptr;
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    struct Args {
        void *dOut;
        int64_t stride;
    };
    Args args = {dOut, stride};
    rtArgsEx_t rtArgs;
    std::memset(&rtArgs, 0, sizeof(rtArgs));
    rtArgs.args = &args;
    rtArgs.argsSize = sizeof(args);

    rtTaskCfgInfo_t cfg = {};
    cfg.schemMode = RT_SCHEM_MODE_BATCH;

    rc = rtKernelLaunchWithHandleV2(binHandle, 0, blocks, &rtArgs, nullptr, stream, &cfg);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtKernelLaunchWithHandleV2失败: " << rc << '\n';
        rtFree(dOut);
        dOut = nullptr;
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    rc = rtStreamSynchronize(stream);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtStreamSynchronize失败: " << rc << '\n';
        rtFree(dOut);
        dOut = nullptr;
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    std::vector<DataPerCore> out(blocks * coresPerBlock);
    rc = rtMemcpy(out.data(), bytes, dOut, bytes, RT_MEMCPY_DEVICE_TO_HOST);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMemcpy D2H失败: " << rc << '\n';
        rtFree(dOut);
        dOut = nullptr;
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    bool ok = true;
    for (uint32_t i = 0; i < blocks * coresPerBlock; ++i) {
        DataPerCore got = out[i];
        if (got.coreId != static_cast<int8_t>(i)) {
            std::cerr << "校验失败 i=" << i << " got=" << static_cast<int>(got.coreId) << " expect=" << i << '\n';
            ok = false;
        }
    }
    std::cout << (ok ? "=== 校验通过 ===" : "=== 校验失败 ===") << '\n';

    rtFree(dOut);
    dOut = nullptr;
    rtStreamDestroy(stream);
    stream = nullptr;
    return ok ? 0 : -1;
}
