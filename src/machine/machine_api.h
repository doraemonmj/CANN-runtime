#pragma once

#include <cstddef>

// Plain C-compatible task definitions for external callers (e.g., ctypes).
struct Runable {
    int data_length;       // size of data buffer in bytes
    void* data;            // caller-managed buffer
    int binary_length;     // size of binary path buffer
    void* binary;          // pointer to null-terminated shared library path
    int entry_length;      // size of entry point buffer
    const char* entry_point; // pointer to null-terminated symbol name
};

struct Task {
    int id;
    Runable runable;
};

extern "C" {
// C interface for Python ctypes and other FFI callers.
struct Machine;
Machine* CreateMachine();
void DestroyMachine(Machine* machine);
// Enqueue/Dequeue generic tasks; returns 1 on success, 0 on failure/empty.
int PushTaskC(Machine* machine, const Task* task);
int PopTaskC(Machine* machine, Task* task);
}

