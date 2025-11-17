#include <command.h>

void free_command(Command* cmd) {
    if (cmd->type == CMD_OTHER && cmd->argv) {
	for (int i=0; i<cmd->argc; i++) {
	    free(cmd->argv[i]);
	}
	free(cmd->argv);
    }
}
