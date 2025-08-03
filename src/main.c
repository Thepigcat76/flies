#include "../include/app.h"
#include "../include/array.h"
#include "../include/config.h"
#include "../include/shared.h"
#include "../include/term.h"
#include <dirent.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define NULLABLE

int main(int argc, char **argv) {
  alloc_init();

  AppConfig config = app_config_load();

  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  APP = (App){.dir_entries = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
              .prev_dirs = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
              .initial = true,
              .terminal_dimensions = {.width = w.ws_col, .height = w.ws_row},
              .input = "",
              .info_msg = "Started :3",
              .action_history = history_new(),
              .config = config};

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

  terminal_clear();

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
      }

      char c = getchar();
      switch (c) {
      case '\033': {
        if (getchar() == '[') {
          char arrow = getchar();
          switch (arrow) {
          case 'A': {
            ssize_t old_index = app->dir_index;
            app->dir_index = fmax(app->dir_index - 1, 0);
            if (old_index == 0) {
              if (app->scrollable) {
                app->scroll_y_offset = fmax(app->scroll_y_offset - 1, 0);
              }
            }
            app->update_rendering = true;
            break;
          }
          case 'B': {
            if (app->dir_index < array_len(app->dir_entries) - 1) {
              if (app->dir_index == app->config.max_rows - 1) {
                if (app->scrollable && app->scroll_y_offset + app->config.max_rows - 1 !=
                                           array_len(app->dir_entries) - 1) {
                  app->scroll_y_offset = fmin(array_len(app->dir_entries) - 1,
                                              app->scroll_y_offset + 1);
                }
              } else {
                app->dir_index++;
              }
              app->update_rendering = true;
            }
            break;
          }
          case 'C': {
            size_t len = array_len(app->prev_dirs);
            if (len > 0) {
              DirEntry entry = app->prev_dirs[len - 1];
              array_remove(app->prev_dirs, len - 1);
              app_open_dir(app, &entry);
            }
            app->update_rendering = true;
            break;
          }
          case 'D': {
            char *slash = strrchr(app->wd, '/');
            if (slash != NULL) {
              int index = app->wd - slash;
              if (index != 0) {
                app_open_dir(app, &PREV_DIR);
              }
            }
            break;
          }
          }
        }
        break;
      }
      // ENTER
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
        goto end;
      }
      default: {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ':' ||
            c == '.' || c == '?' || (c >= '0' && c <= '9')) {
          strcat(app->input, str_fmt("%c", c));
        } else if (c == ' ') {
          strcat(app->input, " ");
        } else if (c == 127 || c == 8) {
          size_t len = strlen(app->input);
          app->input[len - 1] = '\0';
        }
        app->update_rendering = true;
        break;
      }
      }
    }
  }
end:
  app_exit(app);
}