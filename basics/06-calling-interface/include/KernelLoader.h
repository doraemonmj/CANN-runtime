#pragma once

#include <dlfcn.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <cstdint>

typedef void (*KernelFunc)(void *);

/**
 * KernelLoader - Loads a kernel SO and provides the kernel function pointer
 *
 * This class is responsible ONLY for loading the SO file and extracting
 * the kernel function. It does NOT manage argument buffers.
 *
 * Usage:
 *   KernelLoader loader("./libkernel.so");
 *   KernelFunc func = loader.kernel();
 *   func(args_buffer);  // args_buffer managed separately
 */
class KernelLoader {
public:
    explicit KernelLoader(const std::string& so_path)
        : so_path_(so_path)
        , handle_(nullptr)
        , kernel_func_(nullptr)
    {
        load();
    }

    ~KernelLoader() {
        if (handle_) {
            dlclose(handle_);
            handle_ = nullptr;
        }
    }

    // Disable copy, allow move
    KernelLoader(const KernelLoader&) = delete;
    KernelLoader& operator=(const KernelLoader&) = delete;
    KernelLoader(KernelLoader&&) = default;
    KernelLoader& operator=(KernelLoader&&) = default;

    // Get the kernel function pointer
    KernelFunc kernel() const {
        if (!kernel_func_) {
            throw std::runtime_error("Kernel function not loaded");
        }
        return kernel_func_;
    }

private:
    void load() {
        handle_ = dlopen(so_path_.c_str(), RTLD_LAZY);
        if (!handle_) {
            throw std::runtime_error(std::string("Cannot load SO: ") + dlerror());
        }

        kernel_func_ = reinterpret_cast<KernelFunc>(dlsym(handle_, "kernel"));
        if (!kernel_func_) {
            dlclose(handle_);
            handle_ = nullptr;
            throw std::runtime_error(std::string("Cannot find kernel symbol: ") + dlerror());
        }
    }

    std::string so_path_;
    void* handle_;
    KernelFunc kernel_func_;
};
