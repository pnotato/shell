#define _GNU_SOURCE
#include "../include/msgs.h"
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../include/history.h"
#include "../include/main.h"

void shell() {
  // Variables for cwd
  char current_line[MAX_COMMAND_SIZE];
  char cwd_buf[PATH_MAX];

  // Previous for cd -
  char cwd_prev[PATH_MAX];
  char *cwd_prev_ptr = cwd_prev;

  // History
  struct sigaction handler;
  handler.sa_handler = handle_sigint;
  sigemptyset(&handler.sa_mask);
  handler.sa_flags = 0;
  sigaction(SIGINT, &handler, NULL);

  while (true) {
    // Current Working Directory and shell stuff
    char *cwd_ptr = getcwd(cwd_buf, sizeof(cwd_buf));

    if (cwd_ptr == NULL) {
      const char *err = FORMAT_MSG("shell", GETCWD_ERROR_MSG);
      write(STDERR_FILENO, err, strlen(err));
      cwd_ptr = "";
    }
    const char *shell_sign = "$ ";
    write(STDOUT_FILENO, cwd_ptr,
          strlen(
              cwd_ptr)); // For reference, this is the shell path that's printed
    write(STDOUT_FILENO, shell_sign, strlen(shell_sign));
    ssize_t command_size = read(STDIN_FILENO, current_line, MAX_COMMAND_SIZE);
    if (command_size == -1) {
      if (errno == EINTR) {
        continue;
      }

    } else if (command_size == 0) {
      const char *file_err = FORMAT_MSG("shell", GETCWD_ERROR_MSG);
      write(STDERR_FILENO, file_err, strlen(file_err));
      continue;
    }
    current_line[command_size] = '\0';
    if (current_line[0] != '!') {
      add_to_history(current_line);
    }
    run_line(current_line, cwd_ptr, cwd_prev_ptr);
    // NEED A FREE FOR THE LINE THATS PASSED TO RUN_LINE
  }
}

// Handle Siging
void handle_sigint() {
  const char *help_space = "\n";
  const char *help_msg0 = FORMAT_MSG("cd", CD_HELP_MSG);
  const char *help_msg1 = FORMAT_MSG("pwd", PWD_HELP_MSG);
  const char *help_msg2 = FORMAT_MSG("exit", EXIT_HELP_MSG);
  const char *help_msg3 = FORMAT_MSG("help", HELP_HELP_MSG);
  const char *help_msg4 = FORMAT_MSG("history", HISTORY_HELP_MSG);
  write(STDOUT_FILENO, help_space, strlen(help_space));
  write(STDOUT_FILENO, help_msg3, strlen(help_msg3));
  write(STDOUT_FILENO, help_msg1, strlen(help_msg1));
  write(STDOUT_FILENO, help_msg2, strlen(help_msg2));
  write(STDOUT_FILENO, help_msg0, strlen(help_msg0));
  write(STDOUT_FILENO, help_msg4, strlen(help_msg4));
  return;
}

void tokenize_input(char *line, char **commands, bool *is_background) {
  char *saveptr;
  char *token = strtok_r(line, " \n\t", &saveptr);
  int i = 0;

  while (token != NULL) {
    commands[i++] = token;
    token = strtok_r(NULL, " \n\t", &saveptr);
  }
  if (i > 0 && strcmp(commands[i - 1], "&") == 0) {
    // debug("DEBUG: BACKGROUND MODE ON");
    *is_background = true;
    commands[i - 1] = NULL;
  } else {
    commands[i] = NULL;
  };
}

// Foreground Execution
void run_line(char *line, char *cwd, char *cwd_prev_ptr) {
  char *commands[MAX_COMMAND_SIZE]; // CHANGE THIS LATER ON
  memset(commands, 0, sizeof(commands));
  bool is_background = false;
  kill_zombies();

  tokenize_input(line, commands, &is_background);

  // Checks if the array is empty. if it is, immediately return
  if (commands[0] == NULL) {
    return;
  }
  create_child_process(is_background, commands, cwd, cwd_prev_ptr);
}

void create_child_process(bool is_background, char **commands, char *cwd,
                          char *cwd_prev_ptr) {
  // If no internal command is run, continue with fork.
  // Check if an internal command is run.
  if (strcmp(commands[0], "exit") == 0) {
    if (commands[1] == NULL) {
      kill_zombies();
      exit(EXIT_SUCCESS);
    } else {
      const char *exit_cmd_err = FORMAT_MSG("exit", TMA_MSG);
      write(STDERR_FILENO, exit_cmd_err, strlen(exit_cmd_err));
      return;
    }
  } else if (commands[0][0] == '!') {

    if (strcmp(commands[0], "!!") == 0) {
      if (history_count < 1) {
        // Debug, could be an error here since i dont know if this check is
        // valid
        char *history_no_last = FORMAT_MSG("history", HISTORY_NO_LAST_MSG);
        write(STDERR_FILENO, history_no_last, strlen(history_no_last));
        return;
      } else {
        char *last_line = input_history[history_count - 1];
        char *last_line_copy = strdup(last_line);
        if (!last_line_copy)
          return;

        for (int i = 0; i < MAX_COMMAND_SIZE; i++) {
          commands[i] = NULL;
        }

        write(STDOUT_FILENO, last_line_copy, strlen(last_line_copy));
        // write(STDOUT_FILENO, "\n", 1);

        add_to_history(last_line_copy);

        tokenize_input(last_line_copy, commands, &is_background);
        create_child_process(is_background, commands, cwd, cwd_prev_ptr);
        free(last_line_copy);
        return;
      }
    } else {
      char *end;
      char *n_val = commands[0] + 1;
      long index = strtol(n_val, &end, 10);

      if (*end != '\0' || index < 0 || index >= history_numbers) {
        char *history_invalid = FORMAT_MSG("history", HISTORY_INVALID_MSG);
        write(STDERR_FILENO, history_invalid, strlen(history_invalid));
        return;
      }

      char *resolved_line = get_nth_history(index);

      if (!resolved_line) {
        char *invalid_err_msg = FORMAT_MSG("history", HISTORY_INVALID_MSG);
        write(STDERR_FILENO, invalid_err_msg, strlen(invalid_err_msg));
        return;
      }

      // char full_line[PATH_MAX + MAX_COMMAND_SIZE];
      // snprintf(full_line, sizeof(full_line), "%s$ %s\n", cwd, resolved_line);

      write(STDOUT_FILENO, resolved_line, strlen(resolved_line));
      // write(STDOUT_FILENO, "\n", 1);

      for (int i = 0; i < MAX_COMMAND_SIZE; i++) {
        commands[i] = NULL;
      }

      char *temp_line = strdup(resolved_line);
      if (!temp_line)
        return;
      size_t len = strlen(temp_line);
      if (len > 0 && temp_line[len - 1] == '\n') {
        temp_line[len - 1] = '\0';
      }
      tokenize_input(temp_line, commands, &is_background);
      create_child_process(is_background, commands, cwd, cwd_prev_ptr);

      add_to_history(temp_line);
      free(temp_line);
      return;
    }
  }

  else if (strcmp(commands[0], "history") == 0) {
    print_history();
    return;
  } else if (strcmp(commands[0], "pwd") == 0) {
    char cwd_buf[PATH_MAX];
    char *cwd_ptr = getcwd(cwd_buf, sizeof(cwd_buf));
    const char *escape = "\n";

    if (cwd_ptr == NULL) {
      const char *err = FORMAT_MSG("pwd", GETCWD_ERROR_MSG);
      write(STDERR_FILENO, err, strlen(err));
      return;
    }
    if (commands[1] != NULL) {
      const char *err = FORMAT_MSG("pwd", TMA_MSG);
      write(STDERR_FILENO, err, strlen(err));
      return;
    }
    write(STDOUT_FILENO, cwd_ptr, strlen(cwd_ptr));
    write(STDOUT_FILENO, escape, strlen(escape));

    return;
  } else if (strcmp(commands[0], "cd") == 0) {
    // char cwd_buf[PATH_MAX];
    // char *cwd_ptr = getcwd(cwd_buf, sizeof(cwd_buf));
    char *new_path;
    if (commands[2] != NULL) {
      const char *err = FORMAT_MSG("cd", TMA_MSG);
      write(STDERR_FILENO, err, strlen(err));
      return;
    }
    if (commands[1] == NULL || strcmp(commands[1], "~") == 0) {
      struct passwd *pw = getpwuid(getuid());
      if (pw == NULL) {
        const char *cd_err = FORMAT_MSG("cd", GETCWD_ERROR_MSG);
        write(STDERR_FILENO, cd_err, strlen(cd_err));
        return;
      }
      new_path = pw->pw_dir;
    } else if (commands[1][0] == '~') {
      // struct passwd *pw_user = getpwnam(commands[1] + 1);
      struct passwd *pw_user = getpwuid(getuid());
      if (pw_user == NULL) {
        const char *cd_err2 = FORMAT_MSG("cd", GETCWD_ERROR_MSG);
        write(STDERR_FILENO, cd_err2, strlen(cd_err2));
        return;
      }

      char expanded[PATH_MAX];
      snprintf(expanded, sizeof(expanded), "%s%s", pw_user->pw_dir,
               commands[1] + 1);
      new_path = expanded;
    } else if (strcmp(commands[1], "-") == 0) {
      new_path = cwd_prev_ptr;
    } else {
      new_path = commands[1];
    }

    // Actual running of path
    if (chdir(new_path) != 0) {
      const char *err = FORMAT_MSG("cd", CHDIR_ERROR_MSG);
      write(STDERR_FILENO, err, strlen(err));

      snprintf(cwd_prev_ptr, PATH_MAX, "%s", cwd);
      return;
    } else {

      snprintf(cwd_prev_ptr, PATH_MAX, "%s", cwd);
      // const char *err = FORMAT_MSG("cd", CHDIR_ERROR_MSG);
      // write(STDERR_FILENO, err, strlen(err));
    }
  } else if (strcmp(commands[0], "help") == 0) {
    if (commands[2] != NULL) {
      const char *help_cmd_err = FORMAT_MSG("help", TMA_MSG);
      write(STDERR_FILENO, help_cmd_err, strlen(help_cmd_err));
      return;
    }
    if (commands[1] == NULL) {

      const char *help_msg4 = FORMAT_MSG("history", HISTORY_HELP_MSG);
      const char *help_msg0 = FORMAT_MSG("cd", CD_HELP_MSG);
      const char *help_msg1 = FORMAT_MSG("pwd", PWD_HELP_MSG);
      const char *help_msg2 = FORMAT_MSG("exit", EXIT_HELP_MSG);
      const char *help_msg3 = FORMAT_MSG("help", HELP_HELP_MSG);
      write(STDOUT_FILENO, help_msg3, strlen(help_msg3));
      write(STDOUT_FILENO, help_msg1, strlen(help_msg1));
      write(STDOUT_FILENO, help_msg2, strlen(help_msg2));
      write(STDOUT_FILENO, help_msg0, strlen(help_msg0));
      write(STDOUT_FILENO, help_msg4, strlen(help_msg4));
      return;
    } else if (strcmp(commands[1], "cd") == 0) {
      const char *help_cd_msg = FORMAT_MSG("cd", CD_HELP_MSG);
      write(STDOUT_FILENO, help_cd_msg, strlen(help_cd_msg));
      return;
    } else if (strcmp(commands[1], "pwd") == 0) {
      const char *help_pwd_msg = FORMAT_MSG("pwd", PWD_HELP_MSG);
      write(STDOUT_FILENO, help_pwd_msg, strlen(help_pwd_msg));
      return;
    } else if (strcmp(commands[1], "exit") == 0) {
      const char *help_exit_msg = FORMAT_MSG("exit", EXIT_HELP_MSG);
      write(STDOUT_FILENO, help_exit_msg, strlen(help_exit_msg));
      return;
    } else if (strcmp(commands[1], "help") == 0) {
      const char *help_help_msg = FORMAT_MSG("help", HELP_HELP_MSG);
      write(STDOUT_FILENO, help_help_msg, strlen(help_help_msg));
      return;
    } else {
      // debug("break1");
      // FORMAT MSG DOES NOT WORK HERE.
      char help_extern_msg[PATH_MAX];
      snprintf(help_extern_msg, PATH_MAX, "%s: %s\n", commands[1],
               EXTERN_HELP_MSG);
      write(STDOUT_FILENO, help_extern_msg, strlen(help_extern_msg));
      return;
    }

  } else {
    pid_t pid = fork();
    if (pid == -1) {
      const char *err = FORMAT_MSG("shell", FORK_ERROR_MSG);
      write(STDERR_FILENO, err, strlen(err)); // MAYBE CHANGE TO OUT?
    } else if (pid == 0 && commands[0] != NULL) {
      if (execvp(commands[0], commands) == -1) {
        // I think we use execvp so a path doesn't need to be specified
        // also because our arguments are in an array
        const char *err = FORMAT_MSG("shell", EXEC_ERROR_MSG);
        write(STDERR_FILENO, err, strlen(err));
      }
    } else {
      int wstatus = 0;
      if (!is_background) {
        if (waitpid(pid, &wstatus, 0) == -1) {
          const char *err = FORMAT_MSG("shell", WAIT_ERROR_MSG);
          write(STDERR_FILENO, err, strlen(err));
        }
      }
    }
  }
}

// kills all currently running child processes.
void kill_zombies() {
  int wstatus;
  while (waitpid(-1, &wstatus, WNOHANG) > 0) {
    continue;
  }
}

int main() {
  shell();
  return 0;
}
