/* lalr.c */

extern int tokensetsize;
extern short *lookaheads;
extern short *LAruleno;
extern unsigned *LA;
extern short *accessing_symbol;
extern core **state_table;
extern shifts **shift_table;
extern reductions **reduction_table;
extern short *goto_map;
extern short *from_state;
extern short *to_state;

extern void lalr(void);

