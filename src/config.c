#include "../include/config.h"
#include "../include/app.h"
#include "../include/shared.h"
#include "../include/toml.h"
#include "lilc/alloc.h"
#include "lilc/array.h"
#include "lilc/eq.h"
#include "lilc/hash.h"
#include "lilc/hashmap.h"
#include "lilc/log.h"
#include "lilc/str.h"
#include <regex.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static AppConfig cur_config;

struct raw_config {
  char *text_editor;
  bool show_hidden_dirs;
};

char *str_proc(const char *str, size_t str_len,
               Hashmap(char **, char **) * interps, const Allocator *alloc) {
  dyn_string_t proc_str = {0};
  dyn_string_init(&proc_str);

  size_t i = 0;
  while (i < str_len) {
    if (strncmp(str + i, "${", 2) == 0) {
      i += 2;

      dyn_string_t interp_key = {0};
      dyn_string_init(&interp_key);
      while (str[i] != '\0' && str[i] != '}') {
        dyn_string_add_char(&interp_key, str[i]);
        ++i;
      }

      char **interp_val = hashmap_value(interps, &interp_key.string);
      if (interp_val == NULL) {
        log_warn("Interpolation value not found for key: %s",
                 interp_key.string);
      } else {
        dyn_string_add_str(&proc_str, *interp_val);
      }
    } else {
      dyn_string_add_char(&proc_str, str[i]);
    }
    ++i;
  }
  dyn_string_add_char(&proc_str, '\0');
  return proc_str.string;
}

const char *app_config_open_file(const char *file_path,
                                 const char *config_path) {
  toml_result_t result = toml_parse_file_ex(config_path);
  if (!result.ok) {
    log_error("%s", result.errmsg);
    exit(1);
  }

  Hashmap(char **, char **) interps = hashmap_new(
      char **, char **, &HEAP_ALLOCATOR, str_ptrv_hash, str_ptrv_eq, NULL);

  char *key = "filepath";
  hashmap_insert(&interps, &key, &file_path);

  app_config_parse(config_path, &interps);

  dyn_string_t default_open_file_cmd = {0};
  dyn_string_init(&default_open_file_cmd);
  toml_datum_t open_file_table = toml_seek(result.toptab, "open-file");
  // app_set_debug_msg(&APP, "B");
  app_set_debug_msg(&APP, str_fmt("table size: %d for %s",
                                  open_file_table.u.tab.size, config_path));
  for (size_t i = 0; i < (size_t)open_file_table.u.tab.size; i++) {
    if (strv_eq(open_file_table.u.tab.key[i], "default")) {
      dyn_string_copy_str(&default_open_file_cmd,
                          str_proc(open_file_table.u.tab.value[i].u.str.ptr,
                                   open_file_table.u.tab.value[i].u.str.len,
                                   &interps, &HEAP_ALLOCATOR));
      continue;
    }

    const char *pattern = open_file_table.u.tab.key[i];

    regex_t re;

    if (regcomp(&re, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
      fprintf(stderr, "Failed to compile regex\n");
      exit(1);
    }

    bool matches = regexec(&re, file_path, 0, NULL, 0) == 0;
    app_set_debug_msg(&APP,
                      str_fmt("Pattern matches: %s, for pattern: %s, path: %s",
                              matches ? "true" : "false", pattern, file_path));
    regfree(&re);
    toml_datum_t val = open_file_table.u.tab.value[i];
    if (matches && val.type == TOML_STRING) {
      return str_proc(val.u.str.ptr, val.u.str.len, &interps, &HEAP_ALLOCATOR);
    }
  }

  return default_open_file_cmd.string;
}

AppConfig app_config_parse(const char *config_path,
                           Hashmap(char **, char **) * interps) {
  AppConfig conf;

  toml_result_t result = toml_parse_file_ex(config_path);

  if (!result.ok) {
    log_error("%s", result.errmsg);
    exit(1);
  }

  toml_datum_t conf_table = result.toptab;
  const char **conf_keys = conf_table.u.tab.key;
  int32_t *conf_keys_len = conf_table.u.tab.len;
  toml_datum_t *conf_values = conf_table.u.tab.value;
  size_t conf_len = conf_table.u.tab.size;

  for (size_t i = 0; i < conf_len; i++) {
    const char *conf_key = conf_keys[i];
    size_t conf_key_len = conf_keys_len[i];
    const toml_datum_t conf_val = conf_values[i];
    char *proc_conf_val;
    if (conf_val.type == TOML_STRING) {
      proc_conf_val = str_proc(conf_val.u.str.ptr, conf_val.u.str.len, interps,
                               &HEAP_ALLOCATOR);
    }

    if (strncmp(conf_key, "show-hidden-dirs", conf_key_len) == 0) {
      if (conf_val.type != TOML_BOOLEAN) {
        log_warn("Missing or invalid 'show-hidden-dirs' property in config");
      } else {
        conf.show_hidden_dirs = conf_val.u.boolean;
      }
    } else if (strncmp(conf_key, "open-file", conf_key_len) == 0) {
    }

    if (conf_val.type == TOML_STRING) {
      char *interp_key = HEAP_ALLOCATOR.alloc(conf_key_len + 1);
      strncpy(interp_key, conf_key, conf_key_len);
      interp_key[conf_key_len] = '\0';

      hashmap_insert(interps, &interp_key, &proc_conf_val);
      log_debug("Added interp: %s => %s", interp_key, proc_conf_val);
    }
  }

  toml_free(result);

  return conf;
}

typedef struct {
  enum {
    CONFIG_ENTRY_STRING,
    CONFIG_ENTRY_INT,
    CONFIG_ENTRY_BOOL,
    CONFIG_ENTRY_MAP,
  } type;
  union {
    const char *entry_string;
    int entry_int;
    bool entry_bool;
  } var;
} ConfigValue;

typedef struct {
  const char *key;
  ConfigValue val;
} ConfigEntry;

typedef struct {
  ConfigEntry *entries;
} Config;

typedef struct {
  enum {
    TOKEN_IDENT,
    TOKEN_ASSIGN,
    TOKEN_STRING,
    TOKEN_INT,
    TOKEN_BOOL,
    TOKEN_LCURLY,
    TOKEN_RCURLY,
    TOKEN_EOL,
    TOKEN_EOF,
  } type;
  union {
    const char *tok_ident;
    const char *tok_str;
    const char *tok_int;
    const char *tok_bool;
  } var;
} ConfigToken;

#define LEXER_VALID_IDENT_CHAR(c)                                              \
  ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-')

#define LEXER_VALID_INT_CHAR(c) (c >= '0' && c <= '9')

static void tok_print(const ConfigToken *tok) {
  switch (tok->type) {
  case TOKEN_IDENT: {
    printfn("IDENT: %s", tok->var.tok_ident);
    break;
  }
  case TOKEN_STRING: {
    printfn("STRING: \"%s\"", tok->var.tok_str);
    break;
  }
  case TOKEN_ASSIGN: {
    printfn("ASSIGN");
    break;
  }
  case TOKEN_LCURLY: {
    printfn("LCURLY");
    break;
  }
  case TOKEN_RCURLY: {
    printfn("RCURLY");
    break;
  }
  case TOKEN_BOOL: {
    printfn("BOOL: %s", tok->var.tok_bool);
    break;
  }
  case TOKEN_INT: {
    printfn("INT: %s", tok->var.tok_int);
    break;
  }
  case TOKEN_EOL: {
    printfn("EOL");
    break;
  }
  case TOKEN_EOF: {
    printfn("EOF");
    break;
  }
  }
}

static ConfigToken *config_tokenize(const char *config_file_content,
                                    Allocator *allocator) {
  struct {
    char const *cur_char;
  } lexer = {.cur_char = config_file_content};
  ConfigToken *tokens = array_new_capacity(ConfigToken, 128, allocator);
  char *tok_literal = array_new_capacity(char, 64, allocator);
  while (*lexer.cur_char != '\0') {
    array_clear(tok_literal);
    ConfigToken token;

    while (*lexer.cur_char == ' ') {
      lexer.cur_char++;
    }

    if (LEXER_VALID_IDENT_CHAR(*lexer.cur_char)) {
      while (LEXER_VALID_IDENT_CHAR(*lexer.cur_char) ||
             LEXER_VALID_INT_CHAR(*lexer.cur_char)) {
        array_add(tok_literal, *lexer.cur_char);
        lexer.cur_char++;
      }
      char *ident = malloc((array_len(tok_literal) + 1) * sizeof(char));
      strncpy(ident, tok_literal, array_len(tok_literal));
      ident[array_len(tok_literal)] = '\0';
      if (str_eq(ident, "true") || str_eq(ident, "false")) {
        token = (ConfigToken){.type = TOKEN_BOOL, .var = {.tok_bool = ident}};
      } else {
        token = (ConfigToken){.type = TOKEN_IDENT, .var = {.tok_ident = ident}};
      }
    } else if (LEXER_VALID_INT_CHAR(*lexer.cur_char)) {
      while (LEXER_VALID_INT_CHAR(*lexer.cur_char)) {
        array_add(tok_literal, *lexer.cur_char);
        lexer.cur_char++;
      }
      char *integer = malloc((array_len(tok_literal) + 1) * sizeof(char));
      strncpy(integer, tok_literal, array_len(tok_literal));
      integer[array_len(tok_literal)] = '\0';
      token = (ConfigToken){.type = TOKEN_INT, .var = {.tok_int = integer}};
    } else if (*lexer.cur_char == '"') {
      lexer.cur_char++;
      while (*(lexer.cur_char + 1) != '"') {
        array_add(tok_literal, *lexer.cur_char);
        lexer.cur_char++;
      }
      array_add(tok_literal, *lexer.cur_char);
      lexer.cur_char++;
      if (*lexer.cur_char == '"') {
        lexer.cur_char++; // consume closing quote
      }
      char *string = malloc((array_len(tok_literal) + 1) * sizeof(char));
      strncpy(string, tok_literal, array_len(tok_literal));
      string[array_len(tok_literal)] = '\0';
      token = (ConfigToken){.type = TOKEN_STRING, .var = {.tok_str = string}};
    } else if (*lexer.cur_char == '=') {
      lexer.cur_char++;
      token = (ConfigToken){.type = TOKEN_ASSIGN};
    } else if (*lexer.cur_char == '{') {
      lexer.cur_char++;
      token = (ConfigToken){.type = TOKEN_LCURLY};
    } else if (*lexer.cur_char == '}') {
      lexer.cur_char++;
      token = (ConfigToken){.type = TOKEN_RCURLY};
    } else if (*lexer.cur_char == '\n') {
      lexer.cur_char++;
      token = (ConfigToken){.type = TOKEN_EOL};
    } else {
      PANIC("Invalid char: %c", *lexer.cur_char);
    }
    array_add(tokens, token);
    // tok_print(&token);
  }
  array_add(tokens, (ConfigToken){.type = TOKEN_EOF});
  array_free(tok_literal);

  return tokens;
}

static bool tok_expect(ConfigToken const *cur_tok, int expected_tok) {
  return cur_tok->type == expected_tok;
}

static char *tok_to_string(const ConfigToken *tok) {
  switch (tok->type) {
  case TOKEN_IDENT:
    return "IDENT";
  case TOKEN_ASSIGN:
    return "ASSIGN";
  case TOKEN_LCURLY:
    return "LCURLY";
  case TOKEN_RCURLY:
    return "RCURLY";
  case TOKEN_STRING:
    return "STRING";
  case TOKEN_INT:
    return "INT";
  case TOKEN_BOOL:
    return "BOOL";
  case TOKEN_EOL:
    return "EOL";
  case TOKEN_EOF:
    return "EOF";
  }
}

static void config_entry_print(const ConfigEntry *entry, char *buf,
                               size_t buf_capacity) {
  strncat(buf, entry->key, buf_capacity);
  strncat(buf, " = ", buf_capacity);
  switch (entry->val.type) {
  case CONFIG_ENTRY_STRING: {
    char string_buf[128];
    snprintf(string_buf, 128, "\"%s\"", entry->val.var.entry_string);
    strncat(buf, string_buf, buf_capacity);
    break;
  }
  case CONFIG_ENTRY_BOOL: {
    strncat(buf, entry->val.var.entry_bool ? "true" : "false", buf_capacity);
    break;
  }
  case CONFIG_ENTRY_INT: {
    char int_buf[128];
    snprintf(int_buf, 128, "%d", entry->val.var.entry_int);
    strncat(buf, int_buf, buf_capacity);
    break;
  }
  }
}

#define PEEK_TOK(parser_ptr) ((parser_ptr)->cur_token + 1)->type

#define SKIP_EOLS(parser_ptr)                                                  \
  while ((parser_ptr)->cur_token->type == TOKEN_EOL) {                         \
    (parser_ptr)->cur_token++;                                                 \
  }

static void config_parse(Config *config, const ConfigToken *tokens) {
  struct {
    ConfigToken const *cur_token;
  } parser = {.cur_token = tokens};

  while (parser.cur_token->type != TOKEN_EOF) {
    ConfigEntry entry;
    switch (parser.cur_token->type) {
    case TOKEN_IDENT: {
      const char *key = parser.cur_token->var.tok_ident;
      printfn("Key: %s", key);
      if (tok_expect(parser.cur_token + 1, TOKEN_ASSIGN)) {
        parser.cur_token += 2;
      } else {
        PANIC("Expected ASSIGN after IDENT: %s, received %s instead", entry.key,
              tok_to_string(parser.cur_token - 2));
      }
      switch (parser.cur_token->type) {
      case TOKEN_STRING: {
        entry = (ConfigEntry){
            .key = key,
            .val = {.type = CONFIG_ENTRY_STRING,
                    .var = {.entry_string = parser.cur_token->var.tok_str}}};
        break;
      }
      case TOKEN_INT: {
        entry = (ConfigEntry){
            .key = key,
            .val = {.type = CONFIG_ENTRY_INT,
                    .var = {.entry_int = atoi(parser.cur_token->var.tok_int)}}};
        break;
      }
      case TOKEN_BOOL: {
        entry = (ConfigEntry){
            .key = key,
            .val = {.type = CONFIG_ENTRY_BOOL,
                    .var = {.entry_bool =
                                str_eq(parser.cur_token->var.tok_bool, "true")
                                    ? true
                                    : false}}};
        break;
      }
      case TOKEN_LCURLY: {
        SKIP_EOLS(&parser);
        while (parser.cur_token->type != TOKEN_RCURLY) {
          SKIP_EOLS(&parser);

          Hashmap(char **, char *) map =
              hashmap_new(char **, char *, &HEAP_ALLOCATOR, str_ptrv_hash,
                          str_ptrv_eq, NULL);

          dyn_string_t key = {0};
          dyn_string_init(&key);
          if (parser.cur_token->type == TOKEN_IDENT) {
            dyn_string_add_str(&key, parser.cur_token->var.tok_ident);
          } else if (parser.cur_token->type == TOKEN_STRING) {
            dyn_string_add_str(&key, parser.cur_token->var.tok_str);
          }
          parser.cur_token++;

          tok_expect(parser.cur_token, TOKEN_ASSIGN);

          parser.cur_token++;
        }
        if (parser.cur_token->type == TOKEN_RCURLY) {
          entry = (ConfigEntry){.key = key,
                                .val = {.type = CONFIG_ENTRY_MAP, .var = {}}};
        }
        break;
      }
      default: {
        PANIC("Expected config value, received %s instead",
              tok_to_string(parser.cur_token));
        break;
      }
      }
      if (tok_expect(parser.cur_token + 1, TOKEN_EOL)) {
        parser.cur_token += 2;
      } else {
        PANIC("Expected EOL after config entry, received: %s instead",
              tok_to_string(parser.cur_token));
      }
      array_add(config->entries, entry);
      break;
    }
    case TOKEN_EOL: {
      parser.cur_token++;
      break;
    }
    default: {
      PANIC("Expected IDENT at beginning of line, received %s instead",
            tok_to_string(parser.cur_token));
      break;
    }
    }
  }
}

static Config config_new(Allocator *allocator) {
  return (Config){.entries = array_new_capacity(ConfigEntry, 32, allocator)};
}

static Config config_read(const char *config_file_content,
                          Allocator *allocator) {
  Config config = config_new(allocator);

  // ConfigToken *tokens = config_tokenize(config_file_content, allocator);
  // config_parse(&config, tokens);

  // array_free(tokens);

  // for (int i = 0; i < array_len(config.entries); i++) {
  //   config_entry_print(&config.entries[i]);
  // }

  // for (int i = 0; i < array_len(tokens); i++) {
  //   ConfigToken *tok = &tokens[i];
  //   tok_print(tok);
  // }

  return config;
}

static void config_write(const Config *config, char *buf, size_t buf_capacity) {
  for (int i = 0; i < array_len(config->entries); i++) {
    ConfigEntry *entry = &config->entries[i];
    config_entry_print(entry, buf, buf_capacity);
    strncat(buf, "\n", buf_capacity);
  }
}

static const char *config_get_string(const Config *config, const char *key,
                                     const char *default_val) {
  for (int i = 0; i < array_len(config->entries); i++) {
    const char *cur_key = config->entries[i].key;
    if (str_eq(cur_key, key)) {
      if (config->entries[i].val.type == CONFIG_ENTRY_STRING) {
        return config->entries[i].val.var.entry_string;
      }
    }
  }
  return default_val;
}

static int config_get_int(const Config *config, const char *key,
                          int default_val) {
  for (int i = 0; i < array_len(config->entries); i++) {
    const char *cur_key = config->entries[i].key;
    if (str_eq(cur_key, key)) {
      if (config->entries[i].val.type == CONFIG_ENTRY_INT) {
        return config->entries[i].val.var.entry_int;
      }
    }
  }
  return default_val;
}

static bool config_get_bool(const Config *config, const char *key,
                            bool default_val) {
  for (int i = 0; i < array_len(config->entries); i++) {
    const char *cur_key = config->entries[i].key;
    if (str_eq(cur_key, key)) {
      if (config->entries[i].val.type == CONFIG_ENTRY_BOOL) {
        return config->entries[i].val.var.entry_bool;
      }
    }
  }
  return default_val;
}

static void config_add_string(const Config *config, const char *key,
                              const char *val) {
  ConfigEntry entry = (ConfigEntry){
      .key = key,
      .val = {.type = CONFIG_ENTRY_STRING, .var = {.entry_string = val}}};
  array_add(config->entries, entry);
}

static void config_add_int(const Config *config, const char *key, int val) {
  ConfigEntry entry = (ConfigEntry){
      .key = key, .val = {.type = CONFIG_ENTRY_INT, .var = {.entry_int = val}}};
  array_add(config->entries, entry);
}

static void config_add_bool(const Config *config, const char *key, bool val) {
  ConfigEntry entry = (ConfigEntry){
      .key = key,
      .val = {.type = CONFIG_ENTRY_BOOL, .var = {.entry_bool = val}}};
  array_add(config->entries, entry);
}

void app_config_read(AppConfig *config, const char *config_file) {
  Config raw_config = config_read(config_file, &HEAP_ALLOCATOR);

  // config->text_editor = config_get_string(&raw_config, "text-editor",
  // "micro");
  //  config->max_rows = config_get_int(&raw_config, "max-rows", 9);
  config->show_hidden_dirs =
      config_get_bool(&raw_config, "show-hidden-dirs", true);
}

void app_config_write(const AppConfig *config, char *config_file_buf,
                      size_t buf_capacity) {
  Config raw_config = config_new(&HEAP_ALLOCATOR);

  // config_add_string(&raw_config, "text-editor", config->text_editor);
  //  config_add_int(&raw_config, "max-rows", config->max_rows);
  config_add_bool(&raw_config, "show-hidden-dirs", config->show_hidden_dirs);

  config_write(&raw_config, config_file_buf, buf_capacity);
}
