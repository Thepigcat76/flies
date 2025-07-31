#pragma once

typedef struct {
  char *text_editor;
  int max_rows;
  bool show_hidden_dirs;
} AppConfig;

void app_config_read(AppConfig *config, const char *config_file);

void app_config_write(const AppConfig *config, char *config_file);
