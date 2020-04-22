/* verbose.c */

extern void verbose(void);
extern void log_unused(void);
extern void log_conflicts(void);
extern void print_state(int state);
extern void print_conflicts(int state);
extern void print_core(int state);
extern void print_nulls(int state);
extern void print_actions(int stateno);
extern void print_shifts(action *p);
extern void print_reductions(action *p, int defred);
extern void print_gotos(int stateno);
