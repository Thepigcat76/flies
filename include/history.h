#pragma once

typedef struct {
  enum {
    ACTION_MOVE,
    ACTION_CREATE,
    ACTION_DELETE,
  } type;
  union {
    struct {
      const char *new_path;
      const char *old_path;
    } move_action;
    struct {
      const char *path;
    } create_action;
    struct {
      const char *old_path;
      char *file_content;
    } delete_action;
  } var;
} Action;

typedef struct {
  Action *actions;
} History;

History history_new(void);

void history_push(History *history, Action action);

Action history_pop(History *history);

Action action_move(const char *new_path, const char *old_path);

Action action_create(const char *path);

Action action_delete(const char *old_path, char *file_content);
