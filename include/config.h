#pragma once

#include <stddef.h>

typedef struct {
  char *text_editor;
  int max_rows;
  bool show_hidden_dirs;
} AppConfig;

void app_config_read(AppConfig *config, const char *config_file_buf);

void app_config_write(const AppConfig *config, char *config_file_buf, size_t buf_capacity);
