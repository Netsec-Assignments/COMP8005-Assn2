#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vector.h"

static const float VECTOR_GROWTH_RATE = 1.5;

int vector_init(vector_t* vec, size_t item_size, size_t cap)
{
    void* items;
    cap = cap == 0 ? VECTOR_DEFAULT_CAPACITY : cap;
    
    items = malloc(item_size * cap);
    if (!items) return -1;

    vec->items = items;
    vec->cap = cap;
    vec->size = 0;
    vec->item_size = item_size;

    return 0;
}

vector_t* vector_create(size_t item_size, size_t cap)
{
    vector_t* vec = malloc(sizeof(vector_t));
    if (!vec) return NULL;

    if (vector_init(vec, item_size, cap) == -1) return NULL;

    return vec;
}

void vector_remove_at(vector_t* vec, unsigned i)
{
    unsigned char* items = (unsigned char*)vec->items;
    unsigned char* prev = items + (vec->item_size * i);
    unsigned char* start = prev + vec->item_size;
    unsigned char* end = items + (vec->item_size * vec->size);
    unsigned char* cur;

    /* Shift all elements past i down by one */
    for (cur = start; cur != end; cur += vec->item_size)
    {
        memcpy(prev, cur, vec->item_size);
        prev = cur;
    }

    vec->size--;
}

int vector_insert_at(vector_t* vec, void* item, unsigned i)
{
    unsigned char* items;
    unsigned char* prev;
    unsigned char* start;
    unsigned char* end;
    unsigned char* cur;

    if (vec->size == vec->cap)
    {
        size_t new_cap = (size_t)roundf(vec->cap * VECTOR_GROWTH_RATE);
        if (vector_resize(vec, new_cap) == -1)
        {
            return -1;
        }
    }

    items = (unsigned char*)vec->items;
    end = items + (vec->item_size * vec->size);
    start = items + (vec->item_size * i);

    /* Shift items i through the end of the vector up by one */
    for (cur = end; cur > start; cur -= vec->item_size)
    {
        prev = cur - vec->item_size;
        memcpy(cur, prev, vec->item_size);
    }
    
    /* Copy the item into its position */
    memcpy(start, item, vec->item_size);
    vec->size++;

    return 0;
}

int vector_push_back(vector_t* vec, void* item)
{
    return vector_insert_at(vec, item, vec->size);
}

int vector_resize(vector_t* vec, size_t cap)
{
    void* new_items = realloc(vec->items, cap * vec->item_size);
    if (!new_items) return -1;
    vec->items = new_items;
    vec->cap = cap;
    return 0;
}

void vector_reverse_no_alloc(vector_t* vec, void* tmp)
{
    unsigned char* front;
    unsigned char* back;
    unsigned char* items = vec->items;

    for (front = items, back = items + (vec->item_size * (vec->size - 1)); front < back; front += vec->item_size, back -= vec->item_size)
    {
        memcpy(tmp, front, vec->item_size);
        memcpy(front, back, vec->item_size);
        memcpy(back, tmp, vec->item_size);
    }
}

int vector_reverse(vector_t* vec)
{
    unsigned char* tmp;
    unsigned char* items = (unsigned char*)vec->items;
    
    /* No space; use a temporary buffer */
    if (vec->size == vec->cap)
    {
        tmp = malloc(vec->item_size);
        if (!tmp)
        {
            return -1;
        }
        else
        {
            vector_reverse_no_alloc(vec, tmp);
            free(tmp);
            return 0;
        }
    }

    /* There's some space at the end of the vector; use that as scratch space */
    tmp = items + (vec->item_size * vec->size);
    vector_reverse_no_alloc(vec, tmp);
    return 0;
}

void vector_free(vector_t const* vec)
{
    free(vec->items);
}