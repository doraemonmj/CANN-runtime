/**
 * 17-aicore-pmu - Performance Monitoring Unit
 *
 * This example demonstrates:
 * - PMU counters for performance measurement
 * - Cycle counting and timing
 * - Memory bandwidth measurement
 * - Compute utilization metrics
 * - Roofline model analysis
 *
 * PMU allows you to measure actual hardware performance to
 * identify bottlenecks and optimize kernels.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include "platform.h"

int main() {
    printf("=== AICORE Performance Monitoring Unit (PMU) ===\n\n");

    int ret = platform_init(0);
    if (ret != PLATFORM_SUCCESS) {
        printf("Platform init failed\n");
        return 1;
    }

    /* ====== PMU Overview ====== */
    printf("PMU Overview:\n");
    printf("  +-------------------------------------------------------+\n");
    printf("  |  Performance Monitoring Unit (PMU)                    |\n");
    printf("  |                                                       |\n");
    printf("  |  Hardware counters that measure:                      |\n");
    printf("  |  - Cycles (time spent)                                |\n");
    printf("  |  - Instructions executed                              |\n");
    printf("  |  - Memory operations (loads/stores)                   |\n");
    printf("  |  - Cache hits/misses                                  |\n");
    printf("  |  - Cube/Vector unit utilization                       |\n");
    printf("  |                                                       |\n");
    printf("  |  Use PMU to identify bottlenecks!                     |\n");
    printf("  +-------------------------------------------------------+\n\n");

    /* ====== Available Counters ====== */
    printf("Available PMU Counters:\n");
    printf("  +------------------------------------------------------+\n");
    printf("  | Counter ID  | Description           | Unit           |\n");
    printf("  +------------------------------------------------------+\n");
    printf("  | CYCLES      | Total clock cycles    | cycles         |\n");
    printf("  | INSTR       | Instructions executed | count          |\n");
    printf("  | CUBE_CYCLES | Cube unit active      | cycles         |\n");
    printf("  | VEC_CYCLES  | Vector unit active    | cycles         |\n");
    printf("  | DMA_LOAD_B  | Bytes loaded via DMA  | bytes          |\n");
    printf("  | DMA_STORE_B | Bytes stored via DMA  | bytes          |\n");
    printf("  | L1_HITS     | L1 cache hits         | count          |\n");
    printf("  | L1_MISSES   | L1 cache misses       | count          |\n");
    printf("  | MTE_CYCLES  | Memory transfer eng.  | cycles         |\n");
    printf("  | STALLS      | Pipeline stall cycles | cycles         |\n");
    printf("  +------------------------------------------------------+\n\n");

    /* ====== Basic Usage ====== */
    printf("Basic PMU Usage:\n");
    printf("  \n");
    printf("  # In kernel code (PTO-ISA):\n");
    printf("  \n");
    printf("  PMU_START CYCLES, VEC_CYCLES   # Start counting\n");
    printf("  \n");
    printf("  # ... kernel code to measure ...\n");
    printf("  VADD UB[0], UB[0], UB[1024]\n");
    printf("  VMUL UB[0], UB[0], UB[2048]\n");
    printf("  # ...\n");
    printf("  \n");
    printf("  PMU_STOP                        # Stop counting\n");
    printf("  \n");
    printf("  PMU_READ total_cycles, CYCLES   # Read counter\n");
    printf("  PMU_READ vec_cycles, VEC_CYCLES\n");
    printf("  \n");
    printf("  # Store for host to read\n");
    printf("  STORE GM[output_ptr], total_cycles\n");
    printf("  STORE GM[output_ptr+4], vec_cycles\n\n");

    /* ====== Compute Utilization ====== */
    printf("Compute Utilization Analysis:\n");
    printf("  \n");
    printf("  # Measure Cube unit utilization\n");
    printf("  PMU_START CYCLES, CUBE_CYCLES\n");
    printf("  # ... matrix multiply kernel ...\n");
    printf("  PMU_STOP\n");
    printf("  \n");
    printf("  PMU_READ total, CYCLES\n");
    printf("  PMU_READ cube, CUBE_CYCLES\n");
    printf("  \n");
    printf("  # Calculate utilization\n");
    printf("  cube_util = cube / total   # Should be close to 1.0\n");
    printf("  \n");
    printf("  +--------------------------------------------------+\n");
    printf("  | Utilization | Interpretation                     |\n");
    printf("  +--------------------------------------------------+\n");
    printf("  | > 90%%       | Excellent - compute bound          |\n");
    printf("  | 70%% - 90%%   | Good - some overhead               |\n");
    printf("  | 50%% - 70%%   | Fair - memory or sync bottleneck   |\n");
    printf("  | < 50%%       | Poor - investigate stalls          |\n");
    printf("  +--------------------------------------------------+\n\n");

    /* ====== Memory Bandwidth ====== */
    printf("Memory Bandwidth Measurement:\n");
    printf("  \n");
    printf("  # Measure DMA throughput\n");
    printf("  PMU_START CYCLES, DMA_LOAD_B, DMA_STORE_B\n");
    printf("  # ... memory-intensive kernel ...\n");
    printf("  PMU_STOP\n");
    printf("  \n");
    printf("  PMU_READ cycles, CYCLES\n");
    printf("  PMU_READ load_bytes, DMA_LOAD_B\n");
    printf("  PMU_READ store_bytes, DMA_STORE_B\n");
    printf("  \n");
    printf("  # Calculate bandwidth (GB/s)\n");
    printf("  clock_freq = 1.8e9  # 1.8 GHz typical\n");
    printf("  time_sec = cycles / clock_freq\n");
    printf("  bw_gb_s = (load_bytes + store_bytes) / time_sec / 1e9\n");
    printf("  \n");
    printf("  # Compare to peak bandwidth\n");
    printf("  peak_bw = 2000  # ~2 TB/s for A3/910C HBM\n");
    printf("  bw_util = bw_gb_s / peak_bw\n\n");

    /* ====== Roofline Analysis ====== */
    printf("Roofline Model Analysis:\n");
    printf("  \n");
    printf("  Roofline helps identify if kernel is:\n");
    printf("  - Memory-bound (bandwidth limited)\n");
    printf("  - Compute-bound (FLOPS limited)\n");
    printf("  \n");
    printf("  # Measure operational intensity\n");
    printf("  PMU_START CYCLES, DMA_LOAD_B, DMA_STORE_B, CUBE_CYCLES\n");
    printf("  # ... kernel ...\n");
    printf("  PMU_STOP\n");
    printf("  \n");
    printf("  # Calculate metrics\n");
    printf("  flops = cube_cycles * flops_per_cycle  # Hardware constant\n");
    printf("  bytes = load_bytes + store_bytes\n");
    printf("  op_intensity = flops / bytes  # FLOPs per byte\n");
    printf("  \n");
    printf("  # Ridge point\n");
    printf("  ridge = peak_flops / peak_bandwidth\n");
    printf("  \n");
    printf("  +-----------------------------------------+\n");
    printf("  | Op Intensity vs Ridge | Status          |\n");
    printf("  +-----------------------------------------+\n");
    printf("  | OI < ridge            | Memory-bound    |\n");
    printf("  | OI > ridge            | Compute-bound   |\n");
    printf("  | OI ~ ridge            | Balanced        |\n");
    printf("  +-----------------------------------------+\n");
    printf("  \n");
    printf("  A3/910C example:\n");
    printf("  - Peak: 320 TFLOPS (FP16)\n");
    printf("  - BW: 2 TB/s\n");
    printf("  - Ridge: 320/2 = 160 FLOPs/byte\n");
    printf("  \n");
    printf("  GEMM (large): OI ~ 50-100 -> memory-bound for small M,N\n");
    printf("  Elementwise: OI ~ 1-4 -> heavily memory-bound\n\n");

    /* ====== Pipeline Stalls ====== */
    printf("Pipeline Stall Analysis:\n");
    printf("  \n");
    printf("  # Identify stall sources\n");
    printf("  PMU_START CYCLES, STALLS, MTE_CYCLES, VEC_CYCLES\n");
    printf("  # ... kernel ...\n");
    printf("  PMU_STOP\n");
    printf("  \n");
    printf("  stall_pct = stalls / cycles\n");
    printf("  \n");
    printf("  Common stall causes:\n");
    printf("  +---------------------------------------------------+\n");
    printf("  | High MTE_CYCLES, low VEC | Memory transfer stalls |\n");
    printf("  | High STALLS              | Data dependencies      |\n");
    printf("  | Low all utilization      | Poor tiling/scheduling |\n");
    printf("  +---------------------------------------------------+\n\n");

    /* ====== Profiling Workflow ====== */
    printf("Profiling Workflow:\n");
    printf("  \n");
    printf("  1. Baseline measurement:\n");
    printf("     - Run kernel with PMU, record all counters\n");
    printf("     - Calculate utilization and bandwidth\n");
    printf("  \n");
    printf("  2. Identify bottleneck:\n");
    printf("     - Compute-bound? -> Optimize algorithm\n");
    printf("     - Memory-bound? -> Improve data locality\n");
    printf("     - Stall-bound? -> Better scheduling\n");
    printf("  \n");
    printf("  3. Optimize:\n");
    printf("     - Apply optimization\n");
    printf("     - Re-measure with PMU\n");
    printf("     - Compare to baseline\n");
    printf("  \n");
    printf("  4. Repeat until close to roofline limit\n\n");

    /* ====== Example Output ====== */
    printf("Example PMU Output:\n");
    printf("  ----------------------------------------\n");
    printf("  Kernel: matmul_tiled_256x256x256\n");
    printf("  \n");
    printf("  Counters:\n");
    printf("    CYCLES:      1,234,567\n");
    printf("    CUBE_CYCLES: 1,180,000 (95.6%% util)\n");
    printf("    DMA_LOAD_B:  524,288 (512 KB)\n");
    printf("    DMA_STORE_B: 262,144 (256 KB)\n");
    printf("    STALLS:      12,345 (1.0%%)\n");
    printf("  \n");
    printf("  Derived metrics:\n");
    printf("    Time: 0.686 ms (@ 1.8 GHz)\n");
    printf("    FLOPs: 33.5M (2*M*K*N)\n");
    printf("    Bandwidth: 1.15 GB/s\n");
    printf("    Op intensity: 42.6 FLOPs/byte\n");
    printf("  \n");
    printf("  Analysis: Good compute utilization.\n");
    printf("  OI < ridge(160), slightly memory-bound.\n");
    printf("  Consider larger tiles or prefetching.\n");
    printf("  ----------------------------------------\n\n");

    /* Cleanup */
    printf("Cleanup...\n");
    platform_shutdown();
    printf("Done\n");

    printf("\n=== End of PMU Example ===\n");
    return 0;
}
