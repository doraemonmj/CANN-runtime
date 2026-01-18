/**
 * AICPU Kernel: scale_kernel
 *
 * This is an actual AICPU kernel that runs on ARM Cortex-A55 cores.
 * It must be compiled as a shared library (.so) for aarch64.
 *
 * Compile:
 *   aarch64-linux-gnu-g++ -shared -fPIC -O2 -o scale_kernel.so scale_kernel.cpp
 *
 * Or native compile on Ascend device:
 *   g++ -shared -fPIC -O2 -o scale_kernel.so scale_kernel.cpp
 */

#include <cstdint>

extern "C" {

/**
 * Kernel argument structure
 *
 * IMPORTANT: This must match the host-side definition exactly!
 * - Same field order
 * - Same field types (use fixed-width types)
 * - Same alignment/padding
 */
struct ScaleArgs {
    void* input;      /* GM pointer: input array */
    void* output;     /* GM pointer: output array */
    int32_t count;    /* Number of elements to process */
    float scale;      /* Scale factor */
};

/**
 * Kernel entry point
 *
 * The name "scale_kernel_entry" is passed to platform_aicpu_launch().
 * Must use extern "C" to prevent C++ name mangling.
 *
 * @param args Pointer to ScaleArgs structure in device memory
 */
void scale_kernel_entry(ScaleArgs* args) {
    /* Cast GM pointers to typed pointers */
    float* in = reinterpret_cast<float*>(args->input);
    float* out = reinterpret_cast<float*>(args->output);
    int32_t n = args->count;
    float scale = args->scale;

    /*
     * Simple element-wise operation
     *
     * AICPU has full C/C++ support, so we can use:
     * - Standard loops
     * - Conditionals
     * - Math functions (libm)
     * - Dynamic memory (though GM is preferred)
     * - printf for debugging
     */
    for (int32_t i = 0; i < n; i++) {
        out[i] = in[i] * scale;
    }

    /*
     * Notes:
     * 1. in/out point to HBM (Global Memory)
     * 2. AICPU has direct access to HBM
     * 3. Multiple AICPU cores can run different kernels concurrently
     * 4. No need for explicit memory barriers within single kernel
     */
}

/**
 * Alternative kernel: scale with offset
 *
 * Demonstrates multiple entry points in one .so
 */
struct ScaleOffsetArgs {
    void* input;
    void* output;
    int32_t count;
    float scale;
    float offset;
    int32_t _pad;  /* Padding for 8-byte alignment */
};

void scale_offset_entry(ScaleOffsetArgs* args) {
    float* in = reinterpret_cast<float*>(args->input);
    float* out = reinterpret_cast<float*>(args->output);

    for (int32_t i = 0; i < args->count; i++) {
        out[i] = in[i] * args->scale + args->offset;
    }
}

}  /* extern "C" */
