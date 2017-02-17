// Convenience macro to get the next item from the ring buffer and cast it to a type
#define ring_buffer_get_typed(buf, type) (type*)ring_buffer_get(buf)

typedef struct
{
    atomic_size_t head;
    atomic_size_t tail;

    unsigned char* mem; // Should probably be a static fixed-size buffer since it will never be freed
    size_t size;
    size_t elem_size;    
} ring_buffer_t;

/**
 * Tries to add an element to the buffer. This will block if the buffer is full.
 *
 * @param buf       The buffer to initialise.
 * @param mem       The memory to use as a ciruclar buffer.
 * @param size      The number of elem_size elements that the buffer can hold.
 * @param elem_size The size of each element in the buffer.
 */
void ring_buffer_init(ring_buffer_t* buf, void* mem, size_t size, size_t elem_size);

/**
 * Tries to add an element to the buffer. This will block if the buffer is full.
 *
 * @param buf  The buffer to which to add the item.
 * @param item A pointer to the item (which will be copied by value) to add to the buffer.
 */
void ring_buffer_put(ring_buffer_t* buf, void* item);

/**
 * Retrieves the next item from the ring buffer. Blocks if there are no items available.
 *
 * @param buf The buffer from which to retrieve the item.
 * @return A pointer to the retrieved element. The caller should copy this value if it will
 *         be cached/used for a long time since the buffer _could_ wrap around and overwrite
 *         the memory in the returned pointer.
 */
void* ring_buffer_get(ring_buffer_t* buf);