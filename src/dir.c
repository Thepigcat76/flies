#include "../include/dir.h"
#include <string.h>

DirEntry PREV_DIR = {.name = "..", .type = DET_FILE, .ext = NULL};

DirEntry dir_entry_new(const char *path, struct dirent *entry) {
  DirEntry ent = {.type = is_dir(str_fmt("%s/%s", path, entry->d_name))
                              ? DET_DIR
                              : DET_FILE};
  strcpy(ent.name, entry->d_name);
  if (ent.type == DET_FILE) {
    char *dot = strrchr(entry->d_name, '.');
    if (dot != NULL) {
      char *ext = dot + 1;
      if (ext != NULL) {
        ent.ext = ext;
      }
    }
  }
  return ent;
}

void dir_entry_render(const DirEntry *dir_entry, bool selected) {
  printfn("%s%s %s%s", selected ? "\e[47m" : "",
          dir_entry->type == DET_DIR ? "\xF0\x9F\x93\x81" : "\xF0\x9F\x93\x84",
          dir_entry->name, selected ? "\e[0m" : "");
}