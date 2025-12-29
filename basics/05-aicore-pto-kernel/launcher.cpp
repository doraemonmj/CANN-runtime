/**
 * Host launcher loading kernel bin from file with rtRegisterAllKernel.
 * Reference: 04-aicore-kernel/launcher.cpp
 */

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <runtime/rt.h>
#include <string>
#include <vector>

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
    std::cout << "=== Launching AICORE PTO kernel ===" << '\n';

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

    // Read aicore kernel from file
    const std::string binPath = "./kernel/pto_kernel.o";
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

    // Register aicore kernel
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

    // Run a single vector core (1 block) on a 64x64 float tile
    constexpr uint32_t blocks = 1;
    constexpr size_t elems = 64 * 64;
    const size_t bytes = elems * sizeof(float);

    void *dOut = nullptr;
    void *dSrc0 = nullptr;
    void *dSrc1 = nullptr;

    rc = rtMalloc(&dOut, bytes, RT_MEMORY_HBM, 0);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMalloc dOut失败: " << rc << '\n';
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }
    rc = rtMalloc(&dSrc0, bytes, RT_MEMORY_HBM, 0);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMalloc dSrc0失败: " << rc << '\n';
        rtFree(dOut);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }
    rc = rtMalloc(&dSrc1, bytes, RT_MEMORY_HBM, 0);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMalloc dSrc1失败: " << rc << '\n';
        rtFree(dOut);
        rtFree(dSrc0);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    std::vector<float> hSrc0(elems);
    std::vector<float> hSrc1(elems);
    for (size_t i = 0; i < elems; ++i) {
        hSrc0[i] = static_cast<float>(i);
        hSrc1[i] = static_cast<float>(i * 2);
    }

    rc = rtMemcpy(dSrc0, bytes, hSrc0.data(), bytes, RT_MEMCPY_HOST_TO_DEVICE);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMemcpy src0 H2D失败: " << rc << '\n';
        rtFree(dOut);
        rtFree(dSrc0);
        rtFree(dSrc1);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }
    rc = rtMemcpy(dSrc1, bytes, hSrc1.data(), bytes, RT_MEMCPY_HOST_TO_DEVICE);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMemcpy src1 H2D失败: " << rc << '\n';
        rtFree(dOut);
        rtFree(dSrc0);
        rtFree(dSrc1);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    struct Args {
        void *dOut;
        void *dSrc0;
        void *dSrc1;
    };
    Args args = {dOut, dSrc0, dSrc1};
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
        rtFree(dSrc0);
        rtFree(dSrc1);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    rc = rtStreamSynchronize(stream);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtStreamSynchronize失败: " << rc << '\n';
        rtFree(dOut);
        rtFree(dSrc0);
        rtFree(dSrc1);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    std::vector<float> out(elems, -1.0f);
    rc = rtMemcpy(out.data(), bytes, dOut, bytes, RT_MEMCPY_DEVICE_TO_HOST);
    if (rc != RT_ERROR_NONE) {
        std::cerr << "rtMemcpy D2H失败: " << rc << '\n';
        rtFree(dOut);
        rtFree(dSrc0);
        rtFree(dSrc1);
        rtStreamDestroy(stream);
        stream = nullptr;
        return rc;
    }

    bool ok = true;
    constexpr float eps = 1e-6f;
    constexpr int PRINT_COUNT = 10;
    int printCount = 0;
    for (size_t i = 0; i < PRINT_COUNT; ++i) {
        float expect = hSrc0[i] + hSrc1[i];
        if (std::abs(out[i] - expect) > eps) {
            std::cerr << "FAIL: i=" << i << ", src0=" << hSrc0[i] << ", src1=" << hSrc1[i] << ", got=" << out[i] << ", expect=" << expect << '\n';
            ok = false;
        } else if (printCount < PRINT_COUNT) {
            std::cout << "PASS: i=" << i << ", src0=" << hSrc0[i] << ", src1=" << hSrc1[i] << ", got=" << out[i] << ", expect=" << expect << '\n';
            printCount++;
        }
    }
    std::cout << (ok ? "=== 校验通过 ===" : "=== 校验失败 ===") << '\n';

    rtFree(dOut);
    rtFree(dSrc0);
    rtFree(dSrc1);
    rtStreamDestroy(stream);
    stream = nullptr;
    return ok ? 0 : -1;
}
