/*--------------------PROCESSING THE LIST OF BACKGROUND PROCESSES--------------------*/

#include <stdio.h>		// printf(), fprintf(), putchar()
#include <stdlib.h>		// strerror(), malloc(), free()
#include <string.h>		// strlen()
#include <setjmp.h>		// longjmp()
#include <errno.h>		// errno
#include <sys/wait.h>	// waitpid(), WIFEXITED, WEXITSTATUS
#include "bckgrnd.h"
#include "color.h"

extern jmp_buf begin;	// метка для возвращения в shell.c при возникновении ошибки

void add_pid(LIST *sPtr, int pid, char **cmd)
{
	LISTNODEPTR curPtr;

	if (*sPtr == NULL) { // первое обращение
		if ( (*sPtr = (LISTNODEPTR) malloc(sizeof(LISTNODE))) == NULL) { // обработка ошибки
			fprintf(stderr, "%s<ERROR>%s add_pid: malloc: %s\n", RED, RESET, strerror(errno));
			longjmp(begin, 1);
		}
		(*sPtr)->pid = pid;
		(*sPtr)->nextPtr = NULL;
		rewrite_cmd( &((*sPtr)->cmd), cmd);
	}
	else {
		curPtr = *sPtr;
		while (curPtr->nextPtr != NULL)
			curPtr = curPtr->nextPtr;

		curPtr = curPtr->nextPtr = (LISTNODEPTR) malloc(sizeof(LISTNODE));
		if (curPtr == NULL) { // обработка ошибки
			fprintf(stderr, "%s<ERROR>%s add_pid: malloc: %s\n", RED, RESET, strerror(errno));
			longjmp(begin, 1);
		}
		curPtr->pid = pid;
		curPtr->nextPtr = NULL;
		rewrite_cmd( &(curPtr->cmd), cmd);
	}
}

void rewrite_cmd(char **list, char **cmd)
{
	int pos = 0, listSize = 1; // 1 -- для завершающего '\0'

	*list = NULL;
	if (cmd[0] == NULL)
		return;

	for (int i = 0, j = 0; cmd[i] != NULL; i++, j = 0) {
		listSize += strlen(cmd[i]) + 1; // +1 для пробела между символами
		if ( (*list = (char *) realloc( *list, listSize * sizeof(char)) ) == NULL) { // обработка ошибки
			fprintf(stderr, "%s<ERROR>%s rewrite_cmd: realloc: %s\n", RED, RESET, strerror(errno));
			longjmp(begin, 1);
		}
		while (cmd[i][j] != '\0')
			list[0][pos++] = cmd[i][j++];

		if (cmd[i + 1] != NULL)
			list[0][pos++] = ' ';
	}
	list[0][pos] = '\0';
}

void destroy_zombie(LIST *sPtr)
{
	LISTNODEPTR curPtr, tempPtr, prevPtr;
	int status;
	
	curPtr = *sPtr;
	while (curPtr != NULL)
		if ( waitpid(curPtr->pid, &status, WNOHANG) ) {

			printf("[%d] ", curPtr->pid);

			if (WIFEXITED(status) && !WEXITSTATUS(status))
				printf("Done");
			else
				printf("Exit %d", WEXITSTATUS(status));

			if (curPtr->cmd != NULL)
				printf("      %s", curPtr->cmd);
			putchar('\n');

			if (curPtr == *sPtr) { /* delete from the beginning of the list */
				*sPtr = (*sPtr)->nextPtr;
				free(curPtr->cmd);
				free(curPtr);
				curPtr = *sPtr;
			}
			else {
				tempPtr = curPtr;
				prevPtr->nextPtr = curPtr->nextPtr;
				curPtr = curPtr->nextPtr;
				free(tempPtr->cmd);
				free(tempPtr);
			}
		}
		else {
			prevPtr = curPtr;
			curPtr = curPtr->nextPtr;
		}
	// end while()
}

void print_list(LIST sPtr)
{
	if (sPtr == NULL) return;

	printf("\nThe pid-list is:\n");
	while (sPtr != NULL) {
		printf("{%d} ", sPtr->pid);
		sPtr = sPtr->nextPtr;
	}
	putchar('\n');
}

void destroy_backgrndList(LIST *sPtr)
{
	LISTNODEPTR tempPtr;

	while (*sPtr != NULL) {
		tempPtr = *sPtr;
		*sPtr = (*sPtr)->nextPtr;
		free(tempPtr->cmd);
		free(tempPtr);
	}
}