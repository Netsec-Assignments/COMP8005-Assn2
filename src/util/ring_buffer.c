#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t* buf, void* mem, size_t size, size_t elem_size)
{
    buf->mem = mem;
    buf->size = size;
    buf->elem_size = elem_size;
    buf->head = 0;
    buf->tail = 0;
}

void ring_buffer_put(ring_buffer_t* buf, void* item)
{
    size_t diff = 0;
    do
    {
        size_t head = atomic_load_explicit(buf->head);
        //size_t tail = atomic;
    } while (diff == buf->size);
}