#include "../include/app.h"
#include "../include/array.h"
#include "../include/term.h"
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
    array_add(app->dir_entries, dir_entry_new(path));
  }

  DirEntry *new_dir_entries = sort_dirs_alphabetic(app->dir_entries);

  array_clear(app->dir_entries);

  for (int i = 0; i < array_len(new_dir_entries); i++) {
    array_add(app->dir_entries, new_dir_entries[i]);
  }

  array_free(new_dir_entries);

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

void app_render(App *app) {
  size_t len = array_len(app->dir_entries);
  terminal_clear();

#ifdef DEBUG_BUILD
  printf("DEBUG INFO: %s\n", app->debug_message);
#endif

  for (size_t i = 0; i < len; i++) {
    dir_entry_render(&app->dir_entries[i], app->dir_index == i);
  }
  printf("%s> %s", app->wd, app->input);
  app->update_rendering = false;
}

constexpr char NF_CMD[] = ":nf";
constexpr char ND_CMD[] = ":nd";

void app_run_cmd(App *app) {
  size_t nf_cmd_len = sizeof(NF_CMD);
  size_t nd_cmd_len = sizeof(ND_CMD);
  if (str_eq(app->input, ":q")) {
    app_exit(app);
  } else if (str_eq(app->input, ":c")) {
    app_copy_file(app, &app->dir_entries[app->dir_index]);
  } else if (str_eq(app->input, ":p")) {
    app_paste_file(app, app->wd);
  } else if (str_eq(app->input, ":d")) {
    app_delete_file(app, &app->dir_entries[app->dir_index]);
  } else if (strncmp(app->input, NF_CMD, nf_cmd_len - 1) == 0) {
    DirEntry entry =
        dir_entry_new(str_fmt("%s/%s", app->wd, app->input + nf_cmd_len));
    app_new_file(app, entry);
    app_open_entry(app, &entry);
  } else if (strncmp(app->input, ND_CMD, nd_cmd_len - 1) == 0) {
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
  strcpy(app->debug_message, debug_message);
}

void app_refresh(App *app) {
  app_find_entries(app);
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
    system(str_fmt(TEXT_EDITOR " %s/%s", app->wd, entry->name));
    app->update_rendering = true;
  }
}
