/* Owned index storage for the bounded evaluator's value arena. */
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    int32_t *data;
    size_t count;
    size_t capacity;
} IndexBuffer;

void *rawBufferCreate(int32_t capacity, int32_t stride)
{
    if (capacity < 0 || stride != sizeof(int32_t)
        || (size_t)capacity > SIZE_MAX / sizeof(int32_t)) return NULL;
    IndexBuffer *buffer = calloc(1, sizeof(*buffer));
    if (!buffer) abort();
    if (capacity) {
        buffer->data = malloc((size_t)capacity * sizeof(*buffer->data));
        if (!buffer->data) abort();
    }
    buffer->capacity = (size_t)capacity;
    return buffer;
}

int32_t rawBufferAppendInt(void *opaque, int32_t value)
{
    IndexBuffer *buffer = opaque;
    if (!buffer || buffer->count >= INT32_MAX) return -1;
    if (buffer->count == buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity * 2 : 16;
        if (capacity > INT32_MAX) capacity = INT32_MAX;
        if (capacity > SIZE_MAX / sizeof(*buffer->data)) return -1;
        int32_t *data = realloc(buffer->data, capacity * sizeof(*data));
        if (!data) abort();
        buffer->data = data;
        buffer->capacity = capacity;
    }
    buffer->data[buffer->count++] = value;
    return 0;
}

int32_t rawBufferCount(void *opaque)
{
    IndexBuffer *buffer = opaque;
    return buffer ? (int32_t)buffer->count : -1;
}

int32_t rawBufferLoadInt(void *opaque, int32_t index)
{
    IndexBuffer *buffer = opaque;
    if (!buffer || index < 0 || (size_t)index >= buffer->count) abort();
    return buffer->data[index];
}

int32_t rawBufferStoreInt(void *opaque, int32_t index, int32_t value)
{
    IndexBuffer *buffer = opaque;
    if (!buffer || index < 0 || (size_t)index >= buffer->count) return -1;
    buffer->data[index] = value;
    return 0;
}

int32_t rawBufferDestroy(void *opaque)
{
    IndexBuffer *buffer = opaque;
    if (!buffer) return -1;
    free(buffer->data);
    free(buffer);
    return 0;
}
