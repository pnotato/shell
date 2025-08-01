#include <unistd.h>

#define MAX_COMMAND_SIZE 4096
#define MAX_HISTORY_LEN 10

extern char *input_history[MAX_HISTORY_LEN];
extern int history_count;
extern int history_numbers;

void add_to_history(char *input);
void print_history();
void remove_oldest_record();
char *get_nth_history(int index);