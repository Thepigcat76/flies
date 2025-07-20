#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

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
