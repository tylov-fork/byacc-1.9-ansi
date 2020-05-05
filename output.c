#include <string.h>
#include "defs.h"
#include "error.h"
#include "skeleton.h"
#include "lr0.h"
#include "lalr.h"
#include "mkpar.h"
#include "reader.h"
#include "output.h"

#define StaticOrR	(rflag ? "" : "static ")
#define CountLine(fp)   (!rflag || ((fp) == code_file))

static int nvectors;
static int nentries;
static Value_t **froms;
static Value_t **tos;
static Value_t *tally;
static Value_t *width;
static Value_t *state_count;
static Value_t *order;
static Value_t *base;
static Value_t *pos;
static int maxtable;
static Value_t *table;
static Value_t *check;
static int lowzero;
static int high;

static void output_prefix(void);
static void output_rule_data(void);
static void output_yydefred(void);
static void output_actions(void);
static void token_actions(void);
static void goto_actions(void);
static int default_goto(int symbol);
static void save_column(int symbol, int default_state);
static void sort_actions(void);
static void pack_table(void);
static int matching_vector(int vector);
static int pack_vector(int vector);
static void output_base(void);
static void output_table(void);
static void output_check(void);
static int is_C_identifier(char *name);
static void output_defines(void);
static void output_stored_text(void);
static void output_debug(void);
static void output_stype(void);
static void output_trailing_text(void);
static void output_semantic_actions(void);
static void free_itemsets(void);
static void free_shifts(void);
static void free_reductions(void);



void
output(void)
{
    free_itemsets();
    free_shifts();
    free_reductions();
    output_prefix();
    output_stored_text();
    output_defines();
    output_rule_data();
    output_yydefred();
    output_actions();
    free_parser();
    output_debug();
    output_stype();
    if (rflag) write_section(tables);
    write_section(header);
    output_trailing_text();
    write_section(body);
    output_semantic_actions();
    write_section(trailer);
}

static void
putc_code(FILE * fp, int c)
{
    if ((c == '\n') && (fp == code_file))
        ++outline;
    putc(c, fp);
}

static void
write_code_lineno(FILE * fp)
{
    if (!lflag && (fp == code_file))
    {
        ++outline;
        fprintf(fp, line_format, outline + 1, code_file_name);
    }
}

static void
write_input_lineno(void)
{
    if (!lflag)
    {
        ++outline;
        fprintf(code_file, line_format, lineno, input_file_name);
    }
}

static void
output_int(int value)
{
    fprintf(output_file, "%5d,", value);
}

static void
output_newline(void)
{
    if (!rflag)
        ++outline;
    putc('\n', output_file);
}

static void
output_line(const char *value)
{
    fputs(value, output_file);
    output_newline();
}

static void
start_int_table(const char *name, int value)
{
    int need = 34 - (int)(strlen(symbol_prefix) + strlen(name));

    if (need < 6)
        need = 6;
    fprintf(output_file,
            "%sconst YYINT %s%s[] = {%*d,",
            StaticOrR, symbol_prefix, name, need, value);
}

static void
end_table(void)
{
    output_newline();
    output_line("};");
}


static void
define_prefixed(const char *name)
{
    ++outline;
    fprintf(code_file, "\n");

    ++outline;
    fprintf(code_file, "#ifndef %s\n", name);

    ++outline;
    fprintf(code_file, "#define %-10s %s%s\n", name, symbol_prefix, name + 2);

    ++outline;
    fprintf(code_file, "#endif /* %s */\n", name);
}

static void
output_prefix(void)
{
    if (symbol_prefix == NULL)
        symbol_prefix = "yy";
    else
    {
        define_prefixed("yyparse");
        define_prefixed("yylex");
        define_prefixed("yyerror");
        define_prefixed("yychar");
        define_prefixed("yyval");
        define_prefixed("yylval");
        define_prefixed("yydebug");
        define_prefixed("yynerrs");
        define_prefixed("yyerrflag");
        define_prefixed("yylhs");
        define_prefixed("yylen");
        define_prefixed("yydefred");
        define_prefixed("yydgoto");
        define_prefixed("yysindex");
        define_prefixed("yyrindex");
        define_prefixed("yygindex");
        define_prefixed("yytable");
        define_prefixed("yycheck");
        define_prefixed("yyname");
        define_prefixed("yyrule");
    }
    ++outline;
    fprintf(code_file, "#define YYPREFIX \"%s\"\n", symbol_prefix);
}

static void
output_YYINT_typedef(FILE * fp)
{
    /* generate the type used to index the various parser tables */
    if (!rflag) ++outline;
    fprintf(fp, "typedef %s YYINT;\n", CONCAT1("", YYINT));
}

static void
output_rule_data(void)
{
    int i;
    int j;

    output_YYINT_typedef(output_file);

    start_int_table("lhs", symbol_value[start_symbol]);

    j = 10;
    for (i = 3; i < nrules; i++)
    {
        if (j >= 10)
        {
            output_newline();
            j = 1;
        }
        else
            ++j;

        output_int(symbol_value[rlhs[i]]);
    }
    end_table();

    start_int_table("len", 2);

    j = 10;
    for (i = 3; i < nrules; i++)
    {
        if (j >= 10)
        {
            output_newline();
            j = 1;
        }
        else
            j++;

        output_int(rrhs[i + 1] - rrhs[i] - 1);
    }
    end_table();
}

static void
output_yydefred(void)
{
    int i, j;

    start_int_table("defred", (defred[0] ? defred[0] - 2 : 0));

    j = 10;
    for (i = 1; i < nstates; i++)
    {
        if (j < 10)
            ++j;
        else
        {
            output_newline();
            j = 1;
        }

        output_int((defred[i] ? defred[i] - 2 : 0));
    }

    end_table();
}

static void
output_actions(void)
{
    nvectors = 2*nstates + nvars;

    froms = NEW2(nvectors, Value_t *);
    tos = NEW2(nvectors, Value_t *);
    tally = NEW2(nvectors, Value_t);
    width = NEW2(nvectors, Value_t);

    token_actions();
    FREE(lookaheads);
    FREE(LA);
    FREE(LAruleno);
    FREE(accessing_symbol);

    goto_actions();
    FREE(goto_map + ntokens);
    FREE(from_state);
    FREE(to_state);

    sort_actions();
    pack_table();
    output_base();
    output_table();
    output_check();
}

static void
token_actions(void)
{
    int i, j;
    int shiftcount, reducecount;
    int max, min;
    Value_t *actionrow, *r, *s;
    action *p;

    actionrow = NEW2(2*ntokens, Value_t);
    for (i = 0; i < nstates; ++i)
    {
        if (parser[i])
        {
            for (j = 0; j < 2*ntokens; ++j)
            actionrow[j] = 0;

            shiftcount = 0;
            reducecount = 0;
            for (p = parser[i]; p; p = p->next)
            {
                if (p->suppressed == 0)
                {
                    if (p->action_code == SHIFT)
                    {
                        ++shiftcount;
                        actionrow[p->symbol] = p->number;
                    }
                    else if (p->action_code == REDUCE && p->number != defred[i])
                    {
                        ++reducecount;
                        actionrow[p->symbol + ntokens] = p->number;
                    }
                }
            }

            tally[i] = shiftcount;
            tally[nstates+i] = reducecount;
            width[i] = 0;
            width[nstates+i] = 0;
            if (shiftcount > 0)
            {
                froms[i] = r = NEW2(shiftcount, Value_t);
                tos[i] = s = NEW2(shiftcount, Value_t);
                min = MAXYYINT;
                max = 0;
                for (j = 0; j < ntokens; ++j)
                {
                    if (actionrow[j])
                    {
                        if (min > symbol_value[j])
                            min = symbol_value[j];
                        if (max < symbol_value[j])
                            max = symbol_value[j];
                        *r++ = symbol_value[j];
                        *s++ = actionrow[j];
                    }
                }
                width[i] = max - min + 1;
            }
            if (reducecount > 0)
            {
                froms[nstates+i] = r = NEW2(reducecount, Value_t);
                tos[nstates+i] = s = NEW2(reducecount, Value_t);
                min = MAXYYINT;
                max = 0;
                for (j = 0; j < ntokens; ++j)
                {
                    if (actionrow[ntokens+j])
                    {
                        if (min > symbol_value[j])
                            min = symbol_value[j];
                        if (max < symbol_value[j])
                            max = symbol_value[j];
                        *r++ = symbol_value[j];
                        *s++ = actionrow[ntokens+j] - 2;
                    }
                }
                width[nstates+i] = max - min + 1;
            }
        }
    }
    FREE(actionrow);
}


static void
goto_actions(void)
{
    int i, j, k;

    state_count = NEW2(nstates, Value_t);

    k = default_goto(start_symbol + 1);
    start_int_table("dgoto", k);
    save_column(start_symbol + 1, k);

    j = 10;
    for (i = start_symbol + 2; i < nsyms; i++)
    {
        if (j >= 10)
        {
            output_newline();
            j = 1;
        }
        else
            ++j;

        k = default_goto(i);
        output_int(k);
        save_column(i, k);
    }

    end_table();
    FREE(state_count);
}

static int
default_goto(int symbol)
{
    int i;
    int m;
    int n;
    int default_state;
    int max;

    m = goto_map[symbol];
    n = goto_map[symbol + 1];

    if (m == n) return (0);

    for (i = 0; i < nstates; i++)
        state_count[i] = 0;

    for (i = m; i < n; i++)
        state_count[to_state[i]]++;

    max = 0;
    default_state = 0;
    for (i = 0; i < nstates; i++)
    {
        if (state_count[i] > max)
        {
            max = state_count[i];
            default_state = i;
        }
    }

    return (default_state);
}

static void
save_column(int symbol, int default_state)
{
    int i;
    int m;
    int n;
    Value_t *sp;
    Value_t *sp1;
    Value_t *sp2;
    int count;
    int symno;

    m = goto_map[symbol];
    n = goto_map[symbol + 1];

    count = 0;
    for (i = m; i < n; i++)
    {
        if (to_state[i] != default_state)
            ++count;
    }
    if (count == 0) return;

    symno = symbol_value[symbol] + 2*nstates;

    froms[symno] = sp1 = sp = NEW2(count, Value_t);
    tos[symno] = sp2 = NEW2(count, Value_t);

    for (i = m; i < n; i++)
    {
        if (to_state[i] != default_state)
        {
            *sp1++ = from_state[i];
            *sp2++ = to_state[i];
        }
    }

    tally[symno] = count;
    width[symno] = sp1[-1] - sp[0] + 1;
}

static void
sort_actions(void)
{
  int i;
  int j;
  int k;
  int t;
  int w;

  order = NEW2(nvectors, Value_t);
  nentries = 0;

  for (i = 0; i < nvectors; i++)
    {
      if (tally[i] > 0)
        {
          t = tally[i];
          w = width[i];
          j = nentries - 1;

          while (j >= 0 && (width[order[j]] < w))
            j--;

          while (j >= 0 && (width[order[j]] == w) && (tally[order[j]] < t))
            j--;

          for (k = nentries - 1; k > j; k--)
            order[k + 1] = order[k];

          order[j + 1] = i;
          nentries++;
        }
    }
}

static void
pack_table(void)
{
    int i;
    int place;
    int state;

    base = NEW2(nvectors, Value_t);
    pos = NEW2(nentries, Value_t);

    maxtable = 1000;
    table = NEW2(maxtable, Value_t);
    check = NEW2(maxtable, Value_t);

    lowzero = 0;
    high = 0;

    for (i = 0; i < maxtable; i++)
        check[i] = -1;

    for (i = 0; i < nentries; i++)
    {
        state = matching_vector(i);

        if (state < 0)
            place = pack_vector(i);
        else
            place = base[state];

        pos[i] = place;
        base[order[i]] = place;
    }

    for (i = 0; i < nvectors; i++)
    {
        if (froms[i])
            FREE(froms[i]);
        if (tos[i])
            FREE(tos[i]);
    }

    FREE(froms);
    FREE(tos);
    FREE(pos);
}


/*  The function matching_vector determines if the vector specified by        */
/*  the input parameter matches a previously considered        vector.  The        */
/*  test at the start of the function checks if the vector represents        */
/*  a row of shifts over terminal symbols or a row of reductions, or a        */
/*  column of shifts over a nonterminal symbol.  Berkeley Yacc does not        */
/*  check if a column of shifts over a nonterminal symbols matches a        */
/*  previously considered vector.  Because of the nature of LR parsing        */
/*  tables, no two columns can match.  Therefore, the only possible        */
/*  match would be between a row and a column.  Such matches are        */
/*  unlikely.  Therefore, to save time, no attempt is made to see if a        */
/*  column matches a previously considered vector.                        */
/*                                                                        */
/*  Matching_vector is poorly designed.  The test could easily be made        */
/*  faster.  Also, it depends on the vectors being in a specific        */
/*  order.                                                                */

static int
matching_vector(int vector)
{
    int i;
    int j;
    int k;
    int t;
    int w;
    int match;
    int prev;

    i = order[vector];
    if (i >= 2*nstates)
        return (-1);

    t = tally[i];
    w = width[i];

    for (prev = vector - 1; prev >= 0; prev--)
    {
        j = order[prev];
        if (width[j] != w || tally[j] != t)
            return (-1);

        match = 1;
        for (k = 0; match && k < t; k++)
        {
            if (tos[j][k] != tos[i][k] || froms[j][k] != froms[i][k])
                match = 0;
        }

        if (match)
            return (j);
    }

    return (-1);
}



static int
pack_vector(int vector)
{
    int i, j, k, l;
    int t;
    int loc;
    int ok;
    Value_t *from;
    Value_t *to;
    int newmax;

    i = order[vector];
    t = tally[i];
    assert(t);

    from = froms[i];
    to = tos[i];

    j = lowzero - from[0];
    for (k = 1; k < t; ++k)
        if (lowzero - from[k] > j)
            j = lowzero - from[k];
    for (;; ++j)
    {
        if (j == 0)
            continue;
        ok = 1;
        for (k = 0; ok && k < t; k++)
        {
            loc = j + from[k];
            if (loc >= maxtable)
            {
                if (loc >= MAXTABLE)
                    fatal("maximum table size exceeded");

                newmax = maxtable;
                do { newmax += 200; } while (newmax <= loc);
                table = (Value_t *) REALLOC(table, newmax*sizeof(Value_t));
                if (table == 0) no_space();
                check = (Value_t *) REALLOC(check, newmax*sizeof(Value_t));
                if (check == 0) no_space();
                for (l  = maxtable; l < newmax; ++l)
                {
                    table[l] = 0;
                    check[l] = -1;
                }
                maxtable = newmax;
            }

            if (check[loc] != -1)
                ok = 0;
        }
        for (k = 0; ok && k < vector; k++)
        {
            if (pos[k] == j)
                ok = 0;
        }
        if (ok)
        {
            for (k = 0; k < t; k++)
            {
                loc = j + from[k];
                table[loc] = to[k];
                check[loc] = from[k];
                if (loc > high) high = loc;
            }

            while (check[lowzero] != -1)
                ++lowzero;

            return (j);
        }
    }
}


static void
output_base(void)
{
    int i, j;

    fprintf(output_file, "short %ssindex[] = {%39d,", symbol_prefix, base[0]);

    j = 10;
    for (i = 1; i < nstates; i++)
    {
        if (j >= 10)
        {
            if (!rflag) ++outline;
            putc('\n', output_file);
            j = 1;
        }
        else
            ++j;

        fprintf(output_file, "%5d,", base[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\nshort %srindex[] = {%39d,", symbol_prefix,
            base[nstates]);

    j = 10;
    for (i = nstates + 1; i < 2*nstates; i++)
    {
        if (j >= 10)
        {
            if (!rflag) ++outline;
            putc('\n', output_file);
            j = 1;
        }
        else
            ++j;

        fprintf(output_file, "%5d,", base[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\nshort %sgindex[] = {%39d,", symbol_prefix,
            base[2*nstates]);

    j = 10;
    for (i = 2*nstates + 1; i < nvectors - 1; i++)
    {
        if (j >= 10)
        {
            if (!rflag) ++outline;
            putc('\n', output_file);
            j = 1;
        }
        else
            ++j;

        fprintf(output_file, "%5d,", base[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(base);
}


static void
output_table(void)
{
    int i;
    int j;

    ++outline;
    fprintf(code_file, "#define YYTABLESIZE %d\n", high);
    fprintf(output_file, "short %stable[] = {%40d,", symbol_prefix,
            table[0]);

    j = 10;
    for (i = 1; i <= high; i++)
    {
        if (j >= 10)
        {
            if (!rflag) ++outline;
            putc('\n', output_file);
            j = 1;
        }
        else
            ++j;

        fprintf(output_file, "%5d,", table[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(table);
}


static void
output_check(void)
{
    int i;
    int j;

    fprintf(output_file, "short %scheck[] = {%40d,", symbol_prefix,
            check[0]);

    j = 10;
    for (i = 1; i <= high; i++)
    {
        if (j >= 10)
        {
            if (!rflag) ++outline;
            putc('\n', output_file);
            j = 1;
        }
        else
            ++j;

        fprintf(output_file, "%5d,", check[i]);
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(check);
}


static int
is_C_identifier(char* name)
{
    char *s;
    int c;

    s = name;
    c = *s;
    if (c == '"')
    {
        c = *++s;
        if (!isalpha(c) && c != '_' && c != '$')
            return (0);
        while ((c = *++s) != '"')
        {
            if (!isalnum(c) && c != '_' && c != '$')
                return (0);
        }
        return (1);
    }

    if (!isalpha(c) && c != '_' && c != '$')
        return (0);
    while ((c = *++s))
    {
        if (!isalnum(c) && c != '_' && c != '$')
            return (0);
    }
    return (1);
}

static void
output_defines(void)
{
    int c, i;
    char *s;

    for (i = 2; i < ntokens; ++i)
    {
        s = symbol_name[i];
        if (is_C_identifier(s))
        {
            fprintf(code_file, "#define ");
            if (dflag) fprintf(defines_file, "#define ");
            c = *s;
            if (c == '"')
            {
                while ((c = *++s) != '"')
                {
                    putc(c, code_file);
                    if (dflag) putc(c, defines_file);
                }
            }
            else
            {
                do
                {
                    putc(c, code_file);
                    if (dflag) putc(c, defines_file);
                }
                while ((c = *++s));
            }
            ++outline;
            fprintf(code_file, " %d\n", symbol_value[i]);
            if (dflag) fprintf(defines_file, " %d\n", symbol_value[i]);
        }
    }

    ++outline;
    fprintf(code_file, "#define YYERRCODE %d\n", symbol_value[1]);

    if (dflag && unionized)
    {
        fclose(union_file);
        union_file = fopen(union_file_name, "r");
        if (union_file == NULL) open_error(union_file_name);
        while ((c = getc(union_file)) != EOF)
            putc(c, defines_file);
        fprintf(defines_file, " YYSTYPE;\nextern YYSTYPE %slval;\n",
                symbol_prefix);
    }
}

static void
output_stored_text(void)
{
    int c;
    FILE *in, *out;

    fclose(text_file);
    text_file = fopen(text_file_name, "r");
    if (text_file == NULL)
        open_error(text_file_name);
    in = text_file;
    if ((c = getc(in)) == EOF)
        return;
    out = code_file;
    if (c ==  '\n')
        ++outline;
    putc(c, out);
    while ((c = getc(in)) != EOF)
    {
        if (c == '\n')
            ++outline;
        putc(c, out);
    }
    if (!lflag)
        fprintf(out, line_format, ++outline + 1, code_file_name);
}

static void
output_debug(void)
{
    int i, j, k, max;
    char **symnam, *s;

    ++outline;
    fprintf(code_file, "#define YYFINAL %d\n", final_state);
    outline += 3;
    fprintf(code_file, "#ifndef YYDEBUG\n#define YYDEBUG %d\n#endif\n",
            tflag);
    if (rflag)
        fprintf(output_file, "#ifndef YYDEBUG\n#define YYDEBUG %d\n#endif\n",
                tflag);

    max = 0;
    for (i = 2; i < ntokens; ++i)
        if (symbol_value[i] > max)
            max = symbol_value[i];
    ++outline;
    fprintf(code_file, "#define YYMAXTOKEN %d\n", max);

    symnam = (char **) MALLOC((max+1)*sizeof(char *));
    if (symnam == 0) no_space();

    /* Note that it is  not necessary to initialize the element                */
    /* symnam[max].                                                        */
    for (i = 0; i < max; ++i)
        symnam[i] = 0;
    for (i = ntokens - 1; i >= 2; --i)
        symnam[symbol_value[i]] = symbol_name[i];
    symnam[0] = "end-of-file";

    if (!rflag) ++outline;
    fprintf(output_file, "#if YYDEBUG\nchar *%sname[] = {", symbol_prefix);
    j = 80;
    for (i = 0; i <= max; ++i)
    {
        if ((s = symnam[i]))
        {
            if (s[0] == '"')
            {
                k = 7;
                while (*++s != '"')
                {
                    ++k;
                    if (*s == '\\')
                    {
                        k += 2;
                        if (*++s == '\\')
                            ++k;
                    }
                }
                j += k;
                if (j > 80)
                {
                    if (!rflag) ++outline;
                    putc('\n', output_file);
                    j = k;
                }
                fprintf(output_file, "\"\\\"");
                s = symnam[i];
                while (*++s != '"')
                {
                    if (*s == '\\')
                    {
                        fprintf(output_file, "\\\\");
                        if (*++s == '\\')
                            fprintf(output_file, "\\\\");
                        else
                            putc(*s, output_file);
                    }
                    else
                        putc(*s, output_file);
                }
                fprintf(output_file, "\\\"\",");
            }
            else if (s[0] == '\'')
            {
                if (s[1] == '"')
                {
                    j += 7;
                    if (j > 80)
                    {
                        if (!rflag) ++outline;
                        putc('\n', output_file);
                        j = 7;
                    }
                    fprintf(output_file, "\"'\\\"'\",");
                }
                else
                {
                    k = 5;
                    while (*++s != '\'')
                    {
                        ++k;
                        if (*s == '\\')
                        {
                            k += 2;
                            if (*++s == '\\')
                                ++k;
                        }
                    }
                    j += k;
                    if (j > 80)
                    {
                        if (!rflag) ++outline;
                        putc('\n', output_file);
                        j = k;
                    }
                    fprintf(output_file, "\"'");
                    s = symnam[i];
                    while (*++s != '\'')
                    {
                        if (*s == '\\')
                        {
                            fprintf(output_file, "\\\\");
                            if (*++s == '\\')
                                fprintf(output_file, "\\\\");
                            else
                                putc(*s, output_file);
                        }
                        else
                            putc(*s, output_file);
                    }
                    fprintf(output_file, "'\",");
                }
            }
            else
            {
                k = (Value_t) (strlen(s) + 3);
                j += k;
                if (j > 80)
                {
                    if (!rflag) ++outline;
                    putc('\n', output_file);
                    j = k;
                }
                putc('"', output_file);
                do { putc(*s, output_file); } while (*++s);
                fprintf(output_file, "\",");
            }
        }
        else
        {
            j += 2;
            if (j > 80)
            {
                if (!rflag) ++outline;
                putc('\n', output_file);
                j = 2;
            }
            fprintf(output_file, "0,");
        }
    }
    if (!rflag) outline += 2;
    fprintf(output_file, "\n};\n");
    FREE(symnam);

    if (!rflag) ++outline;
    fprintf(output_file, "char *%srule[] = {\n", symbol_prefix);
    for (i = 2; i < nrules; ++i)
    {
        fprintf(output_file, "\"%s :", symbol_name[rlhs[i]]);
        for (j = rrhs[i]; ritem[j] > 0; ++j)
        {
            s = symbol_name[ritem[j]];
            if (s[0] == '"')
            {
                fprintf(output_file, " \\\"");
                while (*++s != '"')
                {
                    if (*s == '\\')
                    {
                        if (s[1] == '\\')
                            fprintf(output_file, "\\\\\\\\");
                        else
                            fprintf(output_file, "\\\\%c", s[1]);
                        ++s;
                    }
                    else
                        putc(*s, output_file);
                }
                fprintf(output_file, "\\\"");
            }
            else if (s[0] == '\'')
            {
                if (s[1] == '"')
                    fprintf(output_file, " '\\\"'");
                else if (s[1] == '\\')
                {
                    if (s[2] == '\\')
                        fprintf(output_file, " '\\\\\\\\");
                    else
                        fprintf(output_file, " '\\\\%c", s[2]);
                    s += 2;
                    while (*++s != '\'')
                        putc(*s, output_file);
                    putc('\'', output_file);
                }
                else
                    fprintf(output_file, " '%c'", s[1]);
            }
            else
                fprintf(output_file, " %s", s);
        }
        if (!rflag) ++outline;
        fprintf(output_file, "\",\n");
    }

    if (!rflag) outline += 2;
    fprintf(output_file, "};\n#endif\n");
}

static void
output_stype(void)
{
    if (!unionized && ntags == 0)
    {
        outline += 3;
        fprintf(code_file, "#ifndef YYSTYPE\ntypedef int YYSTYPE;\n#endif\n");
    }
}

static void
output_trailing_text(void)
{
    int c, last;
    FILE *in;

    if (line == 0)
        return;

    in = input_file;
    c = *cptr;
    if (c == '\n')
    {
        ++lineno;
        if ((c = getc(in)) == EOF)
            return;
        write_input_lineno();
        putc_code(code_file, c);
        last = c;
    }
    else
    {
        write_input_lineno();
        do
        {
            putc_code(code_file, c);
        }
        while ((c = *++cptr) != '\n');
            putc_code(code_file, c);
        last = '\n';
    }

    while ((c = getc(in)) != EOF)
    {
        putc_code(code_file, c);
        last = c;
    }

    if (last != '\n')
    {
        putc_code(code_file, '\n');
    }
    write_code_lineno(code_file);
}

static void
output_semantic_actions(void)
{
    int c, last;

    rewind(action_file);
    if ((c = getc(action_file)) == EOF)
        return;

    last = c;
    putc_code(code_file, c);
    while ((c = getc(action_file)) != EOF)
    {
        putc_code(code_file, c);
        last = c;
    }

    if (last != '\n')
    {
        putc_code(code_file, '\n');
    }

    write_code_lineno(code_file);
}

static void
free_itemsets(void)
{
    core *cp, *next;

    FREE(state_table);
    for (cp = first_state; cp; cp = next)
    {
        next = cp->next;
        FREE(cp);
    }
}

static void
free_shifts(void)
{
    shifts *sp, *next;

    FREE(shift_table);
    for (sp = first_shift; sp; sp = next)
    {
        next = sp->next;
        FREE(sp);
    }
}


static void
free_reductions(void)
{
    reductions *rp, *next;

    FREE(reduction_table);
    for (rp = first_reduction; rp; rp = next)
    {
        next = rp->next;
        FREE(rp);
    }
}
