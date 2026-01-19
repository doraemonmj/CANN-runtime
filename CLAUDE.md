# AI Assistant Instructions

This is an **Ascend NPU programming guide**. You teach hardware concepts through minimal, progressive, standalone examples using direct CANN APIs.

## Project Purpose

This is a **pure learning resource**. Each example demonstrates one hardware concept using real CANN (Ascend Computing Architecture) APIs.

**What this is:**
- Educational tutorial showing Ascend NPU hardware features
- Progressive examples (01 → 20) teaching concepts step-by-step
- Direct API usage demonstrating hardware behavior explicitly
- Each example is self-contained and independently runnable

**What this is NOT:**
- Production runtime library or framework
- Scheduling system or task orchestrator
- Abstraction layer over CANN
- Framework integration (PyTorch/TensorFlow)

## Hardware Model (Simplified)

```
Host CPU
  ↓ PCIe (~3μs)
AICPU (control CPU, coordinates AICore blocks)
  ↓ On-chip (~0μs)
AICore Blocks (24 blocks, each containing):
  - 1 Cube Core (matrix operations)
  - 2 Vector Cores (element-wise SIMD)
  - 1 Scalar Unit (control flow)
  - Shared L1 Buffer

Latencies:
  Host CPU → AICPU/AICore: ~3μs (PCIe transfer)
  AICPU → AICore:          ~0μs (tightly coupled on-chip)
```

## Core Constraints

When assisting with this codebase:

### DO NOT
- Implement scheduling algorithms or policies
- Add framework integration (PyTorch, TensorFlow)
- Create production optimizations or distributed computing logic
- Add abstraction layers over CANN APIs
- Add features beyond what's explicitly requested

### DO
- Focus on hardware behavior and concepts
- Write minimal examples that teach exactly ONE concept
- Use direct CANN/ACL APIs (no abstraction)
- Use hardware-first explanations (what the hardware does, not software abstractions)
- Build examples progressively (each builds on previous)
- Keep code self-documenting
- Add README.md to each example with markdown links to code

## Code Requirements

- **Standard**: C++11/17 (matches CANN requirements)
- **APIs**: Direct ACL (Ascend Computing Language) APIs only
- **Comments**: Explain WHY (hardware behavior), not WHAT (code logic)
- **Dependencies**: Only CANN runtime - no other libraries
- **Structure**: Standalone examples, each with own CMakeLists.txt and README.md

## Example Structure

Each example should:
1. State the concept being demonstrated (in README.md)
2. Provide minimal code showing the concept (using direct ACL APIs)
3. Explain behavior at the hardware level (in README.md)
4. Link to specific code lines using markdown
5. Build independently without dependencies on other examples

```
examples/
  01-device-query/
    CMakeLists.txt    # Standalone build
    main.cpp          # Uses <acl/acl.h> directly
    README.md         # Explains concept, links to code
  02-memory/
    CMakeLists.txt
    main.cpp
    README.md
  03-stream/
    ...
```

Principles:
1. code structure for each examples
    ```txt
    0x-xxx/
      aicpu/ (if needed)
        xxx
        CMakeLists.txt
      aicore/ (if needed)
        xxx
        CMakeLists.txt
      xxx
      CMakeLists.txt
    ```
    The aicore kernel is mainly for computing. The aicpu kernel is mainly for controlling aicore, no need to run compute tasks on aicpu. And you can use at most 4 aicpus and 24 aicores (24 aic + 48 aiv)
2. The code can run with:
    ```bash
    cd examples/0x-xxx/
    mkdir build
    cd build
    cmake ..
    make
    ./xxx
    ```
3. The code can assume that the CANN location is in `$ASCEND_HOME_PATH` env. When testing, always get what to source like `/xxx/ascend-toolkit/latest/bin/setenv.bash` in `~/.bashrc` first to set up the environment properly.
4. The readthedocs doc page shall explain those examples. Each for one page. When editing code, the doc page shall be edited at the same time.

## Repository Workflow

This repository follows a structured workflow for implementing and documenting examples:

### 1. Project Tracking (README.md)
The root `README.md` maintains:
- **Contents**: Complete list of all planned examples (01-20+)
- **Status**: Implementation status for each example
  - ❌ Not started
  - 🚧 In progress
  - ✅ Complete (code + docs)

### 2. Example Implementation Workflow
For each example, follow this sequence:

1. **Implement**: Write the example code
   - Create example directory structure (`examples/XX-example/`)
   - Write minimal, focused code demonstrating ONE concept
   - Use direct ACL APIs only
   - Add hardware-behavior focused comments

2. **Test**: Verify the example works
   - Build the example: `cmake .. && make`
   - Run and verify output matches expected behavior
   - Test on actual Ascend hardware when possible

3. **Clean Up**: Refine code and documentation
   - Remove unnecessary code
   - Ensure comments explain WHY (hardware behavior), not WHAT (code logic)
   - Keep code minimal and focused

4. **Document Example**: Create `examples/XX-example/README.md`
   - Follow the documentation pattern below
   - Use markdown links to reference code: `[filename](filename)` and `[filename:line](filename#Lline)`
   - Links ensure docs stay in sync with code

5. **Document in ReadTheDocs**: Update `docs/` folder
   - Create corresponding page in `docs/` for the example
   - Reference code using links (same as example README)
   - Explain hardware concepts and behavior
   - Link to specific code lines so docs don't duplicate code

6. **Update Status**: Mark example as complete in root `README.md`

### 3. Documentation Synchronization
**Critical**: Code and docs use markdown links for synchronization:
- Example README.md links to its own code files
- ReadTheDocs pages link to code in `examples/`
- When code changes, links remain valid (line numbers may need updates)
- This prevents duplication and keeps docs in sync

## Documentation Pattern

Each `examples/XX-example/README.md` should include:
- **Concept**: One-sentence description
- **Hardware Behavior**: What happens at the hardware level
- **Code Structure**: Links to files using `[filename](filename)`
- **Key CANN APIs**: Links to specific lines using `[filename:line](filename#Lline)`
- **Running**: Build and execution instructions
- **Expected Output**: What the user should see
- **Next Steps**: Link to next example

Each `docs/examples/XX-example.md` (ReadTheDocs page) should include:
- **Overview**: Concept being taught
- **Hardware Explanation**: Detailed hardware behavior
- **Implementation**: Links to code files and key lines
- **API Usage**: CANN APIs used with links to code
- **Running the Example**: Build and execution
- **Key Takeaways**: What the user learned
- **Next Example**: Link to next doc page

This pattern ensures documentation stays synchronized with code through markdown links.
