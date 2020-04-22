/* output.c */

extern void output(void);
extern void output_prefix(void);
extern void output_rule_data(void);
extern void output_yydefred(void);
extern void output_actions(void);
extern void token_actions(void);
extern void goto_actions(void);
extern int default_goto(int symbol);
extern void save_column(int symbol, int default_state);
extern void sort_actions(void);
extern void pack_table(void);
extern int matching_vector(int vector);
extern int pack_vector(int vector);
extern void output_base(void);
extern void output_table(void);
extern void output_check(void);
extern int is_C_identifier(char *name);
extern void output_defines(void);
extern void output_stored_text(void);
extern void output_debug(void);
extern void output_stype(void);
extern void output_trailing_text(void);
extern void output_semantic_actions(void);
extern void free_itemsets(void);
extern void free_shifts(void);
extern void free_reductions(void);
