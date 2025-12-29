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
template <typename T, int kTRows_, int kTCols_, int vRows, int vCols>//内联
__aicore__ __attribute__((always_inline)) void runTAdd(__gm__ T __out__ *out, __gm__ T __in__ *src0, __gm__ T __in__ *src1)
{
    using DynShapeDim5 = Shape<1, 1, 1, vRows, vCols>; // 定义张量的5维形状
    using DynStridDim5 = Stride<1, 1, 1, kTCols_, 1>; // 定义内存步长
    using GlobalData = GlobalTensor<T, DynShapeDim5, DynStridDim5>; // 定义全局内存中的张量视图，将原始指针包装成一个带形状和步长信息的张量对象
    using TileData = Tile<TileType::Vec, T, kTRows_, kTCols_, BLayout::RowMajor, -1, -1>;//定义向量寄存器文件（VRF）中一块 Tile 的布局和容量

    TileData src0Tile(vRows, vCols);//声明三个逻辑 Tile，指定其有效尺寸
    TileData src1Tile(vRows, vCols);
    TileData dstTile(vRows, vCols);
    TASSIGN(src0Tile, 0x0);//将逻辑 Tile 映射到映射到物理存储，完成硬件资源绑定
    TASSIGN(src1Tile, 0x10000);
    TASSIGN(dstTile, 0x20000);

    GlobalData src0Global(src0);//将原始的全局内存指针（src0, src1, out）包装成带有形状（Shape）和步长（Stride）信息的“张量视图”对象
    GlobalData src1Global(src1);
    GlobalData dstGlobal(out);

    TLOAD(src0Tile, src0Global);//局内存数据，异步加载到 src0Tile 和 src1Tile 所绑定的 向量寄存器文件
    TLOAD(src1Tile, src1Global);
    set_flag(PIPE_MTE2, PIPE_V, EVENT_ID0); //MTE2完成当前任务后，向 PIPE_V（向量计算流水线） 发送一个 事件信号（EVENT_ID0）
    wait_flag(PIPE_MTE2, PIPE_V, EVENT_ID0); //PIPE_V 在执行到此指令时，会 暂停，直到收到 EVENT_ID0 信号
    TADD(dstTile, src0Tile, src1Tile);
    set_flag(PIPE_V, PIPE_MTE3, EVENT_ID0);
    wait_flag(PIPE_V, PIPE_MTE3, EVENT_ID0);
    TSTORE(dstGlobal, dstTile); // 从向量寄存器 → 全局内存
}

// Kernel entry point for AIV (vector core)
extern "C" __global__ __aicore__ void KERNEL_ENTRY(aicore_kernel)(__gm__ float __out__ *out,
                                                                 __gm__ float __in__ *src0,
                                                                 __gm__ float __in__ *src1)
{
#ifdef __AIV__
    if (get_subblockid() == 0) { //只让sub-core 0 执行，避免重复计算
        runTAdd<float, 64, 64, 64, 64>(out, src0, src1);
    }
#else
#endif
}
