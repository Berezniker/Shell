#define indentSize 11 /* number of spaces indented when tree is displayed */

typedef enum {NUL, NXT, AND, OR} next_type;
typedef enum {MISS, NEW, CONT} out_type;

struct cmd_inf {
	char			**argv;		/* список из имени команды и аргументов									*/
	char			*infile;	/* переназначенный файл стандартного вывода								*/
	char			*outfile;	/* переназначенный файл стандартного вывода								*/
	out_type		iotp;		/* MISS - нет перенаправления в файл, NEW - ">", CONT - ">>"			*/
	int				backgrnd;	/* = 1, если команда подлежит выполнению в фоновом режиме; иначе = 0	*/
	struct cmd_inf	*psubcmd;	/* команды для запуска в дочернем shell									*/
	struct cmd_inf	*pipe;		/* следующая команда после “|”											*/
	struct cmd_inf	*next;		/* следующая команда после “;” (или после “&”, “&&”, “||”)				*/
	next_type		type;		/* NUL - нет следующей команды, NXT - ';', AND - '&&', OR - '||'		*/
};
typedef struct cmd_inf TREENODE;
typedef TREENODE *TREENODEPTR;
typedef TREENODEPTR TREE;

/**
 * [<...>_lvl: recursive descent tree construction]
 */
TREENODEPTR first_lvl (char **vector, int *pos, int vecSize);	/* ';', '&', '||', '&&' */
TREENODEPTR second_lvl(char **vector, int *pos);				/* '|'					*/
TREENODEPTR third_lvl (char **vector, int *pos);				/* '<', '>>', '>'		*/
TREENODEPTR fourth_lvl(char **vector, int *pos);				/* '(', ')'				*/

/**
 * [tick_backgrnd: puts 1 in the backgrnd field]
 */
void tick_backgrnd(TREE nodePtr);

/**
 * [destruction_tree: deletes the TREE and its elements]
 */
void destruction_tree(TREE *treePtr);

/**
 * [printTree: TREE printing]
 */
void printTree(TREE rootPtr);

/**
 * [makeident: prints j spaces]
 */
void makeident(int j);