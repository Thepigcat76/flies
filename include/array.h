#pragma once

#include "alloc.h"
#include <string.h>

#define ARRAY_INITIAL_CAPACITY 16

#define array_new(type, allocator)                                             \
  (type *)_internal_array_new(ARRAY_INITIAL_CAPACITY, sizeof(type), allocator)

#define array_new_capacity(type, capacity, allocator)                          \
  (type *)_internal_array_new(capacity, sizeof(type), allocator)

#define array_len(arr) _internal_array_len(arr)

#define array_free(arr) _internal_array_free(arr)

typedef struct {
  Allocator *allocator;
  size_t capacity;
  size_t len;
  size_t item_size;
} _InternalArrayHeader;

void *_internal_array_new(size_t capacity, size_t item_size,
                          Allocator *allocator);

void _internal_array_set_len(void *arr, size_t len);

size_t _internal_array_len(void *arr);

void _internal_array_free(void *arr);

void _internal_array_add(void **arr_ptr, void *item, size_t item_size);

void _internal_array_remove(void *arr_ptr, size_t index);

void _internal_array_clear(void *arr_ptr);

void _internal_array_set(void **arr_ptr, void *item, size_t index,
                         size_t item_size);

#define array_set(arr, index, ...)                                             \
  do {                                                                         \
    __typeof__(*(arr)) _tmp = (__VA_ARGS__);                                   \
    _internal_array_set((void **)&(arr), &_tmp, index, sizeof(_tmp));          \
  } while (0)

#define array_fill(arr, n, ...)                                                \
  do {                                                                         \
    for (int i = 0; i < n; i++) {                                              \
      array_add(arr, __VA_ARGS__);                                             \
    }                                                                          \
  } while (0)

#define array_add(arr, ...)                                                    \
  do {                                                                         \
    __typeof__(*(arr)) _tmp = (__VA_ARGS__);                                   \
    _internal_array_add((void **)&(arr), &_tmp, sizeof(_tmp));                 \
  } while (0)

#define array_remove(array, index) _internal_array_remove(array, index)

#define array_clear(array) _internal_array_clear(array)