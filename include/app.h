#pragma once

#include "dir.h"
#include "shared.h"
#include "term.h"
#include "history.h"
#include "config.h"

typedef struct {
  char wd[1024];
  DirEntry *dir_entries;
  DirEntry *prev_dirs;
  bool initial;
  bool update_rendering;
  size_t dir_index;
  size_t prev_len;
  Dimensions terminal_dimensions;
  char input[256];
  char debug_message[256];
  bool cut;
  History action_history;
  AppConfig config;
} App;

extern App APP;

void app_find_entries(App *app);

void app_open_dir(App *app, const DirEntry *entry);

void app_render(App *app);

void app_run_cmd(App *app);

void app_exit(App *app);

void app_set_debug_msg(App *app, const char *debug_message);

void app_refresh(App *app);

void app_copy_file(App *app, const DirEntry *entry);

void app_paste_file(App *app, const char *dir);

void app_delete_file(App *app, const DirEntry *entry);

void app_undo(App *app);

void app_new_file(App *app, DirEntry entry);

void app_new_dir(App *app, DirEntry entry);

void app_open_entry(App *app, const DirEntry *entry);

AppConfig app_config_load(void);
