// точка возрата при обработке прерывания сигналом SIGINT
sigjmp_buf sig_cmd;		// из процесса, выполняющего одиночную команду
sigjmp_buf sig_cnv;		// из конвейера
sigjmp_buf sig_sub;		// из subshell

/**
 * [sigXXX: SIGINT signal handler, <Ctrl+C>]
 */
void sigcmd(int signal_number);
void sigcnv(int signal_number);
void sigsub(int signal_number);

/**
 * [command_process: executing processes when passing the tree rootPtr]
 * @return [1 in case of successful completion of the process, 0 otherwise]
 */
int command_process(TREE rootPtr);

/**
 * [subshell: executing a process in a subshell]
 * @return [1 in case of successful completion of the process, 0 otherwise]
 */
int subshell(TREE treePtr);

/**
 * [exec_conv: executing the instruction pipeline]
 * @param  in [file descriptor according to which reading is done in the running process]
 * @return    [1 in case of successful completion of the process, 0 otherwise]
 */
int exec_conv(TREE treePtr, int in);

/**
 * [exec_cd: change directory comman execution]
 * @return [1 in case of successful completion of the process, 0 otherwise]
 */
int exec_cd(TREE treePtr);

/**
 * [exec_cmd: executing a command in a child process]
 * @return [1 in case of successful completion of the process, 0 otherwise]
 */
int exec_cmd(TREE treePtr);

/**
 * [exec_simpl_cmd: executing a command]
 */
void exec_simpl_cmd(TREE treePtr);

/**
 * [change_iofiles: process I / O redirection to the appropriate files]
 */
void change_iofiles(TREE treePtr);