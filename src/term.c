#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

volatile sig_atomic_t terminal_resized = 0;

void terminal_cursor_up(size_t amount) { printf("\x1b[%zuA", amount); }

void terminal_cursor_down(size_t amount) { printf("\x1b[%zuB", amount); }

void terminal_clear() {
  printf("\033c");
  fflush(stdout);
}

void terminal_enable_raw_mode() {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
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

void terminal_handle_sigint(int sig) {
  (void)sig;
  terminal_disable_raw_mode();
  exit(0);
}

void terminal_handle_sigwinch(int sig) {
  (void)sig;
  terminal_resized = 1;
}

void terminal_handle_sigsegv(int sig) {
  (void)sig;
  terminal_disable_raw_mode();

  fprintf(stderr, "Encountered sigsegv\n");
}