#pragma once

#include "shared.h"
#include <dirent.h>

typedef enum {
  DET_FILE,
  DET_DIR,
} DirEntryType;

typedef struct {
  char path[512];
  char name[128];
  char ext[16];
  DirEntryType type;
} DirEntry;

extern DirEntry PREV_DIR;

DirEntry dir_entry_new(const char *entry_path);

void dir_entry_render(const DirEntry *dir_entry, bool selected);

DirEntry *sort_dirs_alphabetic(DirEntry *strings);

