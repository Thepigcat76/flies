#include "../include/alloc.h"
#include "../include/array.h"
#include "../include/shared.h"
#include <stdlib.h>

typedef struct {
  const char *key;
  struct {
    enum {
      CONFIG_ENTRY_STRING,
      CONFIG_ENTRY_INT,
      CONFIG_ENTRY_BOOL,
    } type;
    union {
      const char *entry_string;
      int entry_int;
      bool entry_bool;
    } var;
  } val;
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
    } else if (*lexer.cur_char == '\n') {
      lexer.cur_char++;
      token = (ConfigToken){.type = TOKEN_EOL};
    } else {
      PANIC("Invalid char: %c", *lexer.cur_char);
    }
    array_add(tokens, token);
    //tok_print(&token);
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

static void config_entry_print(const ConfigEntry *entry) {
  char buf[256] = {'\0'};
  strcat(buf, entry->key);
  strcat(buf, " = ");
  switch (entry->val.type) {
  case CONFIG_ENTRY_STRING: {
    strcat(buf, entry->val.var.entry_string);
    break;
  }
  case CONFIG_ENTRY_BOOL: {
    strcat(buf, entry->val.var.entry_bool ? "true" : "false");
    break;
  }
  case CONFIG_ENTRY_INT: {
    char int_buf[128];
    sprintf(int_buf, "%d", entry->val.var.entry_int);
    strcat(buf, int_buf);
    break;
  }
  }
  puts(buf);
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
      case TOKEN_EOL: {
        parser.cur_token++;
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
      break;
    }
    default: {
      PANIC("Expected IDENT at beginning of line, received %s instead",
            tok_to_string(parser.cur_token));
      break;
    }
    }
    //config_entry_print(&entry);
    array_add(config->entries, entry);
  }
end:
}

static Config config_read(const char *config_file_content,
                          Allocator *allocator) {
  Config config = {.entries = array_new_capacity(ConfigEntry, 32, allocator)};

  ConfigToken *tokens = config_tokenize(config_file_content, allocator);
  config_parse(&config, tokens);

  array_free(tokens);

  //for (int i = 0; i < array_len(config.entries); i++) {
  //  config_entry_print(&config.entries[i]);
  //}

  // for (int i = 0; i < array_len(tokens); i++) {
  //   ConfigToken *tok = &tokens[i];
  //   tok_print(tok);
  // }

  return config;
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
                              const char *val) {}

static void config_add_int(const Config *config, const char *key, int val) {}

static void config_add_bool(const Config *config, const char *key, bool val) {}

// -- APP CONFIG --

typedef struct {
  const char *text_editor;
  int max_rows;
  bool show_hidden_dirs;
} AppConfig;

void app_config_read(AppConfig *config, const char *config_file) {
  Config raw_config = config_read(config_file, &HEAP_ALLOCATOR);

  config->text_editor = config_get_string(&raw_config, "text-editor", "micro");
  config->max_rows = config_get_int(&raw_config, "max-rows", 9);
  config->show_hidden_dirs =
      config_get_bool(&raw_config, "show-hidden-dirs", true);
}

void app_config_write(const AppConfig *config, char *config_file) {}
