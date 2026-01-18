/**
 * AICPU Kernel: transform_kernel
 *
 * Complete example of an AICPU kernel with proper structure.
 *
 * Compile:
 *   aarch64-linux-gnu-g++ -shared -fPIC -O2 -o transform_kernel.so transform_kernel.cpp
 */

#include <cstdint>

extern "C" {

/*
 * Argument structure - must match host exactly
 */
struct TransformArgs {
    void* input;       /* offset 0:  GM pointer */
    void* output;      /* offset 8:  GM pointer */
    int32_t count;     /* offset 16: element count */
    float scale;       /* offset 20: multiply factor */
    float offset;      /* offset 24: add factor */
    int32_t _pad;      /* offset 28: padding */
};

/*
 * Kernel entry point
 *
 * Computes: output[i] = input[i] * scale + offset
 */
void transform_kernel_entry(TransformArgs* args) {
    float* in = reinterpret_cast<float*>(args->input);
    float* out = reinterpret_cast<float*>(args->output);
    const int32_t n = args->count;
    const float scale = args->scale;
    const float offset = args->offset;

    /* Element-wise transform */
    for (int32_t i = 0; i < n; i++) {
        out[i] = in[i] * scale + offset;
    }
}

/*
 * Alternative: Vectorized version (if NEON available)
 *
 * AICPU (ARM Cortex-A55) supports NEON SIMD.
 * For production, consider using intrinsics:
 *
 *   #include <arm_neon.h>
 *
 *   void transform_kernel_neon(TransformArgs* args) {
 *       float* in = (float*)args->input;
 *       float* out = (float*)args->output;
 *       int32_t n = args->count;
 *       float32x4_t vscale = vdupq_n_f32(args->scale);
 *       float32x4_t voffset = vdupq_n_f32(args->offset);
 *
 *       for (int32_t i = 0; i < n; i += 4) {
 *           float32x4_t vin = vld1q_f32(in + i);
 *           float32x4_t vout = vmlaq_f32(voffset, vin, vscale);
 *           vst1q_f32(out + i, vout);
 *       }
 *   }
 */

}  /* extern "C" */
