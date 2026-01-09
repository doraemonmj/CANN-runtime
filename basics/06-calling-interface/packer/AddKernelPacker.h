#pragma once

#include "include/KernelLoader.h"
#include <cstdint>
#include <cstring>

/**
 * AddKernelPacker - Type-safe argument packer for add_kernel
 *
 * This class is a PURE PACKER - it only packs arguments into a buffer.
 * It does NOT load the kernel. Use KernelLoader separately for that.
 *
 * Kernel signature: void kernel(void* args)
 * Arguments layout: int a, int b, int c (output)
 *
 * Usage:
 *   KernelLoader loader("./libadd_kernel.so");
 *   AddKernelPacker packer;
 *   void* buffer = malloc(AddKernelPacker::getArgsBufferSize());
 *   void* ptr = packer.pack(buffer, 10, 20);
 *   loader.kernel()(ptr);
 *   int c;
 *   packer.getResult(buffer, c);  // Extract output parameter
 *   free(buffer);
 */
class AddKernelPacker {
public:
    // Get required buffer size
    static constexpr size_t getArgsBufferSize() {
        return 3 * sizeof(int);  // a, b, c
    }

    // Pack arguments into buffer, returns the buffer pointer for convenience
    void* pack(void* buffer, int a, int b) {
        int* args = static_cast<int*>(buffer);
        args[0] = a;
        args[1] = b;
        args[2] = 0;  // c initialized to 0 (output)
        return buffer;
    }

    // Extract result from buffer via reference parameter
    // Easy to extend: void getResult(void* buffer, int& c, int& d, int& e) { ... }
    void getResult(void* buffer, int& c) const {
        int* args = static_cast<int*>(buffer);
        c = args[2];
    }
};

