#include "../include/app.h"
#include "../include/config.h"
#include "../include/shared.h"
#include "../include/term.h"
#include "lilc/array.h"
#include "lilc/log.h"
#include <dirent.h>
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#define NULLABLE

bool handle_input(App *app) {
  int ch = getch(); // Now ncurses handles escape sequences internally
  switch (ch) {
  case KEY_UP: {
    ssize_t old_index = app->dir_index;
    app->dir_index = fmax(app->dir_index - 1, 0);
    if (old_index == 0 && app->scrollable) {
      app->scroll_y_offset = fmax(app->scroll_y_offset - 1, 0);
    }
    app->update_rendering = true;
    break;
  }
  case KEY_DOWN: {
    if (app->dir_index < array_len(app->dir_entries) - 1) {
      if (app->dir_index == TERMINAL_ROWS - 1) {
        if (app->scrollable && app->scroll_y_offset + TERMINAL_ROWS - 1 !=
                                   array_len(app->dir_entries) - 1) {
          app->scroll_y_offset =
              fmin(array_len(app->dir_entries) - 1, app->scroll_y_offset + 1);
        }
      } else {
        app->dir_index++;
      }
      app->update_rendering = true;
    }
    break;
  }
  case KEY_RIGHT: { // →
    size_t len = array_len(app->prev_dirs);
    if (len > 0) {
      DirEntry entry = app->prev_dirs[len - 1];
      array_remove(app->prev_dirs, len - 1);
      app_open_dir(app, &entry);
    }
    app->update_rendering = true;
    break;
  }
  case KEY_LEFT: { // ←
    char *slash = strrchr(app->wd, '/');
    if (slash != NULL) {
      int index = app->wd - slash;
      if (index != 0) {
        app_open_dir(app, &PREV_DIR);
      }
    }
    break;
  }
  // ENTER key
  case '\n': {
    size_t len = strlen(app->input);
    if (len == 0) {
      DirEntry *entry = app_hovered_entry(app);
      app_open_entry(app, entry);
    } else {
      app_run_cmd(app);
    }
    break;
  }
  // CTRL + C
  case 3: {
    const DirEntry *dir_entry = app_hovered_entry(app);
    app_copy_file(app, dir_entry);
    break;
  }
  // CTRL + D
  case 4: {
    const DirEntry *dir_entry = app_hovered_entry(app);
    app_delete_file(app, dir_entry);
    break;
  }
  // CTRL + V
  case 22: {
    const char *dir = app->wd;
    app_paste_file(app, dir);
    break;
  }
  // CTRL + X
  case 24: {
    const DirEntry *dir_entry = app_hovered_entry(app);
    app->cut = true;
    app_copy_file(app, dir_entry);
    break;
  }
  // CTRL + Z
  case 26: {
    app_undo(app);
    break;
  }
  // CTRL + Q
  case 0x11: {
    return true;
  }
  default: {
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == ':' ||
        ch == '.' || ch == '?' || (ch >= '0' && ch <= '9')) {
      strcat(app->input, str_fmt("%c", ch));
    } else if (ch == ' ') {
      strcat(app->input, " ");
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      size_t len = strlen(app->input);
      if (len > 0)
        app->input[len - 1] = '\0';
    }
    app->update_rendering = true;
    break;
  }
  }
  return false;
}

int main(int argc, char **argv) {
  alloc_init();

  AppConfig config = app_config_load();

  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  APP = (App){.dir_entries = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
              .prev_dirs = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
              .terminal_dimensions = {.width = w.ws_col, .height = w.ws_row},
              .input = "",
              .initial = true,
              .info_msg = "Started :3",
              .action_history = history_new()};

  App *app = &APP;

  if (argc > 1) {
    if (strcmp(argv[1], ".") == 0) {
      current_wd(app->wd, 1024);
    } else if (strcmp(argv[1], "..") == 0) {
      current_wd(app->wd, 1024);
      app_open_dir(app, &PREV_DIR);
      array_clear(app->prev_dirs);
    } else {
      if (argv[1][0] == '/') {
        strcpy(app->wd, argv[1]);
      } else {
        char temp[1024];
        strcpy(temp, app->wd);
        snprintf(app->wd, 1024, "%s/%s", temp, argv[1]);
      }
      app_find_entries(app);
    }
  } else {
    current_wd(app->wd, 1024);
    app_find_entries(app);
  }

  signal(SIGWINCH, terminal_handle_sigwinch);
  signal(SIGSEGV, terminal_handle_sigsegv);
  setlocale(LC_ALL, "");
  initscr();

  terminal_enable_raw_mode();
  {
    while (true) {
      if (terminal_resized) {
        terminal_resized = 0;

        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        app->terminal_dimensions =
            (Dimensions){.width = w.ws_col, .height = w.ws_row};
        app->update_rendering = true;
        terminal_clear();
      }

      if (app->update_rendering || app->initial) {
        app_render(app);
        app->initial = false;
      }

      if (handle_input(app)) {
        goto end;
      }
    }
  }
end:
  terminal_disable_raw_mode();
  endwin();
  exit(0);
  // app_exit(app);
  // log_debug("E it ova");
}