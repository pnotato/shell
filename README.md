# A Simple Shell in C

A lightweight implementation of a Unix-like shell built in C. Supports process execution, built-built in commands, and command history.

## Features
- Executes external programs in separate processes.
- Supports both **foreground execution** (blocking until the process finishes) and **background execution** (non-blocking, using `&` at the end of the command).
- Cleans up background processes to prevent zombies.

### Internal Commands
- `exit`: Exit the shell (rejects extra arguments).
- `pwd`: Display the current working directory.
- `cd`: Change directories, with support for:
  - No argument → go to home directory.
  - `~` expansion for home.
  - `-` to return to the previous directory.
- `help`: Display help messages for built-in commands, or identify unknown commands as external.

### Signal Handling
- Gracefully handles **CTRL-C (SIGINT)** without terminating the shell.
- Displays help information when interrupted and re-displays the prompt.

### History
- Maintains up to the **10 most recent commands** with numbering.
- `history`: Show the last 10 commands (or fewer if less have been run).
- `!!`: Re-run the last command entered.
- `!n`: Re-run a specific command number from history.
- Ensures proper error handling for invalid history references.

## Usage

Clone the repository and build with CMake:

```bash
git clone https://github.com/pnotato/shell.git
cd shell
mkdir build && cd build
cmake ..
make
```

Run the shell with `./shell`.

The shell will display the current working directory followed by `$`. Enter commands as you would in a normal shell. Use `&` to throw a process into the background, `exit` to quit, and `help` for information on internal commands.
