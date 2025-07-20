#include "../include/app.h"
#include "../include/array.h"
#include "../include/term.h"
#include <string.h>

App APP;

void app_find_entries(App *app) {
  struct dirent *entry;
  printfn("wd: %s", app->wd);
  DIR *dp = opendir(app->wd);

  if (dp == NULL) {
    perror("opendir");
    exit(1);
  }

  while ((entry = readdir(dp)) != NULL) {
    array_add(app->dir_entries, dir_entry_new(app->wd, entry));
  }

  closedir(dp);
}

void app_open_dir(App *app, const DirEntry *entry) {
  if (strcmp(entry->name, ".") == 0) {
    app->update_rendering = true;
    return;
  }

  app->prev_len = array_len(app->dir_entries);
  array_clear(app->dir_entries);

  if (strcmp(entry->name, "..") == 0) {
    char *last_slash = strrchr(app->wd, '/');
    if (last_slash != NULL) {
      DirEntry entry = {.type = DET_FILE, .ext = NULL};
      strcpy(entry.name, app->wd);
      array_add(app->prev_dirs, entry);
      *last_slash = '\0';
    }
  } else if (entry->name[0] == '/') {
    strcpy(app->wd, entry->name);
  } else {
    char temp[1024];
    strcpy(temp, app->wd);
    snprintf(app->wd, 1024, "%s/%s", temp, entry->name);
  }

  app_find_entries(app);

  terminal_clear();
  app->update_rendering = true;
  app->dir_index = 0;
}

void app_render(App *app) {
  size_t len = array_len(app->dir_entries);
  terminal_clear();

  for (size_t i = 0; i < len; i++) {
    dir_entry_render(&app->dir_entries[i], app->dir_index == i);
  }
  printf("%s>", app->wd);
  app->update_rendering = false;
}