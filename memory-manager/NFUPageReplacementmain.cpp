#include "os_mem_ex.h"
#include <stdio.h>
#include <assert.h>

int main()
{
    memory_manager_t mm;
    setup_memory_manager(&mm);

    printf("=== TEST 1: Pre-initialization and invalid parameters ===\n");
    assert(mm.read(0, 4) == -1);
    assert(mm.write(0, 4) == -1);
    assert(mm.destroy() == 0);
    assert(mm.create(0, 3) == 0);
    assert(mm.create(4, -2) == 0);

    // Initialize: page size = 4 bytes, physical frames = 3
    assert(mm.create(4, 3) == 1);
    assert(mm.create(4, 3) == 0); // Duplicate create must fail

    printf("\n=== TEST 2: Page boundary validation ===\n");
    assert(mm.page_num(mem_handle_t(80, 2)) == 20);
    assert(mm.page_num(mem_handle_t(83, 2)) == -1); // Crosses page boundary
    assert(mm.page_num(mem_handle_t(-1, 2)) == -1);
    assert(mm.page_num(mem_handle_t(0, 0)) == -1);

    printf("\n=== TEST 3: Filling 3 physical frames (V=20, V=3, V=4) ===\n");
    assert(mm.write(80, 2) == 1); // Frame 0: V=20, dirty=1, freq=1
    assert(mm.read(83, 2) == -1); // Boundary error
    assert(mm.read(15, 1) == 1);  // Frame 1: V=3,  dirty=0, freq=1
    assert(mm.write(16, 4) == 1); // Frame 2: V=4,  dirty=1, freq=1
    assert(mm.read(16, 4) == 1);  // Frame 2: V=4,  dirty=1, freq=2

    printf("\n=== TEST 4: NFU page replacement (clean vs dirty pages) ===\n");
    assert(mm.read(81, 1) == 1);  // Increase V=20 freq to 2
    assert(mm.write(0, 4) == 1);  // Evicts clean V=3 (freq=1), loads V=0 into Frame 1
    assert(mm.read(24, 3) == 1);  // Evicts dirty V=0 (freq=1 -> save), loads V=6 into Frame 1

    printf("\n=== TEST 5: Frequency restoration for re-loaded pages ===\n");
    assert(mm.read(0, 2) == 1);   // Evicts clean V=6, restores V=0 with previous freq=1 -> 2

    printf("\n=== TEST 6: Flush and destroy ===\n");
    mm.flush();                   // Saves dirty frames (Frame 0 and Frame 2)
    mm.flush();                   // Second flush produces no output

    assert(mm.destroy() == 1);
    assert(mm.destroy() == 0);
    assert(mm.read(0, 1) == -1);

    printf("\nAll tests passed successfully!\n");
    return 0;
}
