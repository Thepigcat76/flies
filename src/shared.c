#include "../include/array.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char _internal_fmt_buf[8192];

char *str_fmt(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(_internal_fmt_buf, 4096, fmt, args);
  va_end(args);
  return _internal_fmt_buf;
}

void printfn(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  puts("");
}

bool is_dir(const char *path) {
  struct stat path_stat;
  if (stat(path, &path_stat) != 0) {
    printfn("Path: %s", path);
    perror("stat");
    return false;
  }
  return S_ISDIR(path_stat.st_mode);
}

void current_wd(char *cwd_buf, size_t buf_size) {
  if (!getcwd(cwd_buf, buf_size)) {
    perror("getcwd() error");
    exit(1);
  }
}

bool str_eq(const char *a, const char *b) { return strcmp(a, b) == 0; }

char *read_file_to_string(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    fprintf(stderr, "Error opening file %s - ", filename);
    perror("");
    return NULL;
  }

  // Seek to the end of the file to get the file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET); // Go back to the beginning of the file

  if (file_size < 0) {
    perror("Error determining file size");
    fclose(file);
    return NULL;
  }

  // Allocate memory for the file content + null terminator
  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    perror("Error allocating memory");
    fclose(file);
    return NULL;
  }

  // Read the file content into the buffer
  size_t read_size = fread(buffer, 1, file_size, file);
  if (read_size != file_size) {
    perror("Error reading file");
    free(buffer);
    fclose(file);
    return NULL;
  }

  // Null-terminate the string
  buffer[file_size] = '\0';

  fclose(file);
  return buffer;
}

char *str_cpy_heap(const char *str) {
  char *heap_str = malloc(strlen(str) + 1);
  strcpy(heap_str, str);
  return heap_str;
}

char *str_fmt_heap(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(_internal_fmt_buf, 4096, fmt, args);
  va_end(args);
  
  char *str = malloc(strlen(_internal_fmt_buf));
  strcpy(str, _internal_fmt_buf);
  return str;
}
