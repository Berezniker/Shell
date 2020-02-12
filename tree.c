#include <stdio.h>		// printf(), fprintf(), putchar()
#include <stdlib.h>		// malloc(), realloc(), free(), 
#include <string.h>		// strerror()
#include <errno.h>		// errno
#include "tree.h"
#include "list.h"
#include "color.h"

int pos_NULL;			// pos_NULL: vector[pos_NULL] = NULL
int syntax_error;		// индикатор синтаксической ошибки

TREENODEPTR first_lvl(char **vector, int *pos, int vecSize)
{
	// ';', '&', '&&', '||'
	if (*pos == 0) {
		pos_NULL = vecSize; // кол-во слов в массиве vector ~ нулевая позиция для возварта по ошибке
		syntax_error = 0;	// эта инициализация нужна только при первом входе
	}

	TREENODEPTR nodePtr = second_lvl(vector, pos);

	if (vector[*pos] == NULL)
		return nodePtr;

	if (!strcmp(vector[*pos], ";") && vector[*pos + 1] != NULL && strcmp(vector[*pos + 1], ")") ) { // ;
		nodePtr->type = NXT; // установить флажок для ;
		(*pos)++; // сдвинулись на следующее слово
		nodePtr->next = first_lvl(vector, pos, pos_NULL);
	}
	else if (!strcmp(vector[*pos], "&") && vector[*pos + 1] != NULL && strcmp(vector[*pos + 1], ")") ) { // &
		tick_backgrnd(nodePtr);
		nodePtr->type = NXT; // установить флажок для &
		(*pos)++; // сдвинулись на следующее слово
		nodePtr->next = first_lvl(vector, pos, pos_NULL);
	}
	else if ( !strcmp(vector[*pos], "&&") || !strcmp(vector[*pos], "||") ) {// &&, ||
		if (vector[*pos][0] == '&')
			nodePtr->type = AND; // установить флажок для &&
		else
			nodePtr->type =  OR; // установить флажок для ||

		(*pos)++; // сдвинулись на следующее слово
		if (vector[*pos] == NULL) {
			fprintf(stderr, "shell: syntax error: missing operand after '%s'\n", vector[*pos - 1]);
			syntax_error = 1; // error
			*pos = pos_NULL;
		}
		else
			nodePtr->next = first_lvl(vector, pos, pos_NULL);
	}

	if (vector[*pos] == NULL)
		return nodePtr;

	if ( !strcmp(vector[*pos], ";") )
		(*pos)++; // сдвинулись на следующее слово

	else if ( !strcmp(vector[*pos], "&") ) {
		(*pos)++; // сдвинулись на следующее слово
		tick_backgrnd(nodePtr);
	}

	return nodePtr;
}

TREENODEPTR second_lvl(char **vector, int *pos)
{
	// '|'
	TREENODEPTR nodePtr = third_lvl(vector, pos);

	if (vector[*pos] == NULL)
		return nodePtr;

	if ( !strcmp(vector[*pos], "|") ) {
		(*pos)++; // сдвинулись на следующее слово
		if (vector[*pos] == NULL) {
			fprintf(stderr, "shell: syntax error: missing operand after '|'\n");
			syntax_error = 1; // error
			*pos = pos_NULL;
		}
		else
			nodePtr->pipe = second_lvl(vector, pos);
	}

	return nodePtr;
}

TREENODEPTR third_lvl(char **vector, int *pos)
{
	// '<', '>>', '>'
	TREENODEPTR nodePtr = fourth_lvl(vector, pos);

	if (vector[*pos] == NULL)
		return nodePtr;

	if ( !strcmp(vector[*pos], "<") ) { //infile
		(*pos)++; // сдвинулись на следующее слово
		if (vector[*pos] == NULL || cmpSpecSym(vector[*pos]) ) {
			fprintf(stderr, "shell: syntax error: missing operand after '<'\n");
			syntax_error = 1; // error
			*pos = pos_NULL;
		}
		else {
			nodePtr->infile = vector[*pos];
			(*pos)++; // сдвинулись на следующее слово
		}
	}

	if (vector[*pos] == NULL)
		return nodePtr;

	if ( !strcmp(vector[*pos], ">") || !strcmp(vector[*pos], ">>") ) { // outfile
		if (vector[*pos][1] == '>')
			nodePtr->iotp = CONT; // установить флажок для >>
		else
			nodePtr->iotp = NEW;  // установить флажок для >
		(*pos)++; // сдвинулись на следующее слово
		if (vector[*pos] == NULL || cmpSpecSym(vector[*pos]) ) {
			fprintf(stderr, "shell: syntax error: missing operand after '%s'\n", vector[*pos - 1]);
			syntax_error = 1;  // error
			*pos = pos_NULL;
		}
		else {
			nodePtr->outfile = vector[*pos];
			(*pos)++; // сдвинулись на следующее слово
		}
	}
	
	return nodePtr;
}

TREENODEPTR fourth_lvl(char **vector, int *pos)
{
	// '(', ')'
	TREENODEPTR nodePtr;

	if (vector[*pos] == NULL)
		return NULL;

	// создание звена и инициализация всех полей
	// для возможности корректного удаления дерева при возникновении ошибки
	if ( (nodePtr = (TREENODEPTR) malloc(sizeof(TREENODE))) == NULL) { // обработка ошибки
		fprintf(stderr, "%s<ERROR>%s makeTree_4: malloc: nodePtr: %s\n", RED, RESET, strerror(errno));
		syntax_error = 1;
		*pos = pos_NULL;
	}
	if ( (nodePtr->argv = (char **) malloc(sizeof(char*))) == NULL) { // обработка ошибки
		fprintf(stderr, "%s<ERROR>%s makeTree_4: malloc: argv: %s\n", RED, RESET, strerror(errno));
		syntax_error = 1;
		*pos = pos_NULL;
	}
	nodePtr->argv[0]  = NULL;
	nodePtr->infile   = NULL;
	nodePtr->outfile  = NULL;
	nodePtr->psubcmd  = NULL;
	nodePtr->pipe     = NULL;
	nodePtr->next     = NULL;
	nodePtr->type     = NUL;
	nodePtr->iotp     = MISS;
	nodePtr->backgrnd = 0;

	if ( !strcmp(vector[*pos], "(") ) {
		(*pos)++; // прочитали открывающую скобку и сдвинулись на следующее слово
		nodePtr->psubcmd = first_lvl(vector, pos, pos_NULL);
		if (vector[*pos] == NULL) { // была экранирована единственная открывающая скобка и не нашлось закрывающейся
			fprintf(stderr, "shell: syntax error: no command to execute\n");
			syntax_error = 1;	// error
			*pos = pos_NULL;
		}
		else {
			(*pos)++; // прочитали закрывающую скобку и сдвинулись на следующее слово
			// запишем "subshell" в nodePtr->argv[0], чтобы поле argv не оставалось пустым
			if ( (nodePtr->argv = (char **) realloc(nodePtr->argv, 2 * sizeof(char *))) == NULL ) {
				fprintf(stderr, "%s<ERROR>%s makeTree_4: realloc: argv: %s\n", RED, RESET, strerror(errno));
				syntax_error = 1;
				*pos = pos_NULL;
			}
			insertInLast(nodePtr->argv, "subshell");
		}
	}
	else {
		int getmem = 1;
		while (vector[*pos] != NULL && !cmpSpecSym(vector[*pos])) {
			// выделить 1 блок памяти для указателя на слово
			if ( (nodePtr->argv = (char **) realloc(nodePtr->argv, ++getmem * sizeof(char *))) == NULL ) {
				fprintf(stderr, "%s<ERROR>%s makeTree_4: realloc: argv: %s\n", RED, RESET, strerror(errno));
				syntax_error = 1;	// error; это не синтаксическая ошибка, но так мы вернемся в shell.c
				*pos = pos_NULL;	// сообщение об ошибке уже выдано; в shell.c освободим память, выделенную под всё дерево
			}
			// закрепить указатель на слов в массиве
			insertInLast(nodePtr->argv, vector[*pos]);
			(*pos)++; // шаг цикла. сдвинулись на следующее слово
		}
		if (nodePtr->argv[0] == NULL) { // пропущено имя утилиты
			fprintf(stderr, "shell: syntax error: no command to execute\n");
			syntax_error = 1; // error
			*pos = pos_NULL;
		}
	}

	return nodePtr;
}

void tick_backgrnd(TREE nodePtr)
{
	if (nodePtr == NULL) return;

	nodePtr->backgrnd = 1;
	tick_backgrnd(nodePtr->pipe);
}

void destruction_tree(TREE *treePtr)
{
	if (*treePtr == NULL) return;

	free( (*treePtr)->argv );
	destruction_tree( &(*treePtr)->psubcmd );
	destruction_tree( &(*treePtr)->pipe    );
	destruction_tree( &(*treePtr)->next    );
	free(*treePtr);
	*treePtr = NULL;
}

void printTree(TREE rootPtr)
{
	int i = 0;
	static int j = -indentSize;

	if (rootPtr == NULL)
		return;

	j += indentSize;

	makeident(j);
	printf("-----> %p\n", rootPtr);
	makeident(j);
	printf("argv =\n");
	do {
		makeident(j);
		printf("       %s\n", rootPtr->argv[i]);
	}
	while (rootPtr->argv[i++] != NULL);
	makeident(j);
	printf("infile     = %s\n", rootPtr->infile);
	makeident(j);
	printf("outfile    = %s\n", rootPtr->outfile);
	makeident(j);
	printf("out_type   = %d\n", rootPtr->iotp);
	makeident(j);
	printf("background = %d\n", rootPtr->backgrnd);
	makeident(j);
	printf("cmd_type   = %d\n", rootPtr->type);
	makeident(j);
	printf("psubcmd    = %p\n", rootPtr->psubcmd);
	makeident(j);
	printf("pipe       = %p\n", rootPtr->pipe);
	makeident(j);
	printf("next       = %p\n", rootPtr->next);
	putchar('\n');

	if (rootPtr->psubcmd != NULL)
		printTree(rootPtr->psubcmd);
	if (rootPtr->pipe != NULL)
		printTree(rootPtr->pipe);
	if (rootPtr->next != NULL)
		printTree(rootPtr->next);

	j -= indentSize;
}

void makeident(int j)
{
	for (; j > 0; j--)
		putchar(' ');
}