#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

void run_line(char *line, char *cwd, char *cwd_prev);
void handle_sigint();
void create_child_process(bool is_background, char **commands, char *cwd,
                          char *cwd_prev_ptr);
void tokenize_input(char *line, char **commands, bool *is_background);
void debug(char *line) { write(STDERR_FILENO, line, strlen(line)); }
void kill_zombies();