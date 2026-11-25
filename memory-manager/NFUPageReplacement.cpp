#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_mem_ex.h"

typedef struct evicted_page_node
{
    int virtual_page;
    int freq;
    evicted_page_node* next = NULL;
} evicted_page_node;

typedef struct page_frame
{
    int virtual_page;
    int dirty;
    int freq;
} page_frame;

static page_frame* physical_frames = NULL;
static evicted_page_node* evicted_history = NULL;
static int page_size = 0;
static int frame_count = 0;

int vmm_create(int size, int num_pages)
{
    if ((size > 0) && (num_pages > 0) && (!physical_frames))
    {
        physical_frames = (page_frame*)malloc(sizeof(page_frame) * num_pages);
        if (physical_frames)
        {
            page_size = size;
            frame_count = num_pages;
            for (int i = 0; i < frame_count; i++)
            {
                physical_frames[i].virtual_page = -1;
                physical_frames[i].dirty = 0;
                physical_frames[i].freq = 0;
            }
            return 1;
        }
    }
    return 0;
}

// Updates evicted page frequency and retrieves historical frequency if the page was previously evicted
static int sync_evicted_history(page_frame evicted_frame, int target_virtual_page)
{
    if (evicted_history)
    {
        int restored_freq = 0;
        evicted_page_node* curr = evicted_history;
        while (curr)
        {
            if (curr->virtual_page == evicted_frame.virtual_page)
            {
                curr->freq = evicted_frame.freq;
            }
            else if (curr->virtual_page == target_virtual_page)
            {
                restored_freq = curr->freq;
            }
            curr = curr->next;
        }

        evicted_page_node* new_node = (evicted_page_node*)malloc(sizeof(evicted_page_node));
        if (new_node)
        {
            new_node->next = evicted_history;
            new_node->freq = evicted_frame.freq;
            new_node->virtual_page = evicted_frame.virtual_page;
            evicted_history = new_node;
        }
        return restored_freq;
    }
    else
    {
        evicted_history = (evicted_page_node*)malloc(sizeof(evicted_page_node));
        if (evicted_history)
        {
            evicted_history->next = NULL;
            evicted_history->freq = evicted_frame.freq;
            evicted_history->virtual_page = evicted_frame.virtual_page;
        }
    }
    return 0;
}

int vmm_destroy()
{
    if (page_size)
    {
        free(physical_frames);
        physical_frames = NULL;
        page_size = 0;
        frame_count = 0;

        evicted_page_node* next_node = NULL;
        while (evicted_history)
        {
            next_node = evicted_history->next;
            free(evicted_history);
            evicted_history = next_node;
        }
        return 1;
    }
    return 0;
}

int vmm_page_num(mem_handle_t block)
{
    if ((page_size) && (block.size > 0) && (block.addr >= 0) &&
        ((block.addr % page_size + block.size) <= page_size))
    {
        return block.addr / page_size;
    }
    return -1;
}

// Loads virtual_page into a physical frame, applying NFU replacement if use_nfu is non-zero
static int load_page_into_frame(int frame_idx, int virtual_page, int use_nfu)
{
    if (use_nfu)
    {
        int victim_idx = 0;
        while (frame_idx < frame_count)
        {
            if (physical_frames[frame_idx].freq < physical_frames[victim_idx].freq)
            {
                victim_idx = frame_idx;
            }
            frame_idx++;
        }
        frame_idx = victim_idx;

        int restored_freq = sync_evicted_history(physical_frames[frame_idx], virtual_page);
        if (physical_frames[frame_idx].dirty)
        {
            printf("save %d %d\n", frame_idx, physical_frames[frame_idx].virtual_page);
        }
        physical_frames[frame_idx].dirty = 0;
        physical_frames[frame_idx].freq = restored_freq;
    }

    physical_frames[frame_idx].virtual_page = virtual_page;
    printf("load %d %d\n", virtual_page, frame_idx);
    return frame_idx;
}

static int handle_memory_access(int addr, int size, int is_write)
{
    mem_handle_t block(addr, size);
    int virtual_page = vmm_page_num(block);
    int frame_idx = 0;

    if (virtual_page != -1)
    {
        while (frame_idx < frame_count)
        {
            if (physical_frames[frame_idx].virtual_page == -1)
            {
                load_page_into_frame(frame_idx, virtual_page, 0);
                break;
            }
            else if (physical_frames[frame_idx].virtual_page == virtual_page)
            {
                break;
            }
            frame_idx++;
        }

        if (frame_idx == frame_count)
        {
            frame_idx = load_page_into_frame(0, virtual_page, 1);
        }

        if (is_write)
        {
            printf("write %d %d %d\n", addr, size, frame_idx);
            physical_frames[frame_idx].dirty = 1;
        }
        else
        {
            printf("read %d %d %d\n", addr, size, frame_idx);
        }

        physical_frames[frame_idx].freq += 1;
        return 1;
    }
    return -1;
}

int vmm_read(int addr, int size)
{
    return handle_memory_access(addr, size, 0);
}

int vmm_write(int addr, int size)
{
    return handle_memory_access(addr, size, 1);
}

void vmm_flush()
{
    for (int i = 0; i < frame_count; i++)
    {
        if (physical_frames[i].dirty)
        {
            physical_frames[i].dirty = 0;
            printf("save %d %d\n", i, physical_frames[i].virtual_page);
        }
    }
}

void setup_memory_manager(memory_manager_t* mm)
{
    mm->create = vmm_create;
    mm->destroy = vmm_destroy;
    mm->read = vmm_read;
    mm->write = vmm_write;
    mm->page_num = vmm_page_num;
    mm->flush = vmm_flush;
}
