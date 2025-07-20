#include "../include/app.h"
#include "../include/array.h"
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

#define TEXT_EDITOR "micro"

int main(int argc, char **argv) {
  alloc_init();

  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  APP = (App) {.dir_entries = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
             .prev_dirs = array_new_capacity(DirEntry, 256, &HEAP_ALLOCATOR),
             .initial = true,
             .prev_len = 0,
             .terminal_dimensions = {.width = w.ws_col, .height = w.ws_row}};

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

  signal(SIGINT, terminal_handle_sigint);
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
            if (app->dir_index > 0) {
              app->dir_index = app->dir_index - 1;
            }
            app->prev_len = array_len(app->dir_entries);
            app->update_rendering = true;
            break;
          }
          case 'B': {
            app->dir_index =
                fmin(array_len(app->dir_entries) - 1, app->dir_index + 1);
            app->prev_len = array_len(app->dir_entries);
            app->update_rendering = true;
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
      case '\n': {
        DirEntry *entry = &app->dir_entries[app->dir_index];
        if (entry->type == DET_DIR) {
          app_open_dir(app, entry);
        } else {
          system(str_fmt(TEXT_EDITOR " %s/%s", app->wd, entry->name));
          app->update_rendering = true;
        }
        break;
      }
      case 'q':
        goto end;
      default:
        app->update_rendering = true;
        break;
      }
    }
  }
end:
  terminal_disable_raw_mode();
}