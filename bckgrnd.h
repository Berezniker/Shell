struct backgrndList {				// list of background processes
	int  pid;						// processes' pid
	char *cmd;						// command name
	struct backgrndList *nextPtr;	// pointer to the next link in the list
};
typedef struct backgrndList LISTNODE;
typedef LISTNODE *LISTNODEPTR;
typedef LISTNODEPTR LIST;

/**
 * [add_pid: creates the link in the list 'pidlist', writing the processes' pid and the executing command cmd_name]
 */
void add_pid(LIST *pidlist, int pid, char **cmd_name);

/**
 * [rewrite_cmd: rewrites to the string '*to' words from the array of words cmd_name[] through the space]
 */
void rewrite_cmd(char **to, char **cmd_name);

/**
 * [destroy_zombie: deletes the links of finished background processes in the list 'pidlist']
 */
void destroy_zombie(LIST *pidlist);

/**
 * [print_list: prints the list 'pidlist' of the executed background processes]
 */
void print_list(LIST pidlist);

/**
 * [destroy_backgrndList: destroys the list 'pidlist']
 */
void destroy_backgrndList(LIST *pidlist);