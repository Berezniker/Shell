#include <stdio.h>		// fprintf(), putchar()
#include <stdlib.h>		// malloc(), free(), getenv()
#include <string.h>		// strcmp(), strcpy(), strlen(), strerror()
#include <sys/wait.h>	// waitpid()
#include <fcntl.h>		// O_WRONLY, O_CREAT, O_TRUNC, O_RDONLY, O_APPEND
#include <unistd.h>		// fork(), pipe(), dup2, close(), execvp()
#include <dirent.h>		// chdir()
#include <signal.h>		// signal()
#include <setjmp.h>		// sigsetjmp(), longjmp(), siglongjmp()
#include <errno.h>		// errno
#include "tree.h"
#include "exec.h"
#include "bckgrnd.h"
#include "color.h"

extern jmp_buf begin;		// метка для возвращения в shell.c при возникновении ошибки
extern jmp_buf exitshell;	// метка для возвращения в shell.c и завершения shell по команде exit
extern sigjmp_buf sigset;	// метка для возвращения при обработке прерывания сигналом SIGINT

LIST backgrnd = NULL;		// список фоновых процессов
LIST convpid = NULL;		// список процессов, запущенных в конвейере
char *OLDPWD = NULL;		// путь к директории, до её изменения
int  PSUB = 0;				// > 0, если мы находимя в subshell , 0 - основной процесс

void sigcmd(int signum) {
	siglongjmp(sig_cmd, 1);
}

void sigcnv(int signum) {
	siglongjmp(sig_cnv, 1);
}

void sigsub(int signum) {
	siglongjmp(sig_sub, 1);
}

int command_process(TREE treePtr)
{
	int finstat = 0;
	static int prevstat;

	if (treePtr == NULL) 
		return 1;

	// conveyor
	if (treePtr->pipe != NULL)
		prevstat = exec_conv(treePtr, STDIN_FILENO);
		// переход к следующей команде...
	// subshell
	else if (treePtr->psubcmd != NULL)
		prevstat = subshell(treePtr);
		// переход к следующей команде...
	// execute the command
	else
		prevstat = exec_cmd(treePtr);
		// переход к следующей команде...

	// prevstat = 1 -- успешное завершение предыдущей команды, иначе = 0
	if (treePtr->type == NXT)
		finstat = command_process(treePtr->next);
	else if (treePtr->type == OR) {
		if (prevstat == 1) 					// если предыдущая команда выполнилась успешно
			while (treePtr->type == OR)		// то пропускаем все || до следующей отличной
				treePtr = treePtr->next;
		finstat =  command_process(treePtr->next);
	}
	else if (treePtr->type == AND) {
		if (prevstat == 0)					// если предыдущая команда выполнилась неуспешно
			while (treePtr->type == AND)	// то пропускаем все && до следующей отличной
				treePtr = treePtr->next;
		finstat = command_process(treePtr->next);
	}
	else
		finstat = prevstat ? 1 : -1; // finstat = 1 - успешное завершение последней команды, = -1 ошибка

	return finstat;
} // end command_process()

int subshell(TREE treePtr)
{
	int pid, status;

	// установка обработчика сигнала SIGINT
	signal(SIGINT, sigsub);
	PSUB++;
	if ( (pid = fork()) == 0) { // процесс-subshell
		int substat;

		change_iofiles(treePtr);						// настраиваем перенаправление ввода-вывода
		substat = command_process(treePtr->psubcmd);	// выполнение команд в subshell
		longjmp(exitshell, substat);					// завершение процесса-subshell
	}
	// зомби от процессов, выполняющихся в subshell, не останется
	// т.к. они являются внуками по отношению к основному процессу
	// и после завершения subshell перейдут под покравительство init
	PSUB--;

	if (sigsetjmp(sig_sub, 1)) {
		waitpid(pid, NULL, 0);
		if (PSUB)
			longjmp(exitshell, 1);
		siglongjmp(sigset, 1);
	}

	// основной процесс:
	if (treePtr->backgrnd == 0) { // дождаться subshell
		waitpid(pid, &status, 0);
		return WIFEXITED(status) && !WEXITSTATUS(status);
	}
	else { // subshell выполняется в фоновом режиме
		add_pid(&backgrnd, pid, treePtr->argv); // записать pid в список процессов
		return 1;
	}
}

int exec_conv(TREE treePtr, int in)
{
	int fd[2], out, next_in, pid, status, sigcall = 0;
	static TREE convHead = NULL;

	if (convHead == NULL)
		convHead = treePtr; // запомнили звено - начало конвейера

	pipe(fd);				// fd[0] - читать, fd[1] - писать
	out = fd[1];			// писать
	next_in = fd[0];		// читать для следующего

	// установка обработчика сигнала SIGINT
	signal(SIGINT, sigcnv);

	if ( (pid = fork()) == 0) {
		int convstat;

		close(next_in);
		if (in != STDIN_FILENO) {
			dup2(in, STDIN_FILENO);
			close(in);
		}
		if (treePtr->pipe != NULL)
			dup2(out, STDOUT_FILENO);
		close(out);

		// subshell
		if (treePtr->psubcmd != NULL) {
			convstat = subshell(treePtr);
			convstat = convstat ? 1 : -1;
		}

		// execute the command
		else {
			// exit
			if (!strcmp(treePtr->argv[0], "exit"))
				convstat = 1;
			// cd
			else if (!strcmp(treePtr->argv[0], "cd")) {
				convstat = exec_cd(treePtr);
				convstat = convstat ? 1 : -1;
			}
			else {
				exec_simpl_cmd(treePtr);
				convstat = -1;
			}
			
		}
		longjmp(exitshell, convstat);
	} // end son' process

	if (in != STDIN_FILENO)
		close(in);
	close(out);

	add_pid(&convpid, pid, treePtr->argv);

	if (treePtr->pipe != NULL)
		exec_conv(treePtr->pipe, next_in);

	close(next_in);

	if (treePtr == convHead) {
		LISTNODEPTR curpid = convpid;

		if (treePtr->backgrnd == 0) { // дождаться
			if (sigsetjmp(sig_cnv, 1))
				sigcall = 1; // флаг: происходит обработку сигнала SIGINT
			while (curpid != NULL) {
				waitpid(curpid->pid, &status, 0);
				curpid = curpid->nextPtr;
			}
		}
		else // процесс выполняется в фоновом режиме
			while (curpid != NULL) {
				add_pid(&backgrnd, curpid->pid, convHead->argv); // записать pid в список
				curpid = curpid->nextPtr;
				convHead = convHead->pipe;
			}

		destroy_backgrndList(&convpid);
		convHead = NULL;
		convpid = NULL;
		if (sigcall) { // обработка сигнала SIGINT по установленному флагу
			if (PSUB)
				longjmp(exitshell, 1);
			siglongjmp(sigset, 1);
		}
		if (treePtr->backgrnd == 0)
			return WIFEXITED(status) && !WEXITSTATUS(status);
	}

	return 1;
}

int exec_cmd(TREE treePtr)
{
	int pid, status;

	// exit
	if (!strcmp(treePtr->argv[0], "exit"))
		longjmp(exitshell, 1);
		// завершение shell

	// cd
	if (!strcmp(treePtr->argv[0], "cd"))
		return exec_cd(treePtr);
		// переход к следующей команде...

	// установка обработчика сигнала SIGINT
	signal(SIGINT, sigcmd);

	// процесс-сын: execute the command
	if ( (pid = fork()) == 0) {
		exec_simpl_cmd(treePtr);
		longjmp(exitshell, -1);
	}

	if (sigsetjmp(sig_cmd, 1)) { // обработка сигнала SIGINT
		waitpid(pid, NULL, 0);
		if (PSUB)
			longjmp(exitshell, 1);
		siglongjmp(sigset, 1);
	}

	// процесс-отец
	if (treePtr->backgrnd == 0) { // дождаться
		waitpid(pid, &status, 0);
		return WIFEXITED(status) && !WEXITSTATUS(status);
	}
	else { // процесс выполняется в фоновом режиме
		add_pid(&backgrnd, pid, treePtr->argv); // записать pid в список
		return 1;
	}
}

void exec_simpl_cmd(TREE treePtr)
{
	// устанавливаем обработку сигнала SIGINT для сыновьего процесса
	if (treePtr->backgrnd == 1)
		signal(SIGINT, SIG_IGN); // игнорировать для фоновых
	else
		signal(SIGINT, SIG_DFL);

	// настраиваем перенаправление ввода-вывода
	change_iofiles(treePtr);

	// выполнить команду
	execvp(treePtr->argv[0], treePtr->argv );
	fprintf(stderr, "shell: %s: command not found\n", treePtr->argv[0]); // обработка ошибки
}

void change_iofiles(TREE treePtr)
{
	int fd_file;

	if (treePtr->infile != NULL) {
		if ( (fd_file = open(treePtr->infile, O_RDONLY)) == -1) {
			fprintf(stderr, "shell: %s: %s\n", treePtr->infile, strerror(errno)); // обработка ошибки
			longjmp(exitshell, -1);
		}
		dup2(fd_file, STDIN_FILENO);
		close(fd_file);
	}
	if (treePtr->outfile != NULL) {
		if (treePtr->iotp == NEW) {
			if ( (fd_file = open(treePtr->outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1) {
				fprintf(stderr, "%s<ERROR>%s change_iofile: open: %s\n", RED, RESET, strerror(errno)); // обработка ошибки
				longjmp(exitshell, -1);
			}
		}
		else if (treePtr->iotp == CONT) {
			if ( (fd_file = open(treePtr->outfile, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU)) == -1) {
				fprintf(stderr, "%s<ERROR>%s change_iofile: open: %s\n", RED, RESET, strerror(errno)); // обработка ошибки
				longjmp(exitshell, -1);
			}
		}
		dup2(fd_file, STDOUT_FILENO);
		close(fd_file);
	}
}

int exec_cd(TREE treePtr)
{
	char *tempdir = NULL;
	tempdir = getcwd(tempdir, 0);

	if (treePtr->argv[1] != NULL && treePtr->argv[2] != NULL) {
		fprintf(stderr, "shell: cd: too many arguments\n");
		free(tempdir);
		return 0;
	}
	else if (treePtr->argv[1] == NULL || !strcmp(treePtr->argv[1], "~") ) {
		if (chdir(getenv("HOME")) == -1) {
			fprintf(stderr, "%s<ERROR>%s exec_cd: chdir: %s\n", RED, RESET, strerror(errno)); // обработка ошибки
			free(tempdir);
			return 0;
		}
		else {
			free(OLDPWD);
			OLDPWD = tempdir;
			return 1;
		}
	}
	else if (!strcmp(treePtr->argv[1], "-")) {
		if (OLDPWD == NULL) {
			free(tempdir);
			fprintf(stderr, "shell: cd: OLDPWD not set\n"); // обработка ошибки
			return 0;
		}
		chdir(OLDPWD);
		free(OLDPWD);
		OLDPWD = tempdir;
		return 1;
	}
	else if (chdir(treePtr->argv[1]) == 0) {
		free(OLDPWD);
		OLDPWD = tempdir;
		return 1;
	}
	else {
		fprintf(stderr, "shell: cd: %s: %s\n", treePtr->argv[1], strerror(errno));
		free(tempdir);
		return 0;
	}
}