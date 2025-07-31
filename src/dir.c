#include "../include/dir.h"
#include "../include/array.h"
#include <stdlib.h>
#include <string.h>

DirEntry PREV_DIR = {.name = "..", .type = DET_FILE, .ext = ""};

DirEntry dir_entry_new(const char *path) {
  DirEntry entry;
  strcpy(entry.path, path);
  bool dir = is_dir(entry.path);
  const char *slash = strrchr(entry.path, '/');
  strcpy(entry.name, slash != NULL ? slash + 1 : "");
  const char *dot = strrchr(entry.path, '.');
  strcpy(entry.ext, dir ? "" : (dot != NULL ? dot + 1 : ""));
  entry.type = dir ? DET_DIR : DET_FILE;
  return entry;
}

void dir_entry_render(const DirEntry *dir_entry, bool selected) {
  printfn("%s%s %s%s", selected ? "\e[47m" : "",
          dir_entry->type == DET_DIR ? "\xF0\x9F\x93\x81" : "\xF0\x9F\x93\x84",
          dir_entry->name, selected ? "\e[0m" : "");
}

static int cmp_dir_names(const void *a, const void *b) {
  const char *s1 = ((const DirEntry *)a)->name;
  const char *s2 = ((const DirEntry *)b)->name;
  return strcmp(s1, s2);
}

DirEntry *sort_dirs_alphabetic(DirEntry *dirs) {
  int len = array_len(dirs);
  if (len == 0)
    return NULL;

  // Copy to native C array
  DirEntry *temp = malloc(sizeof(DirEntry) * len);
  for (int i = 0; i < len; i++) {
    temp[i] = dirs[i];
  }

  // Sort it
  qsort(temp, len, sizeof(DirEntry), cmp_dir_names);

  // Create sorted array
  DirEntry *sorted =
      array_new_capacity(DirEntry, array_len(dirs), &HEAP_ALLOCATOR);
  for (int i = 0; i < len; i++) {
    array_add(sorted, temp[i]);
  }

  free(temp);
  return sorted;
}
