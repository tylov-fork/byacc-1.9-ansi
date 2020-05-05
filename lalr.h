/* lalr.c */

extern int tokensetsize;
extern Value_t *lookaheads;
extern Value_t *LAruleno;
extern unsigned *LA;
extern Value_t *accessing_symbol;
extern core **state_table;
extern shifts **shift_table;
extern reductions **reduction_table;
extern Value_t *goto_map;
extern Value_t *from_state;
extern Value_t *to_state;

extern void lalr(void);

