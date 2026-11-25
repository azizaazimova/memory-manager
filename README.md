# memory-manager

# Virtual Memory Manager (NFU Page Replacement)

A C/C++ simulation of a paged virtual memory manager implementing the **NFU (Not Frequently Used)** page replacement algorithm, virtual-to-physical address translation, and dirty-page tracking.

## Features

- **Virtual Address Translation:** Maps virtual memory blocks to virtual page numbers while enforcing strict single-page boundary checks.
- **NFU Page Replacement:** Tracks page access frequencies and evicts the least frequently used page when physical frames are full.
- **Eviction History Tracking:** Preserves historical access counters for evicted pages so returning pages retain their prior usage weight.
- **Dirty Page Management:** Tracks modified pages (`write` operations) and flushes dirty frames back to storage on eviction or explicit `flush()` calls.
- **Zero Memory Leaks:** Full lifecycle management (`create` / `destroy`) with dynamic allocation cleanup.

## Project Structure

```text
├── os_mem_ex.h                 # Memory manager interface definitions
├── NFUPageReplacement.cpp      # Virtual memory manager & NFU implementation
├── NFUPageReplacementmain.cpp  # Unit tests covering edge cases and replacement logic
└── Makefile                    # Build and test automation
```

## Build & Run Tests

```bash
make test
```

To clean build artifacts:

```bash
make clean
```
