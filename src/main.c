#include <emscripten.h>
#include <emscripten/emscripten.h>

#include "../include/app.h"
#include "../include/array.h"
#include "../include/config.h"
#include "../include/shared.h"
#include <dirent.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define NULLABLE

EMSCRIPTEN_KEEPALIVE
void app_up_down(int move) {
  App *app = &APP;
  if (move == 1) {
    if (app->dir_index < array_len(app->dir_entries) - 1) {
      if (app->dir_index == app->config.max_rows - 1) {
        if (app->scrollable &&
            app->scroll_y_offset + app->config.max_rows - 1 !=
                array_len(app->dir_entries) - 1) {
          app->scroll_y_offset =
              fmin(array_len(app->dir_entries) - 1, app->scroll_y_offset + 1);
        }
      } else {
        app->dir_index++;
      }
      app->update_rendering = true;
    }
  } else {
    ssize_t old_index = app->dir_index;
    app->dir_index = fmax(app->dir_index - 1, 0);
    if (old_index == 0) {
      if (app->scrollable) {
        app->scroll_y_offset = fmax(app->scroll_y_offset - 1, 0);
      }
    }
    app->update_rendering = true;
  }
}

EMSCRIPTEN_KEEPALIVE
void app_left_right(int move) {
  App *app = &APP;
  if (move == 1) {
    char *slash = strrchr(app->wd, '/');
    if (slash != NULL) {
      int index = app->wd - slash;
      if (index != 0) {
        app_open_dir(app, &PREV_DIR);
      }
    }
  } else {
    size_t len = array_len(app->prev_dirs);
    if (len > 0) {
      DirEntry entry = app->prev_dirs[len - 1];
      array_remove(app->prev_dirs, len - 1);
      app_open_dir(app, &entry);
    }
    app->update_rendering = true;
  }
}

EMSCRIPTEN_KEEPALIVE
char *app_open_hovered_file(void) {
  App *app = &APP;
  DirEntry *entry = app_hovered_entry(app);
  return read_file_to_string(entry->path);
}

EMSCRIPTEN_KEEPALIVE
char *app_open_hovered_entry(void) {
  App *app = &APP;
  DirEntry *entry = app_hovered_entry(app);
  if (str_eq(app->wd, "/")) {
    if (str_eq(entry->name, "..")) {
      return "DIR";
    }
  }
  return app_open_entry(app, entry);
}

EMSCRIPTEN_KEEPALIVE
void app_run_cmds(char *cmd) {
  App *app = &APP;
  strcpy(app->input, cmd);
  app_run_cmd(app);
}

EMSCRIPTEN_KEEPALIVE
void app_ctrl_key(char c) {
  printfn("App ctrl key %c", c);
  App *app = &APP;
  if (c == 'd') {
    const DirEntry *dir_entry = app_hovered_entry(app);
    app_delete_file(app, dir_entry);
  } else if (c == 'c') {
    printfn("Copy keybind");
    const DirEntry *dir_entry = app_hovered_entry(app);
    app_copy_file(app, dir_entry);
  } else if (c == 'v') {
    const char *dir = app->wd;
    app_paste_file(app, dir);
  } else if (c == 'z') {
    app_undo(app);
  } else if (c == 'x') {
    const DirEntry *dir_entry = app_hovered_entry(app);
    app->cut = true;
    app_copy_file(app, dir_entry);
  }
  app_refresh(app);
}

EMSCRIPTEN_KEEPALIVE
void app_input(char c) {
  printfn("Input: %c", c);
  App *app = &APP;
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ':' ||
      c == '.' || c == '?' || (c >= '0' && c <= '9')) {
    strcat(app->input, str_fmt("%c", c));
  } else if (c == ' ') {
    strcat(app->input, " ");
  } else if (c == '\b') {
    size_t len = strlen(app->input);
    if (len > 0)
      app->input[len - 1] = '\0';
  } else if (c == '\n') {
    if (strlen(app->input) == 0) {
      DirEntry *entry = app_hovered_entry(app);
      app_open_entry(app, entry);
    } else {
      app_run_cmd(app);
    }
  }
  app->update_rendering = true;
}

EMSCRIPTEN_KEEPALIVE
char *app_render_frame() {
  App *app = &APP;

  if (app->update_rendering || app->initial) {
    app_render(app);
    app->update_rendering = false;
    app->initial = false;
  }

  return app->term.buffer;
}

EMSCRIPTEN_KEEPALIVE
void app_init() {
  App *app = &APP;

  alloc_init();
  AppConfig config = app_config_load();

  APP = (App){.dir_entries = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
              .prev_dirs = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
              .initial = true,
              .input = "",
              .info_msg = "Started in WebAssembly",
              .action_history = history_new(),
              .config = config};

  app = &APP;
  current_wd(app->wd, 1024);
  app_find_entries(app);
  app->update_rendering = true;

  term_printf("App initialized\n");
}
