#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <malloc.h>
#include <limits.h>

#define CONCAT(first,second)    first #second
#define CONCAT1(string,number)  CONCAT(string, number)
#define CONCAT2(first,second)   #first "." #second

/*  machine-dependent definitions                       */
/*  the following definitions are for the Tahoe         */
/*  they might have to be changed for other machines    */
/*                                                      */
/*  MAXCHAR is the largest unsigned character value     */
/*  MAXYYINT is the largest value of a Value_t          */
/*  MINYYINT is the most negative value of a Value_t    */
/*  MAXTABLE is the maximum table size                  */
/*  BITS_PER_WORD is the number of bits in a C unsigned */
/*  WORDSIZE computes the number of words needed to     */
/*  store n bits                                        */
/*  BIT returns the value of the n-th bit starting      */
/*  from r (0-indexed)                                  */
/*  SETBIT sets the n-th bit starting from r            */

#define	MAXCHAR		UCHAR_MAX
#ifndef MAXTABLE
#define MAXTABLE	32500
#endif
#if MAXTABLE <= SHRT_MAX
#define YYINT		short
#define MAXYYINT	SHRT_MAX
#define MINYYINT	SHRT_MIN
#elif MAXTABLE <= INT_MAX
#define YYINT		int
#define MAXYYINT	INT_MAX
#define MINYYINT	INT_MIN
#else
#error "MAXTABLE is too large for this machine architecture!"
#endif

#define BITS_PER_WORD	  ((int) sizeof (unsigned) * CHAR_BIT)
#define WORDSIZE(n)	      (((n)+(BITS_PER_WORD-1))/BITS_PER_WORD)
#define BIT(r, n)	      ((((r)[(n)/BITS_PER_WORD])>>((n)&(BITS_PER_WORD-1)))&1)
#define SETBIT(r, n)	  ((r)[(n)/BITS_PER_WORD]|=((unsigned)1<<((n)&(BITS_PER_WORD-1))))


/*  character names  */

#define NUL               '\0'    /*  the null character  */
#define NEWLINE           '\n'    /*  line feed  */
#define SP                ' '     /*  space  */
#define BS                '\b'    /*  backspace  */
#define HT                '\t'    /*  horizontal tab  */
#define VT                '\013'  /*  vertical tab  */
#define CR                '\r'    /*  carriage return  */
#define FF                '\f'    /*  form feed  */
#define QUOTE             '\''    /*  single quote  */
#define DOUBLE_QUOTE      '\"'    /*  double quote  */
#define BACKSLASH         '\\'    /*  backslash  */


/* defines for constructing filenames */

#define CODE_SUFFIX       ".code.c"
#define DEFINES_SUFFIX    ".tab.h"
#define OUTPUT_SUFFIX     ".tab.c"
#define VERBOSE_SUFFIX    ".output"


/* keyword codes */

enum KeyCodes
{
    TOKEN = 0,
    LEFT,
    RIGHT,
    NONASSOC,
    MARK,
    TEXT,
    TYPE,
    START,
    UNION,
    IDENT,
};

/*  symbol classes  */

enum SymCodes
{
    UNKNOWN = 0,
    TERM,
    NONTERM,
};

/*  the undefined value  */

enum
{
    UNDEFINED = -1
};


/*  action codes  */

enum ActCodes
{
    SHIFT = 1,
    REDUCE = 2,
};


/*  character macros  */

#define IS_IDENT(c)       (isalnum(c) || (c) == '_' || (c) == '.' || (c) == '$')
#define IS_OCTAL(c)       ((c) >= '0' && (c) <= '7')
#define NUMERIC_VALUE(c)  ((c) - '0')


/*  symbol macros  */

#define ISTOKEN(s)        ((s) < start_symbol)
#define ISVAR(s)          ((s) >= start_symbol)


/*  storage allocation macros  */

#define CALLOC(k,n)       (calloc((unsigned)(k),(unsigned)(n)))
#define TCALLOC(t,k,n)    ((t*)calloc((unsigned)(k),(unsigned)(n)))
#define FREE(x)           (free((char*)(x)))
#define MALLOC(n)         (malloc((unsigned)(n)))
#define TMALLOC(t,n)      ((t*)malloc((unsigned)(n)))
#define NEW(t)            ((t*)allocate(sizeof(t)))
#define NEW2(n,t)         ((t*)allocate((unsigned)((n)*sizeof(t))))
#define REALLOC(p,n)      (realloc((char*)(p),(unsigned)(n)))
#define TREALLOC(t,p,n)   ((t*)realloc((char*)(p),(unsigned)(n)))

typedef char Assoc_t;
typedef char Class_t;
typedef YYINT Index_t;
typedef YYINT Value_t;

/*  the structure of a symbol table entry  */

typedef struct bucket bucket;
struct bucket
{
    struct bucket *link;
    struct bucket *next;
    char *name;
    char *tag;
    Value_t value;
    Index_t index;
    Value_t prec;
    Class_t class;
    Assoc_t assoc;
};

/*  the structure of the LR(0) state machine  */

typedef struct core core;
struct core
{
    struct core *next;
    struct core *link;
    Value_t number;
    Value_t accessing_symbol;
    Value_t nitems;
    Value_t items[1];
};

/*  the structure used to record shifts  */

typedef struct shifts shifts;
struct shifts
{
    struct shifts *next;
    Value_t number;
    Value_t nshifts;
    Value_t shift[1];
};


/*  the structure used to store reductions  */

typedef struct reductions reductions;
struct reductions
{
    struct reductions *next;
    Value_t number;
    Value_t nreds;
    Value_t rules[1];
};


/*  the structure used to represent parser actions  */

typedef struct action action;
struct action
{
    struct action *next;
    Value_t symbol;
    Value_t number;
    Value_t prec;
    char action_code;
    Assoc_t assoc;
    char suppressed;
};


/* global variables */

extern char dflag;
extern char lflag;
extern char rflag;
extern char tflag;
extern char vflag;
extern char *file_prefix;
extern char *symbol_prefix;

extern char *myname;
extern char *cptr;
extern char *line;
extern int lineno;
extern int outline;

extern char *banner[];
extern char *tables[];
extern char *header[];
extern char *body[];
extern char *trailer[];

extern char *action_file_name;
extern char *code_file_name;
extern char *defines_file_name;
extern char *input_file_name;
extern char *output_file_name;
extern char *text_file_name;
extern char *union_file_name;
extern char *verbose_file_name;

extern FILE *action_file;
extern FILE *code_file;
extern FILE *defines_file;
extern FILE *input_file;
extern FILE *output_file;
extern FILE *text_file;
extern FILE *union_file;
extern FILE *verbose_file;

extern int nitems;
extern int nrules;
extern int nsyms;
extern int ntokens;
extern int nvars;

extern Value_t start_symbol;
extern char **symbol_name;
extern Value_t *symbol_value;
extern Value_t *symbol_prec;
extern char *symbol_assoc;

extern Value_t *ritem;
extern Value_t *rlhs;
extern Value_t *rrhs;
extern Value_t *rprec;
extern Assoc_t *rassoc;

extern Value_t **derives;
extern char *nullable;

/* global functions */

extern void done(int k);
extern char *allocate(unsigned n);

