#pragma once

#include <dirent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ARRAY(...) {__VA_ARGS__ __VA_OPT__(, ) NULL}

#define STR_CMP_OR(str, ...)                                                   \
  _internal_str_cmp_or(str, (char *[128]){__VA_ARGS__})

static bool _internal_str_cmp_or(char *base_str, char **strs) {
  for (int i = 0; strs[i] != NULL; i++) {
    char *str = strs[i];
    if (strcmp(base_str, str) == 0) {
      return true;
    }
  }
  return false;
}

static char _internal_src_files_buf[4096];
static char _internal_libs_buf[4096];
static char _internal_flags_buf[1024];
static char _internal_build_name_buf[256];
static char _internal_cmd_buf[8192];
static char _internal_fmt_buf[8192];

static char *str_fmt(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(_internal_fmt_buf, 4096, fmt, args);
  va_end(args);
  return _internal_fmt_buf;
}

static char *collect_src_files(char *directory) {
  struct dirent *entry;
  DIR *dp = opendir(directory);
  if (dp == NULL) {
    perror("opendir");
    exit(1);
  }

  while ((entry = readdir(dp)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (entry->d_type == DT_DIR) {
      char dir_buf[256];
      strcpy(dir_buf, str_fmt("%s/%s/", directory, entry->d_name));
      collect_src_files(dir_buf);
      continue;
    }

    const char *ext;
    const char *dot = strrchr(entry->d_name, '.');
    if (dot != NULL) {
      ext = dot + 1;
      if (ext && strcmp(ext, "c") == 0) {
        strcat(_internal_src_files_buf,
               str_fmt("%s/%s ", directory, entry->d_name));
      }
    }
  }

  closedir(dp);

  return _internal_src_files_buf;
}

static char *link_libs(char **libraries) {
  _internal_libs_buf[0] = '\0';
  if (libraries == NULL) {
    return _internal_libs_buf;
  }

  for (int i = 0; libraries[i] != NULL; i++) {
    strcat(_internal_libs_buf, str_fmt("-l%s ", libraries[i]));
  }
  return _internal_libs_buf;
}

static char *build_name(char *name, int target);

static char *build_flags(void *opts);

static char *build_compiler(char *default_compiler, int target);

static void make_dir(char *dirname) { mkdir(dirname, 0755); }

static int compile(const char *cmd_fmt, ...) {
  va_list args;
  va_start(args, cmd_fmt);
  vsnprintf(_internal_cmd_buf, 4096, cmd_fmt, args);
  va_end(args);

  return system(_internal_cmd_buf);
}

static int run(char *build_dir, char *build_name, int argc, char **argv) {
  char run_buf[2048];
  snprintf(run_buf, 2048, "./%s/%s ", build_dir, build_name);
  for (int i = 2; i < argc; i++) {
    strcat(run_buf, argv[i]);
    strcat(run_buf, " ");
  }
  puts(run_buf);
  return system(run_buf);
}

static void dbg(char *build_dir, char *build_name, bool dbg) {
  if (!dbg) {
    fprintf(stderr, "Debug needs to be enabled in the build options in order "
                    "to be able to debug the program\n");
    exit(1);
  }
  char dbg_buf[512];
  snprintf(dbg_buf, 512, "gdb ./%s/%s", build_dir, build_name);
  system(dbg_buf);
}

// Standard build file layout

/*
// To build the project, compile this file with a compiler of your choice and
run the compiled executable.
// The project also requires the gurd header, which can be found at
<https://github.com/Thepigcat76/gurd/blob/main/gurd.h>
//

#include "gurd.h"

typedef enum {
  TARGET_LINUX,
  TARGET_WIN,
} BuildTarget;

typedef struct {
  char *compiler;
  bool debug;
  bool release;
  char *std;
  BuildTarget target;
  char *out_dir;
  char *out_name;
  char *libraries[64];
} BuildOptions;

static BuildOptions OPTS = {.compiler = "clang",
                            .debug = true,
                            .release = false,
                            .std = "c23",
                            .target = TARGET_LINUX,
                            .out_dir = "./build/",
                            .out_name = "cozy-wrath",
                            .libraries = ARRAY("raylib", "GL", "m", "pthread",
"dl", "rt", "X11", "cjson")};

int main(int argc, char **argv) {
  char *compiler = build_compiler(OPTS.compiler, OPTS.target);
  char *files = collect_src_files("./src/");
  char *libraries = link_libs(OPTS.libraries);
  char *flags = build_flags(&OPTS);
  char *out_name = build_name(OPTS.out_name, OPTS.target);

  build_dir(OPTS.out_dir);
  compile("%s %s %s %s -o %s%s", OPTS.compiler, files, libraries, flags,
OPTS.out_dir, out_name);
  // printf("Cmd: %s\n", _internal_cmd_buf);

  if (argc >= 2) {
    if (STR_CMP_OR(argv[1], "r", "run")) {
      run(OPTS.out_dir, out_name);
      return 0;
    } else if (STR_CMP_OR(argv[1], "d", "dbg")) {
      dbg(OPTS.out_dir, out_name, OPTS.debug);
      return 0;
    } else {
      fprintf(stderr, "[Error]: Invalid first arg: %s\n", argv[1]);
      return 1;
    }
  }
}

static char *build_name(char *default_name, int target) {
  if (target == TARGET_WIN) {
    strcpy(_internal_build_name_buf, str_fmt("%s.exe", default_name));
    return _internal_build_name_buf;
  }
  return default_name;
}

static char *build_compiler(char *default_compiler, int target) {
  if (target == TARGET_WIN) {
    return "x86_64-w64-mingw32-gcc";
  }
  return default_compiler;
}

static char *build_flags(void *_opts) {
  BuildOptions *opts = (BuildOptions *)_opts;
  _internal_flags_buf[0] = '\0';

  if (opts->debug) {
    strcat(_internal_flags_buf, "-g ");
  } else if (opts->release) {
    strcat(_internal_flags_buf, "-O3 ");
  }

  if (opts->std != NULL) {
    strcat(_internal_flags_buf, str_fmt("-std=%s ", opts->std));
  }

  return _internal_flags_buf;
}

*/
