#pragma once

#include "shared.h"
#include <dirent.h>

typedef enum {
  DET_FILE,
  DET_DIR,
} DirEntryType;

typedef struct {
  char name[512];
  DirEntryType type;
  NULLABLE char *ext;
} DirEntry;

extern DirEntry PREV_DIR;

DirEntry dir_entry_new(const char *path, struct dirent *entry);

void dir_entry_render(const DirEntry *dir_entry, bool selected);

DirEntry *sort_dirs_alphabetic(DirEntry *strings);

