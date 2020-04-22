/* symtab.c */

extern bucket **symbol_table;
extern bucket *first_symbol;
extern bucket *last_symbol;

extern int hash(char *name);
extern bucket *make_bucket(char *name);
extern bucket *lookup(char *name);
extern void create_symbol_table(void);
extern void free_symbol_table(void);
extern void free_symbols(void);
