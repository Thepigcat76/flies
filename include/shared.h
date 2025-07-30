#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdbool.h>

#define NULLABLE

typedef struct {
  int width;
  int height;
} Dimensions;

char *str_fmt(const char *fmt, ...);

void printfn(const char *fmt, ...);

bool is_dir(const char *path);

void current_wd(char *cwd_buf, size_t buf_size);

bool str_eq(const char *a, const char *b);

char *read_file_to_string(const char *filename);

char *str_fmt_heap(const char *fmt, ...);

char *str_cpy_heap(const char *fmt, ...);
