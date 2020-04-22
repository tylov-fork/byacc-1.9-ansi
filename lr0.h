/* lr0.c */

extern int nstates;
extern core *first_state;
extern shifts *first_shift;
extern reductions *first_reduction;

extern void allocate_itemsets(void);
extern void allocate_storage(void);
extern void append_states(void);
extern void free_storage(void);
extern void generate_states(void);
extern int get_state(int symbol);
extern void initialize_states(void);
extern void new_itemsets(void);
extern core *new_state(int symbol);
extern void show_cores(void);
extern void show_ritems(void);
extern void show_rrhs(void);
extern void show_shifts(void);
extern void save_shifts(void);
extern void save_reductions(void);
extern void set_derives(void);
extern void free_derives(void);
extern void set_nullable(void);
extern void free_nullable(void);
extern void lr0(void);
