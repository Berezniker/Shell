#include <stdio.h>			// printf(), putchar(), fflush(), feof()
#include <stdlib.h>			// calloc(), realloc(), free(), getenv()
#include <string.h>			// strerror()
#include <setjmp.h>			// setjmp(), longjmp(), sigsetjmp(), siglongjmp()
#include <unistd.h>			// getcwd(), gethostname(), _exit()
#include <signal.h>			// signal()
#include <errno.h>			// errno
#include "list.h"
#include "tree.h"
#include "exec.h"
#include "bckgrnd.h"
#include "color.h"

jmp_buf		begin;			// точка возврата из модулей при возникновении ошибки
jmp_buf		exitshell;		// точка возрата из exec для завершения shell по команде exit
sigjmp_buf	sigset;			// точка возрата при обработке прерывания сигналом SIGINT
extern int	syntax_error;	// флаг из tree.c, информирующий об обнаружении синтаксической ошибки при построении дерева
extern LIST backgrnd;		// список фоновых процессов, формирующийся в exec.c
extern LIST convpid;		// список процессов, запущенных в конвейере, формируется в exec.c
extern char *OLDPWD;		// путь к предыдущей директории, изменяется в exec.c

/**
 * [makeList: bulds the string (array of words) with the hellp of getword()]
 * @param vecSize [vector array size]
 * @param vector  [array of words]
 */
void makeList(char ***vector, int *vecSize);

/**
 * [invite: prints the invetation to enter]
 */
void invite(void);

/**
 * [makeTree: bulds the tree according to the given vector]
 * @param treePtr [tree pointer]
 * @param vector  [array of words]
 * @param vecSize [vector array size]
 */
void makeTree(TREE *treePtr, char **vector, int vecSize);

/**
 * [sighandler: SIGINT signal handler, <Ctrl+C>]
 */
void sighandler(int signum) {
	putchar('\n');
	siglongjmp(sigset, 1);
}

void error(int error_num) {
	fflush(stdin);
}

int main(int argc, char *argv[])
{
	char	**vector = NULL;	// массив указателей на строки
	int		vecSize;			// размер массива vector
	int		status;				// статус завершения процесса
	TREE	rootPtr = NULL;		// корень дерева

	while (1) {
		// точки возврата для завершения shell по команде exit или по <Ctrl-D>
		if ( (status = setjmp(exitshell)) == 1 || status == -1) {
			destruction_list(vector);
			destruction_tree(&rootPtr);
			destroy_backgrndList(&convpid);
			destroy_backgrndList(&backgrnd);
			free(OLDPWD);
			if (status == 1)
				_exit(0);
			else
				_exit(-1);
		}
		// точка возврата при возникновении ошибки
		if (setjmp(begin))
			error(0);
		// точка возрата при обработке прерывания сигналом SIGINT
		sigsetjmp(sigset, 1);
		signal(SIGINT, sighandler);

		// удаление списка и дерева
		destruction_list(vector);
		destruction_tree(&rootPtr);
		// очистка зомби из списка (таблицы) зарегистрированных фоновых процессов
		destroy_zombie(&backgrnd);

		// создание списка: разбиение строки на лексемы, лексический анализ
		makeList(&vector, &vecSize);
		// printList(vector, vecSize); /* отладочная печать списка */

		if (feof(stdin) && *vector == NULL)
			longjmp(exitshell, 1);

		// создание дерева: формирование звеньев и путей, синтаксический анализ
		makeTree(&rootPtr, vector, vecSize);
		// printTree(rootPtr); /* отладочная печать дерева */

		// выполнить команду, обходя дерево и создавая соответствующие процессы, файлы, каналы
		command_process(rootPtr);
	}
}

void makeList(char ***vector, int *vecSize)
{
	char  *str;	// temp-переменная для хранения слова, возвращенного getword

	*vecSize = 0;
	if ( (*vector = (char **) calloc(++(*vecSize), sizeof(char *))) == NULL) {
		fprintf(stderr, "%s<ERROR>%s makeList: calloc: %s\n", RED, RESET, strerror(errno)); // обработка ошибки
		longjmp(begin, 2);
	}
	**vector = NULL;

	invite(); // приглашение к вводу
	while ( (str = getword()) != NULL) {
		// выделить 1 блок памяти для полученного слова
		if ( (*vector = (char **) realloc(*vector, ++(*vecSize) * sizeof(char *))) == NULL) {
			fprintf(stderr, "%s<ERROR>%s makeList: realloc: %s\n", RED, RESET, strerror(errno)); // обработка ошибки
			longjmp(begin, 2);
		}
		// вставить слово-str в конец массива *vector
		insertInLast(*vector, str);
	}
	(*vecSize)--; // размер массива слов без учёта последнего элемента-NULL
}

void invite()
{
	char	*cur_dir_path = NULL;	// путь к каталогу, в котором находится пользователь
	char	*user_name = NULL;		// имя пользователя
	char	host_name[64];			// имя компьютера

	cur_dir_path = getcwd(cur_dir_path, 0);
	user_name = getenv("USER");
	gethostname(host_name, 64);

	// приглашение к вводу:
	printf("%s%s@%s%s:%s%s%s$ ", YELLOW, user_name != NULL ?  user_name :  "<user>",
										 host_name != NULL ?  host_name :  "<host>", RESET,
								BLUE, cur_dir_path != NULL ? cur_dir_path : "<dir>", RESET);

	free(cur_dir_path);
}

void makeTree(TREE *treePtr, char **vector, int vecSize)
{
	int pos = 0;

	*treePtr = NULL;
	if (*vector == NULL)
		return;

	*treePtr = first_lvl(vector, &pos, vecSize);

	if (syntax_error || vector[pos] != NULL) {
		if (vector[pos] != NULL) // => соощение об ошибке не было выдано при построении дерева
			fprintf(stderr, "shell: syntax error\n"); // обработка ошибки
		destruction_tree(treePtr);
	}
}