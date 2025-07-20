#pragma once

#include <signal.h>

extern volatile sig_atomic_t terminal_resized;

static void terminal_cursor_up(size_t amount);

static void terminal_cursor_down(size_t amount);

void terminal_clear();

void terminal_enable_raw_mode();

void terminal_disable_raw_mode();

void terminal_clear_last_lines(size_t n);

void terminal_handle_sigint(int sig);

void terminal_handle_sigwinch(int sig);

void terminal_handle_sigsegv(int sig);