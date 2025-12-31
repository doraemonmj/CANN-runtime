/**
 * PTO TADD kernel (single block, vector core path also built).
 * Ported from pto-isa/tests/npu/a2a3/src/st/testcase/tadd/tadd_kernel.cpp
 */

#include <pto/pto-inst.hpp>
#include <pto/common/constants.hpp>

#ifndef __gm__
#define __gm__
#endif

#ifndef __global__
#define __global__
#endif

#ifndef __aicore__
#define __aicore__ [aicore]
#endif

#ifdef __AIV__
#define KERNEL_ENTRY(x) x##_0_mix_aiv
#else
#define KERNEL_ENTRY(x) x##_0_mix_aic
#endif

using namespace pto;

// TADD implementation (float path used by launcher)
template <typename T, int kTRows_, int kTCols_, int vRows, int vCols>
__aicore__ __attribute__((always_inline)) void runTAdd(__gm__ T __out__ *out, __gm__ T __in__ *src0, __gm__ T __in__ *src1)
{
    using DynShapeDim5 = Shape<1, 1, 1, vRows, vCols>;
    using DynStridDim5 = Stride<1, 1, 1, kTCols_, 1>;
    using GlobalData = GlobalTensor<T, DynShapeDim5, DynStridDim5>;
    using TileData = Tile<TileType::Vec, T, kTRows_, kTCols_, BLayout::RowMajor, -1, -1>;

    TileData src0Tile(vRows, vCols);
    TileData src1Tile(vRows, vCols);
    TileData dstTile(vRows, vCols);
    TASSIGN(src0Tile, 0x0);
    TASSIGN(src1Tile, 0x10000);
    TASSIGN(dstTile, 0x20000);

    GlobalData src0Global(src0);
    GlobalData src1Global(src1);
    GlobalData dstGlobal(out);

    TLOAD(src0Tile, src0Global);
    TLOAD(src1Tile, src1Global);
    set_flag(PIPE_MTE2, PIPE_V, EVENT_ID0);
    wait_flag(PIPE_MTE2, PIPE_V, EVENT_ID0);
    TADD(dstTile, src0Tile, src1Tile);
    set_flag(PIPE_V, PIPE_MTE3, EVENT_ID0);
    wait_flag(PIPE_V, PIPE_MTE3, EVENT_ID0);
    TSTORE(dstGlobal, dstTile);
}

// Kernel entry point for AIV (vector core)
extern "C" __global__ __aicore__ void KERNEL_ENTRY(aicore_kernel)(__gm__ float __out__ *out,
                                                                 __gm__ float __in__ *src0,
                                                                 __gm__ float __in__ *src1)
{
#ifdef __AIV__
    if (get_subblockid() == 0) {
        runTAdd<float, 64, 64, 64, 64>(out, src0, src1);
    }
#else
#endif
}
