#include "../include/term.h"
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

volatile sig_atomic_t terminal_resized = 0;

void terminal_init() {
  //tcgetattr(STDIN_FILENO, &term->prev_term);
  //term->cur_term = term->prev_term;
}

void terminal_cursor_up(size_t amount) { printf("\x1b[%zuA", amount); }

void terminal_cursor_down(size_t amount) { printf("\x1b[%zuB", amount); }

void terminal_clear() {
  clear();
  refresh();
}

void terminal_enable_raw_mode() {
  // term->cur_term.c_lflag &= ~(ECHO | ICANON | ISIG); // raw mode
  // term->cur_term.c_iflag &= ~(IXON);
  // tcsetattr(STDIN_FILENO, TCSANOW, &term->cur_term);
  raw();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  //nodelay(stdscr, TRUE);
  scrollok(stdscr, FALSE);
}

void terminal_disable_raw_mode() {
  // Fully restore previous termios (not modify in place)
  //tcsetattr(STDIN_FILENO, TCSANOW, &term->prev_term);

  // Re-enable cursor and clear screen
  clear();
  attrset(A_NORMAL);
  standend();
  curs_set(1);
  echo();
  refresh();
}

void terminal_clear_last_lines(size_t n) {
  int y, x;
  getyx(stdscr, y, x); // Get current cursor position

  for (size_t i = 0; i < n; i++) {
    if (y > 0) {
      y--;        // Move up one line
      move(y, 0); // Move to start of that line
      clrtoeol(); // Clear the entire line
    }
  }

  move(y, 0); // Leave cursor at top cleared line
  refresh();
}

void terminal_handle_sigwinch(int sig) {
  (void)sig;
  terminal_resized = 1;
}

void terminal_handle_sigsegv(int sig) {
  (void)sig;
  terminal_disable_raw_mode();

  fprintf(stderr, "Encountered sigsegv\n");

  exit(1);
}