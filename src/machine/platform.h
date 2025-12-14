#pragma once

// Platform abstraction layer for Machine.
// CMake supplies a compile definition to choose implementation:
//   - MACHINE_PLATFORM_HOST (default)
//   - MACHINE_PLATFORM_BARE  (reserved for future)

#if !defined(MACHINE_PLATFORM_HOST) && !defined(MACHINE_PLATFORM_BARE)
#define MACHINE_PLATFORM_HOST
#endif

#if defined(MACHINE_PLATFORM_HOST)
#include "host/platform_host.h"
#elif defined(MACHINE_PLATFORM_BARE)
#include "bare/platform_bare.h"
#endif

