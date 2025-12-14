#include "platform_host.h"

#include <cstdlib>

namespace platform {

void* Alloc(std::size_t size) {
    return std::malloc(size);
}

void Free(void* ptr) {
    std::free(ptr);
}

}  // namespace platform

