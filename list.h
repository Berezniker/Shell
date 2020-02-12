#define GET_MEMORY 64 /* the number of allocated blocks when memory is insufficient */

/**
 * [getword: forms the word from stdin]
 * @return [word; NULL - in case of new string or end of file]
 */
char *getword();

/**
 * [check_memory: checks the memory sufficiency in the str]
 * @param  strCount [amount of memory occupied]
 * @param  strSize  [number of allocated memory]
 */
void check_memory(char **str, int strCount, int *strSize);

/**
 * [replaceVar: substitutes variables with $ in line str]
 */
void replaceVar(char **str, int *strCount, int *strSize);

/**
 * [insertInLast: inserts the word to the end of the array]
 */
void insertInLast(char **array, char *word);

/**
 * [printList: prints the array and its size into stdout]
 */
void printList(char **array, int size);

/**
 * [destruction: deletes the array and its elements]
 */
void destruction_list(char **array);

/**
 * @param  c [character to check]
 * @return   [1 - ';', '<', '(', ')', '|', '&', '>'; 0 - otherwise]
 */
int isSpecSym(char c);

/**
 * @param  s [string to check]
 * @return   [1 - ';', '<', '(', ')', '|', '||, '&', '&&', '>', '>>; 0 - otherwise]
 */
int cmpSpecSym(char *s);

/**
 * @param  c [character to check]
 * @return   [1 - ';', '<', '(', ')'; 0 - otherwise]
 */
int is1SpecSym(char c);

/**
 * @param  c [character to check]
 * @return   [1 - '|', '&', '>'; 0 - otherwise]
 */
int is2SpecSym(char c);

/**
 * @param  c [character to check]
 * @return   [1 - whitespace BUT NOT '\n'; 0 - otherwise]
 */
int _isspace(char c);