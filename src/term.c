#include "../include/app.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

volatile sig_atomic_t terminal_resized = 0;

void terminal_cursor_up(size_t amount) { printf("\x1b[%zuA", amount); }

void terminal_cursor_down(size_t amount) { printf("\x1b[%zuB", amount); }

void terminal_clear() {
  Term *term = &APP.term;

  term->buffer[0] = '\0';
  term->cursor = 0;
}

void terminal_enable_raw_mode() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ECHO | ICANON | ISIG); // raw mode
  term.c_iflag &= ~(IXON);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void terminal_disable_raw_mode() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= ICANON | ECHO; // Restore modes
  tcsetattr(STDIN_FILENO, TCSANOW, &term);

  terminal_clear();

  printf("\033[0m");   // Reset all attributes
  printf("\033[?25h"); // Show cursor
}

void terminal_clear_last_lines(size_t n) {
  for (int i = 0; i < n; i++) {
    printf("\033[F");  // move cursor up one line
    printf("\033[2K"); // clear entire line
  }
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

void term_printf(const char *format, ...) {
  Term *term = &APP.term;

  va_list args;
  va_start(args, format);
  int n = vsnprintf(&term->buffer[term->cursor],
                    sizeof(term->buffer) - term->cursor, format, args);
  va_end(args);
  if (n > 0) {
    term->cursor += n;
  }
}

void term_printfn(const char *format, ...) {
  Term *term = &APP.term;

  va_list args;
  va_start(args, format);
  int n = vsnprintf(&term->buffer[term->cursor],
                    sizeof(term->buffer) - term->cursor, format, args);
  va_end(args);
  if (n > 0) {
    term->cursor += n;
    if (term->cursor < sizeof(term->buffer) - 1) {
      term->buffer[term->cursor++] = '\n';
      term->buffer[term->cursor] = '\0';
    }
  }
}
