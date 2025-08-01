#include "../include/history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COMMAND_SIZE 4096
#define MAX_HISTORY_LEN 10

char *input_history[MAX_HISTORY_LEN];
int history_count = 0;
int history_numbers = 0;

// History
void add_to_history(char *input) {
  if (history_count >= MAX_HISTORY_LEN) {
    remove_oldest_record();
  }
  input_history[history_count++] = strdup(input);
  history_numbers++;
}

void remove_oldest_record() {
  if (history_count > 0) {
    free(input_history[0]);
    for (int i = 1; i < history_count; i++) {
      input_history[i - 1] = input_history[i];
    }
    (history_count)--;
  }
}

void print_history() {
  int index = history_numbers - 1;
  int print_nums = history_count < 10 ? history_count : 10;
  for (int i = 0; i < print_nums; i++) {
    int hist_index = history_count - 1 - i;
    char *cmd = input_history[hist_index];
    size_t len = strlen(cmd);
    if (len > 0 && cmd[len - 1] == '\n') {
      cmd[len - 1] = '\0';
    }
    char history_buf[MAX_COMMAND_SIZE + 32];
    snprintf(history_buf, sizeof(history_buf), "%d\t%s\n", index - i, cmd);
    write(STDOUT_FILENO, history_buf, strlen(history_buf));
  }
}

char *get_nth_history(int index) {
  int start = history_numbers - history_count;
  for (int i = 0; i < history_count; i++) {
    if ((start + i) == index) {
      return input_history[i];
    }
  }
  return NULL;
}