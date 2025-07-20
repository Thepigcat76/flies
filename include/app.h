#pragma once

#include "dir.h"
#include "shared.h"

typedef struct {
  char wd[1024];
  DirEntry *dir_entries;
  DirEntry *prev_dirs;
  bool initial;
  bool update_rendering;
  size_t dir_index;
  size_t prev_len;
  Dimensions terminal_dimensions;
} App;

extern App APP;

void app_find_entries(App *app);

void app_open_dir(App *app, const DirEntry *entry);

void app_render(App *app);
