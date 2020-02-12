#include <stdio.h>		// printf(), fprintf(), sprintf(), getchar(), ungetc()
#include <stdlib.h>		// realloc(), free(), getenv()
#include <string.h>		// strerror(), strcmp()
#include <setjmp.h>		// longjmp()
#include <unistd.h>		// getuid()
#include <ctype.h>		// isspace(), isalnum()
#include <errno.h>		// errno
#include "list.h"
#include "color.h"

extern jmp_buf begin; /* метка для возвращения в shell.c при возникновении ошибки */

char *getword()
{
	static int	c, newline = 0, slash = 0, bracket = 0;
	int			space = 0, q1 = 0, q2 = 0, quote = 0, strCount = 0, strSize = 0;
	char		*str = NULL; // строка, в которой формируем слово

	while (1) {
		if (!newline) // newline = 1, для корректного завершения предыдущего слова 
			c = getchar();

		if (space && strCount != 0) { // до этого была ПП и слово уже начали формировать
			ungetc(c, stdin); // вернуть символ обратно
			break;
		}
		if (!slash) { // '\' - экранирует \", \', \(, \), \#
			if ( ((c == '\"') && !q1) || ((c == '\'') && !q2) ) {
				q2 = (c == '\"') ^ q2;	// q2 = 1, если встретилась открывающая двойная кавычка
				q1 = (c == '\'') ^ q1;	// q1 = 1, если встретилась открывающая одинарная кавычка
				quote = q1 | q2;		// quote - флаг наличия открывающей кавычки
				continue; // кавычки не записываем
				// НО! кавычки внутри кавычек записываются за счёт условия проверки if
			}

			if (c == '(' && strCount == 0) 
				bracket++;
			if (c == ')' && strCount == 0) 
				bracket--;
			if (bracket < 0) // точно ошибка
				while ( (c = getchar()) != '\n' && c != EOF); // пропуск символов до окончания ввода

			if (c == '#' && strCount == 0) // комментарий: только если '#' стоит в начале слова
				while ( (c = getchar()) != '\n' && c != EOF); // пропуск символов до окончания ввода
		}
		if (c == EOF || c == '\n') {
			if (quote || bracket) { // quote = 1     => не встретилась закрывающая " или '
				free(str);			// bracket != 0  => не встретилась '(' или ')'
				fprintf(stderr, "shell: syntax error: expected %c\n", q1 ? '\'' : q2 ? '\"' : bracket > 0 ? ')' : '(');
				bracket = 0;
				longjmp(begin, 1);
			}

			newline = strCount ? 1 : 0;

			if (strCount == 0) // конец ввода
				return NULL;
			else // strCount != 0 => завершить слово
				break;
		}

		if ( !slash && !quote && _isspace(c) ) { // если quote = 1, т.е. идёт запись в "..." ('...') => не реагировать на пробелы 
			space = 1;
			continue;
		}
		space = 0;

		// добавить памяти для строки
		check_memory(&str, strCount, &strSize);

		if (c == '\\' && !quote) { // обработка '\\'
			if (slash) {
				*(str + strCount++) = c;
				slash = 0;
			}
			else
				slash = 1;
			continue;
		}

		if (c == '$' && !slash && !q1)
			replaceVar(&str, &strCount, &strSize);	// замена переменной
		else if (!isSpecSym(c) || quote || slash)	// с - любой символ, отличный от специального, или идёт запись в "..."
			*(str + strCount++) = c;				// или экранирование => записываем в строку и переходим на новый шаг цикла
		else { // c - спец. символ
			if (strCount != 0) // уже формировали слово => завершаем
				ungetc(c, stdin); // вернуть символ в поток, чтобы потом вновь считать
			else { // strCount == 0, т.е. строка пустая
				*(str + strCount++) = c; // записали спец. символ
				if (is2SpecSym(c)) { // c - повторный спец. символ
					// посмотрим следующий символ:
					if ( (c = getchar()) == *(str + strCount - 1))
						*(str + strCount++) = c;
					else
						ungetc(c, stdin);
				}
			} // завершили обработку спец. символов
			break;
		}
		slash = c == '\\'; // если встретился '\', то slash = 1 на весь следующий шаг цикла
	} // end while(1)

	// завершаем слово и обрезаем память:
	*(str + strCount++) = '\0';
	if ( (str = (char *) realloc(str, strCount * sizeof(char))) == NULL) {
		fprintf(stderr, "%s<ERROR>%s getword: realloc: %s\n", RED, RESET, strerror(errno));
		longjmp(begin, 1);
	}

	return str;
}

void check_memory(char **str, int strCount, int *strSize)
{
	if (strCount + 1 >= *strSize) { // +1 - чтобы всегда было место для заверщающего '\0'
		*strSize += GET_MEMORY;
		if ( (*str = (char *) realloc (*str, *strSize * sizeof(char))) == NULL) { // обработка ошибки
			fprintf(stderr, "%s<ERROR>%s getword: realloc: %s\n", RED, RESET, strerror(errno));
			longjmp(begin, 1);
		}
	}	
}

void replaceVar(char **str, int *strCount, int *strSize)
{
	int  c, i = 0, bufCount = 0, bufSize = 0, id;
	char *buf = NULL;	// для хранения введенных символов - на вывод не подается
	char *env = NULL;	// для получения значения переменной окружения
	char uid[12];		// для записи uid пользователя
	
	while ( isalnum(c = getchar()) ) {
		// выделение памяти:
		if (bufCount + 1 >= bufSize) { // +1 - чтобы всегда было место для заверщающего '\0'
			bufSize += GET_MEMORY;
			if ( (buf = (char *) realloc(buf, bufSize * sizeof(char))) == NULL) { // обработка ошибки
				free(*str);
				fprintf(stderr, "%s<ERROR>%s replaceVar: realloc: %s\n", RED, RESET, strerror(errno));
				longjmp(begin, 1);
			}
		}
		buf[bufCount++] = c; // записываем символ
	}
	ungetc(c, stdin); // вернули последний символ в поток, т.к. он не идёт на запись в buf

	if (bufCount == 0)
		return;

	buf[bufCount++] = '\0'; // завершаем слово
	if ( (buf = (char *) realloc(buf, bufCount * sizeof(char))) == NULL) { // обрезаем память
		free(*str);
		fprintf(stderr, "%s<ERROR>%s replaceVar: realloc: %s\n", RED, RESET, strerror(errno));
		longjmp(begin, 1);
	}

	if ( (env = getenv(buf)) != NULL) {
		do { // нашли совпадение, переписываем env в str
			check_memory(str, *strCount, strSize); // добавить памяти
			str[0][(*strCount)++] = env[i++];
		} while (env[i] != '\0');
	}
	else if (!strcmp(buf, "EUID")) { // частный случай
		id = getuid();
		sprintf(uid, "%d", id);
		do {
			check_memory(str, *strCount, strSize); // добавить памяти
			str[0][(*strCount)++] = uid[i++];
		} while (uid[i] != '\0');
	}
	free(buf);
}

void insertInLast(char **str, char *word)
{
	while (*str != NULL)
		(void)*str++;
	*str++ = word; 
	*str = NULL;
}

void printList(char **str, int size)
{
	printf("List length = %d\n", size);

	while (*str != NULL)
		printf("%s\n", *str++);
}

void destruction_list(char **str)
{
	int i = 0;

	if (str == NULL) return;

	while (*(str + i) != NULL)
		free(*(str + i++));
	free(str);
}

int isSpecSym(char c)
{
	return is1SpecSym(c) || is2SpecSym(c);
}

int cmpSpecSym(char *s)
{
	return	!strcmp(s, "(") || !strcmp(s, ")") || !strcmp(s, ";")  || !strcmp(s, "&")  || !strcmp(s, "|")  || 
			!strcmp(s, "<") || !strcmp(s, ">") || !strcmp(s, ">>") || !strcmp(s, "&&") || !strcmp(s, "||");
}

int is1SpecSym(char c)
{
	return c == ';' || c == '<' || c == '(' || c == ')';
}

int is2SpecSym(char c)
{
	return c == '|' || c == '&' || c == '>';
}

int _isspace(char c)
{
	return isspace(c) && c != '\n';
}