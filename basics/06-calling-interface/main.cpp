#include <dlfcn.h>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>

#ifdef __APPLE__
    #define LIB_EXT ".dylib"
#else
    #define LIB_EXT ".so"
#endif

typedef void (*KernelFunc)(void *);

// Pack arguments for add_kernel: 3 ints (a, b, c)
void pack_add_args(void *args, int a, int b, int c) {
    struct __attribute__((packed)) AddArgs {
        int a;
        int b;
        int c;
    };
    AddArgs *add_args = (AddArgs *)args;
    add_args->a = a;
    add_args->b = b;
    add_args->c = c;
}

// Pack arguments for mul_kernel: 1 int (a) + 2 floats (b, c)
void pack_mul_args(void *args, int a, float b, float c) {
    struct __attribute__((packed)) MulArgs {
        int a;
        float b;
        float c;
    };
    MulArgs *mul_args = (MulArgs *)args;
    mul_args->a = a;
    mul_args->b = b;
    mul_args->c = c;
}

int main() {
    // Test add_kernel
    std::string add_lib_name = "./libadd_kernel" + std::string(LIB_EXT);
    void *add_handle = dlopen(add_lib_name.c_str(), RTLD_LAZY);
    if (!add_handle) {
        std::cerr << "Cannot load add_kernel: " << dlerror() << std::endl;
        return 1;
    }

    KernelFunc add_kernel = (KernelFunc)dlsym(add_handle, "kernel");
    if (!add_kernel) {
        std::cerr << "Cannot find kernel symbol in add_kernel: " << dlerror() << std::endl;
        dlclose(add_handle);
        return 1;
    }

    // Prepare arguments for add_kernel: 3 ints (a, b, c)
    void *add_args = malloc(3 * sizeof(int));
    pack_add_args(add_args, 10, 20, 0);
    
    struct __attribute__((packed)) AddArgs {
        int a;
        int b;
        int c;
    };
    AddArgs *add_args_ptr = (AddArgs *)add_args;
    add_kernel(add_args);
    std::cout << "add_kernel: " << add_args_ptr->a << " + " << add_args_ptr->b 
              << " = " << add_args_ptr->c << std::endl;

    // Test mul_kernel
    std::string mul_lib_name = "./libmul_kernel" + std::string(LIB_EXT);
    void *mul_handle = dlopen(mul_lib_name.c_str(), RTLD_LAZY);
    if (!mul_handle) {
        std::cerr << "Cannot load mul_kernel: " << dlerror() << std::endl;
        free(add_args);
        dlclose(add_handle);
        return 1;
    }

    KernelFunc mul_kernel = (KernelFunc)dlsym(mul_handle, "kernel");
    if (!mul_kernel) {
        std::cerr << "Cannot find kernel symbol in mul_kernel: " << dlerror() << std::endl;
        free(add_args);
        dlclose(add_handle);
        dlclose(mul_handle);
        return 1;
    }

    // Prepare arguments for mul_kernel: 1 int (a) + 2 floats (b, c)
    // Use add_kernel's output (c) as mul_kernel's input (a)
    // Memory layout: int (4 bytes) + float (4 bytes) + float (4 bytes)
    void *mul_args = malloc(sizeof(int) + 2 * sizeof(float));
    pack_mul_args(mul_args, add_args_ptr->c, 3.5f, 0.0f);
    
    struct __attribute__((packed)) MulArgs {
        int a;
        float b;
        float c;
    };
    MulArgs *mul_args_ptr = (MulArgs *)mul_args;
    mul_kernel(mul_args);
    std::cout << "mul_kernel: " << mul_args_ptr->a << " * " << mul_args_ptr->b 
              << " = " << mul_args_ptr->c << std::endl;
    free(mul_args);
    free(add_args);

    dlclose(mul_handle);
    dlclose(add_handle);

    return 0;
}

