#include "../include/history.h"
#include "../include/array.h"

History history_new() {
  return (History){.actions = array_new_capacity(Action, 128, &HEAP_ALLOCATOR)};
}

void history_push(History *history, Action action) {
  array_add(history->actions, action);
}

Action history_pop(History *history) {
  Action action = history->actions[array_len(history->actions) - 1];
  array_remove(history->actions, array_len(history->actions) - 1);
  return action;
}

// TODO: Copy all strings to heap

Action action_move(const char *old_path, const char *new_path) {
  return (Action){.type = ACTION_MOVE,
                  .var = {.move_action = {
                              .old_path = old_path,
                              .new_path = new_path,
                          }}};
}

Action action_create(const char *path) {
  return (Action){.type = ACTION_CREATE, .var = {.create_action = {.path = path}}};
}

Action action_delete(const char *old_path, const char *file_content) {
  return (Action){.type = ACTION_DELETE,
                  .var = {.delete_action = {.old_path = old_path,
                                            .file_content = file_content}}};
}
