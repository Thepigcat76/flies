#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdbool.h>

#define NULLABLE

#define TEXT_EDITOR "micro"

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

char *str_cpy_heap(const char *str);

void stack_trace_print(void);

#define ANSI_RED "\033[1;31m"
#define ANSI_RESET "\033[0m"

#define PANIC(...)                                                                                                                \
  do {                                                                                                                                     \
    /* We allocate a huge buffer cuz this is gonna crash the program anyways lol */                                                        \
    char buf[4096 + 2];                                                                                                                    \
    __VA_OPT__(snprintf(buf, 4096 + 2, __VA_ARGS__);)                                                                               \
    buf[4096] = '\n';                                                                                                                      \
    buf[4096 + 1] = '\0';                                                                                                                  \
    puts(ANSI_RED "Program panicked" ANSI_RESET);                                                                                          \
    __VA_OPT__(puts(buf);)                                                                                                         \
    puts("Crashed at:");                                                                                                                   \
    stack_trace_print();                                                                                                                   \
    exit(1);                                                                                                                               \
  } while (0)
