/* reader.c */

extern char *cache;
extern int cinc;
extern int cache_size;
extern int ntags;
extern int tagmax;
extern char **tag_table;
extern char saw_eof;
extern char unionized;
extern char *cptr;
extern char *line;
extern int linesize;
extern bucket *goal;
extern int prec;
extern int gensym;
extern char last_was_action;
extern int maxitems;
extern bucket **pitem;
extern int maxrules;
extern bucket **plhs;
extern int name_pool_size;
extern char *name_pool;
extern char line_format[];

/* functions */
extern void cachec(int c);
extern void get_line(void);
extern char *dup_line(void);
extern void skip_comment(void);
extern int nextc(void);
extern int keyword(void);
extern void copy_ident(void);
extern void copy_text(void);
extern void copy_union(void);
extern int hexval(int c);
extern bucket *get_literal(void);
extern int is_reserved(char *name);
extern bucket *get_name(void);
extern int get_number(void);
extern char *get_tag(void);
extern void declare_tokens(int assoc);
extern void declare_types(void);
extern void declare_start(void);
extern void read_declarations(void);
extern void initialize_grammar(void);
extern void expand_items(void);
extern void expand_rules(void);
extern void advance_to_start(void);
extern void start_rule(bucket *bp, int s_lineno);
extern void end_rule(void);
extern void insert_empty_rule(void);
extern void add_symbol(void);
extern void copy_action(void);
extern int mark_symbol(void);
extern void read_grammar(void);
extern void free_tags(void);
extern void pack_names(void);
extern void check_symbols(void);
extern void pack_symbols(void);
extern void pack_grammar(void);
extern void print_grammar(void);
extern void reader(void);
