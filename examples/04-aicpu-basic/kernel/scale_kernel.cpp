/**
 * AICPU Kernel: scale_kernel (Backend Server Pattern)
 *
 * Hardware execution: Runs on ARM Cortex-A55 cores with full C++ support.
 * AICPU has access to complete libc/libstdc++, can use STL, exceptions,
 * floating-point math, etc. Unlike AICore, AICPU has no specialized compute
 * units - it's a standard ARM CPU.
 *
 * Backend server pattern: System kernel (libaicpu_extend_kernels.so) loads
 * our .so and calls entry points by name. This pattern is required by CANN
 * runtime for AICPU kernel execution.
 */

#include <cstdint>

extern "C" {

/**
 * Kernel argument structure
 * Hardware memory: Resides in HBM, accessible directly by AICPU cores.
 * Must match host-side definition exactly (alignment, sizes, padding).
 */
struct ScaleArgs {
    void* input;      // HBM pointer: input array
    void* output;     // HBM pointer: output array
    int32_t count;    // Number of elements
    float scale;      // Scale factor
};

/**
 * DeviceArgs structure (matches host side)
 * Hardware indirection: System kernel passes this to extract our custom args.
 * We use customArgsPtr to work around limited arg passing in backend pattern.
 */
struct DeviceArgs {
    uint64_t unused[12];
    uint64_t aicpuSoBin;
    uint64_t aicpuSoLen;
    uint64_t customArgsPtr;  // Pointer to our ScaleArgs in HBM
};

/* Global pointer: Stores custom args across init/exec phases */
static ScaleArgs* g_scaleArgs = nullptr;

/**
 * Backend server initialization entry point
 * Hardware execution: Called once during DynTileFwkKernelServerInit launch.
 * Extracts custom args pointer from DeviceArgs for use by main kernel.
 *
 * System kernel interface: The 'arg' parameter contains internal runtime
 * structure. We extract DeviceArgs at known offset (offset +40 bytes).
 */
__attribute__((visibility("default")))
int DynTileFwkBackendKernelServerInit(void *arg) {
    if (arg == nullptr) {
        return -1;
    }

    /* Hardware indirection: System kernel wraps DeviceArgs in internal struct.
     * Extract DeviceArgs pointer at offset +40 from arg base. */
    DeviceArgs* devArgs = reinterpret_cast<DeviceArgs*>(
        *reinterpret_cast<uint64_t**>(reinterpret_cast<char*>(arg) + 40));

    if (devArgs != nullptr && devArgs->customArgsPtr != 0) {
        /* Hardware memory access: Read HBM pointer to our ScaleArgs */
        g_scaleArgs = reinterpret_cast<ScaleArgs*>(devArgs->customArgsPtr);
    }

    return 0;
}

/**
 * Backend server main entry point
 * Hardware execution: Called during DynTileFwkKernelServer launch.
 * Runs on AICPU ARM cores - standard C++ execution with full language support.
 *
 * Performance characteristics:
 * - Memory access: Direct HBM load/store (no L1 buffer like AICore)
 * - Compute: ARM NEON SIMD if vectorized, otherwise scalar
 * - Control flow: Full C++ conditionals, loops, function calls
 */
__attribute__((visibility("default")))
int DynTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr || g_scaleArgs == nullptr) {
        return -1;
    }

    /* Use args pointer set during init phase */
    ScaleArgs* args = g_scaleArgs;

    /* Hardware memory: Cast HBM pointers to typed pointers.
     * AICPU accesses same HBM as AICore - no separate address space. */
    float* in = reinterpret_cast<float*>(args->input);
    float* out = reinterpret_cast<float*>(args->output);
    int32_t n = args->count;
    float scale = args->scale;

    /* Hardware execution: Element-wise scaling on ARM CPU.
     * AICPU has full C/C++ support - can use loops, STL, libm, etc.
     * Compiler may auto-vectorize with NEON instructions if optimization enabled. */
    for (int32_t i = 0; i < n; i++) {
        out[i] = in[i] * scale;
    }

    return 0;
}

/**
 * Static backend server entry point (optional)
 * Placeholder for static kernel pattern (not used in this example)
 */
__attribute__((visibility("default")))
int StaticTileFwkBackendKernelServer(void *arg) {
    if (arg == nullptr) {
        return -1;
    }
    return 0;
}

}  /* extern "C" */
