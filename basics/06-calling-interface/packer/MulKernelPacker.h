#pragma once

#include "include/KernelLoader.h"
#include <cstdint>
#include <cstring>

/**
 * MulKernelPacker - Type-safe argument packer for mul_kernel
 *
 * This class is a PURE PACKER - it only packs arguments into a buffer.
 * It does NOT load the kernel. Use KernelLoader separately for that.
 *
 * Kernel signature: void kernel(void* args)
 * Arguments layout: int a, float b, float c (output)
 *
 * Usage:
 *   KernelLoader loader("./libmul_kernel.so");
 *   MulKernelPacker packer;
 *   void* buffer = malloc(MulKernelPacker::getArgsBufferSize());
 *   void* ptr = packer.pack(buffer, 30, 3.5f);
 *   loader.kernel()(ptr);
 *   float c;
 *   packer.getResult(buffer, c);  // Extract output parameter
 *   free(buffer);
 */
class MulKernelPacker {
public:
    // Get required buffer size
    static constexpr size_t getArgsBufferSize() {
        return sizeof(int) + 2 * sizeof(float);  // a(int), b(float), c(float)
    }

    // Pack arguments into buffer, returns the buffer pointer for convenience
    void* pack(void* buffer, int a, float b) {
        uint8_t* ptr = static_cast<uint8_t*>(buffer);

        // Pack int a
        std::memcpy(ptr, &a, sizeof(int));
        ptr += sizeof(int);

        // Pack float b
        std::memcpy(ptr, &b, sizeof(float));
        ptr += sizeof(float);

        // Pack float c (output, initialized to 0.0f)
        float c = 0.0f;
        std::memcpy(ptr, &c, sizeof(float));

        return buffer;
    }

    // Extract result from buffer via reference parameter
    // Easy to extend: void getResult(void* buffer, float& c, float& d) { ... }
    void getResult(void* buffer, float& c) const {
        uint8_t* ptr = static_cast<uint8_t*>(buffer);
        ptr += sizeof(int) + sizeof(float);  // Skip to 'c'
        std::memcpy(&c, ptr, sizeof(float));
    }
};
