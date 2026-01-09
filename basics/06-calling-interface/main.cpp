#include <iostream>
#include <string>
#include <cstdlib>
#include "include/KernelLoader.h"
#include "packer/AddKernelPacker.h"
#include "packer/MulKernelPacker.h"

#ifdef __APPLE__
    #define LIB_EXT ".dylib"
#else
    #define LIB_EXT ".so"
#endif

int main() {
    try {
        // Load kernels once (reusable!)
        std::string add_lib_name = "./libadd_kernel" + std::string(LIB_EXT);
        std::string mul_lib_name = "./libmul_kernel" + std::string(LIB_EXT);

        KernelLoader add_loader(add_lib_name);
        KernelLoader mul_loader(mul_lib_name);

        // Create packers (stateless, lightweight)
        AddKernelPacker add_packer;
        MulKernelPacker mul_packer;

        // Allocate argument buffers (caller manages memory)
        void* add_args = malloc(AddKernelPacker::getArgsBufferSize());
        void* mul_args = malloc(MulKernelPacker::getArgsBufferSize());

        if (!add_args || !mul_args) {
            std::cerr << "Failed to allocate argument buffers" << std::endl;
            free(add_args);
            free(mul_args);
            return 1;
        }

        // Execute add_kernel: 10 + 20 = ?
        void* add_ptr = add_packer.pack(add_args, 10, 20);

        KernelFunc add_func = add_loader.kernel();
        add_func(add_ptr);

        int add_result;
        add_packer.getResult(add_args, add_result);
        std::cout << "add_kernel: 10 + 20 = " << add_result << std::endl;

        // Execute mul_kernel: add_result * 3.5 = ?
        void* mul_ptr = mul_packer.pack(mul_args, add_result, 3.5f);

        KernelFunc mul_func = mul_loader.kernel();
        mul_func(mul_ptr);

        float mul_result;
        mul_packer.getResult(mul_args, mul_result);
        std::cout << "mul_kernel: " << add_result << " * 3.5 = " << mul_result << std::endl;

        // Clean up
        free(mul_args);
        free(add_args);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

