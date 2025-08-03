#include "../include/app.h"
#include "../include/array.h"
#include "../include/term.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

App APP;

void app_find_entries(App *app) {
  array_clear(app->dir_entries);

  struct dirent *entry;
  printfn("wd: %s", app->wd);
  DIR *dp = opendir(app->wd);

  if (dp == NULL) {
    perror("opendir");
    exit(1);
  }

  while ((entry = readdir(dp)) != NULL) {
    char *path = str_fmt("%s/%s", app->wd, entry->d_name);
    if (!app->config.show_hidden_dirs && entry->d_name[0] == '.' &&
        is_dir(path)) {
      continue;
    }
    array_add(app->dir_entries, dir_entry_new(path));
  }

  DirEntry *new_dir_entries = sort_dirs_alphabetic(app->dir_entries);

  array_clear(app->dir_entries);
  if (new_dir_entries != NULL) {
    for (int i = 0; i < array_len(new_dir_entries); i++) {
      array_add(app->dir_entries, new_dir_entries[i]);
    }

    array_free(new_dir_entries);
  }

  closedir(dp);

  app->scroll_y_offset = 0;
  app->scrollable = array_len(app->dir_entries) > app->config.max_rows;
}

void app_open_dir(App *app, const DirEntry *entry) {
  if (strcmp(entry->name, ".") == 0) {
    app->update_rendering = true;
    return;
  }

  array_clear(app->dir_entries);

  if (strcmp(entry->name, "..") == 0) {
    char *last_slash = strrchr(app->wd, '/');
    if (last_slash != NULL) {
      DirEntry entry = dir_entry_new(app->wd);
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

static AppConfig DEFAULT_CONFIG = {
    .text_editor = "micro", .max_rows = 9, .show_hidden_dirs = true};

static constexpr char FLIES_DIR[] = ".flies";
static constexpr char FLIES_CONFIG_FILE[] = "flies.cfg";

AppConfig app_config_load(void) {
  const char *home_dir = getenv("HOME");
  // /home/.../.flies/ + \0
  size_t flies_dir_path_len = strlen(home_dir) + 1 + sizeof(FLIES_DIR) + 1 + 1;
  char flies_dir_path[flies_dir_path_len];
  snprintf(flies_dir_path, flies_dir_path_len, "%s/%s/", home_dir, FLIES_DIR);
  char cfg_file_path[flies_dir_path_len + sizeof(FLIES_CONFIG_FILE)];
  snprintf(cfg_file_path, flies_dir_path_len + sizeof(FLIES_CONFIG_FILE),
           "%s/%s", flies_dir_path, FLIES_CONFIG_FILE);
  // Check if .flies directory exists
  if (!is_dir(flies_dir_path)) {
    dir_create(flies_dir_path);
  }

  if (!file_exists(cfg_file_path)) {
    char buf[128] = {0};
    app_config_write(&DEFAULT_CONFIG, buf, 128);
    FILE *file = fopen(cfg_file_path, "w");
    if (file) {
      fprintf(file, "%s", buf);
      fclose(file);
      printfn("Wrote %s to file %s", buf, cfg_file_path);
    } else {
      perror("fopen");
      exit(1);
    }
  }

  char *config_file = read_file_to_string(cfg_file_path);

  AppConfig config;
  app_config_read(&config, config_file);
  printfn("Text editor: %s", config.text_editor);
  printfn("Rows: %d", config.max_rows);
  printfn("Show hidden dirs: %s", config.show_hidden_dirs ? "true" : "false");

  return config;
}

static void app_reload(App *app) { app->config = app_config_load(); }

void app_render(App *app) {
  size_t start = 0;
  size_t len = array_len(app->dir_entries);
  terminal_clear();

  printfn("\xE2\x84\xB9\xEF\xB8\x8F  %s", app->info_msg);

  if (len == 0) {
    printfn("<EMPTY>");
  }

  // if (app->scrollable) {
  //   start = app->scroll_y_offset;
  //   len = fmin(app->scroll_y_offset + app->config.max_rows, len);
  // }

  for (size_t i = 0; i < app->config.max_rows; i++) {
    if (app->scroll_y_offset + i < array_len(app->dir_entries)) {
      dir_entry_render(&app->dir_entries[app->scroll_y_offset + i],
                       app->dir_index == i);
    } else {
      printfn("");
    }
  }

  if (app->scrollable && app->scroll_y_offset + app->config.max_rows - 1 !=
                             array_len(app->dir_entries) - 1) {
    printfn("...");
  } else {
    printfn("");
  }

  printf("%s> %s", app->wd, app->input);
  app->update_rendering = false;
}

static constexpr char NF_CMD[] = ":nf";
static constexpr char ND_CMD[] = ":nd";
static constexpr char RN_CMD[] = ":rn";

void app_run_cmd(App *app) {
  size_t nf_cmd_len = sizeof(NF_CMD);
  size_t nd_cmd_len = sizeof(ND_CMD);
  size_t rn_cmd_len = sizeof(RN_CMD);
  if (str_eq(app->input, ":q")) {
    app_exit(app);
  } else if (str_eq(app->input, ":c")) {
    app_copy_file(app, app_hovered_entry(app));
  } else if (str_eq(app->input, ":p")) {
    app_paste_file(app, app->wd);
  } else if (str_eq(app->input, ":d")) {
    app_delete_file(app, app_hovered_entry(app));
  } else if (str_eq(app->input, ":h")) {
    app_set_debug_msg(app, "[NYI] Help command");
  } else if (str_eq(app->input, ":r")) {
    app_set_debug_msg(app, "Reloaded Config");
    app_reload(app);
  } else if (strncmp(app->input, RN_CMD, rn_cmd_len - 1) == 0) {
    app_set_debug_msg(app, "Renamed file");
    char new_name[512];
    char *path = app->input + rn_cmd_len;
    strcpy(new_name, app_hovered_entry(app)->path);
    char *slash = strrchr(new_name, '/') + 1;
    strcpy(slash, path);
    rename(app_hovered_entry(app)->path, new_name);
    app_refresh(app);
  } else if (strncmp(app->input, NF_CMD, nf_cmd_len - 1) == 0) {
    app_set_debug_msg(app, "Created new file");
    DirEntry entry =
        dir_entry_new(str_fmt("%s/%s", app->wd, app->input + nf_cmd_len));
    app_new_file(app, entry);
    app_open_entry(app, &entry);
  } else if (strncmp(app->input, ND_CMD, nd_cmd_len - 1) == 0) {
    app_set_debug_msg(app, "Created new directory");
    char *path = str_fmt("%s/%s", app->wd, app->input + nd_cmd_len);
    DirEntry entry = dir_entry_new(path);
    entry.type = DET_DIR;
    app_new_dir(app, entry);
    app_open_entry(app, &entry);
  } else {
    app_set_debug_msg(app, str_fmt("Unknown command: %s", app->input));
  }
  app->input[0] = '\0';
}

void app_set_debug_msg(App *app, const char *debug_message) {
  strcpy(app->info_msg, debug_message);
}

void app_refresh(App *app) {
  int scroll_y_offset = app->scroll_y_offset;
  app_find_entries(app);
  app->scroll_y_offset = scroll_y_offset;
  app->update_rendering = true;
}

void app_copy_file(App *app, const DirEntry *entry) {
  FILE *pipe = popen("wl-copy --type text/uri-list", "w");
  if (pipe) {
    fputs(str_fmt("file://%s/%s", app->wd, entry->name), pipe);
    pclose(pipe);

    app_set_debug_msg(app, app->cut ? "Cut file" : "Copied file");
  } else {
    perror("wl-copy");
  }
}

void app_paste_file(App *app, const char *dir) {
  FILE *pipe = popen("wl-paste --type text/uri-list", "r");
  if (pipe) {
    // TODO: Use file_read_to_string helper function
    char buf[1024];
    size_t len = fread(buf, sizeof(char), 1024, pipe);
    buf[len] = '\0';
    pclose(pipe);

    char *newline = strchr(buf, '\n');
    if (newline)
      *newline = '\0';

    char *prefix = "file://";
    if (strncmp(buf, prefix, strlen(prefix)) != 0) {
      app_set_debug_msg(app, "Clipboard content is not a file");
      return;
    }
    char *pasted_file_path = buf + strlen(prefix) - 1;
    char *pasted_file_content = read_file_to_string(pasted_file_path);

    if (pasted_file_content != NULL) {
      char *file_name = strrchr(pasted_file_path, '/') + 1;
      FILE *pasted_file = fopen(str_fmt("%s/%s", dir, file_name), "w");
      fprintf(pasted_file, "%s", pasted_file_content);
      fclose(pasted_file);
      free(pasted_file_content);
      app_set_debug_msg(app, "Pasted file");

      if (app->cut) {
        remove(pasted_file_path);
        app->cut = false;
        history_push(&app->action_history,
                     action_move(str_cpy_heap(pasted_file_path),
                                 str_fmt_heap("%s/%s", dir, file_name)));
      } else {
        history_push(&app->action_history,
                     action_create(str_fmt_heap("%s/%s", dir, file_name)));
      }

      app_refresh(app);
    }
  } else {
    perror("wl-paste");
  }
}

void app_delete_file(App *app, const DirEntry *entry) {
  char *file_content =
      read_file_to_string(str_fmt("%s/%s", app->wd, entry->name));
  char *old_path = str_fmt_heap("%s/%s", app->wd, entry->name);
  history_push(&app->action_history, action_delete(old_path, file_content));
  remove(str_fmt("%s/%s", app->wd, entry->name));
  app_refresh(app);
}

void app_undo(App *app) {
  Action action = history_pop(&app->action_history);
  char *action_name;
  switch (action.type) {
  case ACTION_MOVE: {
    const char *old_path = action.var.move_action.old_path;
    const char *new_path = action.var.move_action.new_path;
    char *file_content = read_file_to_string(new_path);
    FILE *f = fopen(old_path, "w");
    if (f) {
      fprintf(f, "%s", file_content);
      fclose(f);
      free(file_content);
      remove(new_path);
      app_refresh(app);
      app_set_debug_msg(
          app, str_fmt("Undid action MOVE from %s to %s", old_path, new_path));
    } else {
      perror("fopen");
    }
    break;
  }
  case ACTION_CREATE: {
    const char *path = action.var.create_action.path;
    remove(path);
    app_refresh(app);
    app_set_debug_msg(app, str_fmt("Undid action CREATE at %s", path));
    break;
  }
  case ACTION_DELETE: {
    const char *path = action.var.delete_action.old_path;
    char *file_content = action.var.delete_action.file_content;
    FILE *file = fopen(path, "w");
    if (file) {
      fprintf(file, "%s", file_content);
      fclose(file);
      free(file_content);
      app_set_debug_msg(app, str_fmt("Undid action DELETE at %s", path));
    } else {
      perror("fopen");
      exit(1);
    }
    app_refresh(app);
    break;
  }
  }
  // app_set_debug_msg(app, str_fmt("Undo action: %d", action.type));
}

void app_new_file(App *app, DirEntry entry) {
  FILE *f = fopen(str_fmt("%s/%s", app->wd, entry.name), "w");
  if (f) {
    fclose(f);
  } else {
    perror("fopen");
  }
  app_refresh(app);
}

void app_new_dir(App *app, DirEntry entry) { mkdir(entry.path, 0777); }

void app_exit(App *app) {
  terminal_disable_raw_mode();
  exit(0);
}

void app_open_entry(App *app, const DirEntry *entry) {
  if (entry->type == DET_DIR) {
    app_open_dir(app, entry);
  } else {
    system(str_fmt("%s %s/%s", app->config.text_editor, app->wd, entry->name));
    app->update_rendering = true;
  }
}

DirEntry *app_hovered_entry(const App *app) {
  return &app->dir_entries[app->scroll_y_offset + app->dir_index];
}
