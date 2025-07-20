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
