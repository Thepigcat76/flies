#pragma once

#include "lilc/hashmap.h"
#include <stddef.h>

typedef struct {
  bool show_hidden_dirs;
  Hashmap(char *, char *) open_file;
} AppConfig;

AppConfig app_config_parse(const char *config_path, Hashmap(char **, char **) *interps);

const char *app_config_open_file(const char *file_path,
                                 const char *config_path);

void app_config_read(AppConfig *config, const char *config_file_buf);

void app_config_write(const AppConfig *config, char *config_file_buf,
                      size_t buf_capacity);
