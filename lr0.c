#include <stddef.h>
#include "defs.h"
#include "closure.h"
#include "error.h"
#include "lr0.h"

extern Value_t *itemset;
extern Value_t *itemsetend;
extern unsigned *ruleset;

int nstates;
core *first_state;
shifts *first_shift;
reductions *first_reduction;

int get_state(int symbol);
core *new_state(int symbol);

static core **state_set;
static core *this_state;
static core *last_state;
static shifts *last_shift;
static reductions *last_reduction;

static int nshifts;
static Value_t *shift_symbol;

static Value_t *redset;
static Value_t *shiftset;

static Value_t **kernel_base;
static Value_t **kernel_end;
static Value_t *kernel_items;

void
allocate_itemsets(void)
{
    Value_t *itemp;
    Value_t *item_end;
    int symbol;
    int i;
    int count;
    int max;
    Value_t *symbol_count;

    count = 0;
    symbol_count = NEW2(nsyms, Value_t);

    item_end = ritem + nitems;
    for (itemp = ritem; itemp < item_end; itemp++)
    {
        symbol = *itemp;
        if (symbol >= 0)
        {
            count++;
            symbol_count[symbol]++;
        }
    }

    kernel_base = NEW2(nsyms, Value_t *);
    kernel_items = NEW2(count, Value_t);

    count = 0;
    max = 0;
    for (i = 0; i < nsyms; i++)
    {
        kernel_base[i] = kernel_items + count;
        count += symbol_count[i];
        if (max < symbol_count[i])
            max = symbol_count[i];
    }

    shift_symbol = symbol_count;
    kernel_end = NEW2(nsyms, Value_t *);
}

void
allocate_storage(void)
{
    allocate_itemsets();
    shiftset = NEW2(nsyms, Value_t);
    redset = NEW2(nrules + 1, Value_t);
    state_set = NEW2(nitems, core *);
}

void
append_states(void)
{
    int i;
    int j;
    int symbol;

#ifdef        TRACE
    fprintf(stderr, "Entering append_states()\n");
#endif
    for (i = 1; i < nshifts; i++)
    {
        symbol = shift_symbol[i];
        j = i;
        while (j > 0 && shift_symbol[j - 1] > symbol)
        {
            shift_symbol[j] = shift_symbol[j - 1];
            j--;
        }
        shift_symbol[j] = symbol;
    }

    for (i = 0; i < nshifts; i++)
    {
        symbol = shift_symbol[i];
        shiftset[i] = get_state(symbol);
    }
}

void
free_storage(void)
{
    FREE(shift_symbol);
    FREE(redset);
    FREE(shiftset);
    FREE(kernel_base);
    FREE(kernel_end);
    FREE(kernel_items);
    FREE(state_set);
}


void
generate_states(void)
{
    allocate_storage();
    itemset = NEW2(nitems, Value_t);
    ruleset = NEW2(WORDSIZE(nrules), unsigned);
    set_first_derives();
    initialize_states();

    while (this_state)
    {
        closure(this_state->items, this_state->nitems);
        save_reductions();
        new_itemsets();
        append_states();

        if (nshifts > 0)
            save_shifts();

        this_state = this_state->next;
    }

    finalize_closure();
    free_storage();
}



int
get_state(int symbol)
{
    int key;
    Value_t *isp1;
    Value_t *isp2;
    Value_t *iend;
    core *sp;
    int found;
    ptrdiff_t n;

#ifdef        TRACE
    fprintf(stderr, "Entering get_state(%d)\n", symbol);
#endif

    isp1 = kernel_base[symbol];
    iend = kernel_end[symbol];
    n = iend - isp1;

    key = *isp1;
    assert(0 <= key && key < nitems);
    sp = state_set[key];
    if (sp)
    {
        found = 0;
        while (!found)
        {
            if (sp->nitems == n)
            {
                found = 1;
                isp1 = kernel_base[symbol];
                isp2 = sp->items;

                while (found && isp1 < iend)
                {
                    if (*isp1++ != *isp2++)
                        found = 0;
                }
            }

            if (!found)
            {
                if (sp->link)
                {
                    sp = sp->link;
                }
                else
                {
                    sp = sp->link = new_state(symbol);
                    found = 1;
                }
            }
        }
    }
    else
    {
        state_set[key] = sp = new_state(symbol);
    }

    return (sp->number);
}


void
initialize_states(void)
{
    int i;
    Value_t *start_derives;
    core *p;

    start_derives = derives[start_symbol];
    for (i = 0; start_derives[i] >= 0; ++i)
        continue;

    p = (core *) MALLOC(sizeof(core) + i*sizeof(Value_t));
    if (p == 0) no_space();

    p->next = 0;
    p->link = 0;
    p->number = 0;
    p->accessing_symbol = 0;
    p->nitems = i;

    for (i = 0;  start_derives[i] >= 0; ++i)
        p->items[i] = rrhs[start_derives[i]];

    first_state = last_state = this_state = p;
    nstates = 1;
}

void
new_itemsets(void)
{
    int i;
    int shiftcount;
    Value_t *isp;
    Value_t *ksp;
    int symbol;

    for (i = 0; i < nsyms; i++)
        kernel_end[i] = 0;

    shiftcount = 0;
    isp = itemset;
    while (isp < itemsetend)
    {
        i = *isp++;
        symbol = ritem[i];
        if (symbol > 0)
        {
            ksp = kernel_end[symbol];
            if (!ksp)
            {
                shift_symbol[shiftcount++] = symbol;
                ksp = kernel_base[symbol];
            }

            *ksp++ = i + 1;
            kernel_end[symbol] = ksp;
        }
    }

    nshifts = shiftcount;
}



core *
new_state(int symbol)
{
    ptrdiff_t n;
    core *p;
    Value_t *isp1;
    Value_t *isp2;
    Value_t *iend;

#ifdef        TRACE
    fprintf(stderr, "Entering new_state(%d)\n", symbol);
#endif

    if (nstates >= MAXYYINT)
        fatal("too many states");

    isp1 = kernel_base[symbol];
    iend = kernel_end[symbol];
    n = iend - isp1;

    p = (core *) allocate((unsigned) (sizeof(core) + (n - 1) * sizeof(Value_t)));
    p->accessing_symbol = symbol;
    p->number = nstates;
    p->nitems = (Value_t) n;

    isp2 = p->items;
    while (isp1 < iend)
        *isp2++ = *isp1++;

    last_state->next = p;
    last_state = p;

    nstates++;

    return (p);
}


/* show_cores is used for debugging */
void
show_cores(void)
{
    core *p;
    int i, j, k, n;
    int itemno;

    k = 0;
    for (p = first_state; p; ++k, p = p->next)
    {
        if (k) printf("\n");
        printf("state %d, number = %d, accessing symbol = %s\n",
                k, p->number, symbol_name[p->accessing_symbol]);
        n = p->nitems;
        for (i = 0; i < n; ++i)
        {
            itemno = p->items[i];
            printf("%4d  ", itemno);
            j = itemno;
            while (ritem[j] >= 0) ++j;
            printf("%s :", symbol_name[rlhs[-ritem[j]]]);
            j = rrhs[-ritem[j]];
            while (j < itemno)
                printf(" %s", symbol_name[ritem[j++]]);
            printf(" .");
            while (ritem[j] >= 0)
                printf(" %s", symbol_name[ritem[j++]]);
            printf("\n");
            fflush(stdout);
        }
    }
}


/* show_ritems is used for debugging */
void
show_ritems(void)
{
    int i;

    for (i = 0; i < nitems; ++i)
        printf("ritem[%d] = %d\n", i, ritem[i]);
}


/* show_rrhs is used for debugging */
void
show_rrhs(void)
{
    int i;

    for (i = 0; i < nrules; ++i)
        printf("rrhs[%d] = %d\n", i, rrhs[i]);
}


/* show_shifts is used for debugging */
void
show_shifts(void)
{
    shifts *p;
    int i, j, k;

    k = 0;
    for (p = first_shift; p; ++k, p = p->next)
    {
        if (k) printf("\n");
        printf("shift %d, number = %d, nshifts = %d\n", k, p->number,
                p->nshifts);
        j = p->nshifts;
        for (i = 0; i < j; ++i)
            printf("\t%d\n", p->shift[i]);
    }
}

void
save_shifts(void)
{
    shifts *p;
    Value_t *sp1;
    Value_t *sp2;
    Value_t *send;

    p = (shifts *) allocate((unsigned) (sizeof(shifts) +
                        (nshifts - 1) * sizeof(Value_t)));

    p->number = this_state->number;
    p->nshifts = nshifts;

    sp1 = shiftset;
    sp2 = p->shift;
    send = shiftset + nshifts;

    while (sp1 < send)
        *sp2++ = *sp1++;

    if (last_shift)
    {
        last_shift->next = p;
        last_shift = p;
    }
    else
    {
        first_shift = p;
        last_shift = p;
    }
}


void
save_reductions(void)
{
    Value_t *isp;
    Value_t *rp1;
    Value_t *rp2;
    int item;
    int count;
    reductions *p;
    Value_t *rend;

    count = 0;
    for (isp = itemset; isp < itemsetend; isp++)
    {
        item = ritem[*isp];
        if (item < 0)
        {
            redset[count++] = -item;
        }
    }

    if (count)
    {
        p = (reductions *) allocate((unsigned) (sizeof(reductions) +
                                        (count - 1) * sizeof(Value_t)));

        p->number = this_state->number;
        p->nreds = count;

        rp1 = redset;
        rp2 = p->rules;
        rend = rp1 + count;

        while (rp1 < rend)
            *rp2++ = *rp1++;

        if (last_reduction)
        {
            last_reduction->next = p;
            last_reduction = p;
        }
        else
        {
            first_reduction = p;
            last_reduction = p;
        }
    }
}

void
set_derives(void)
{
    int i, k;
    int lhs;
    Value_t *rules;

    derives = NEW2(nsyms, Value_t *);
    rules = NEW2(nvars + nrules, Value_t);

    k = 0;
    for (lhs = start_symbol; lhs < nsyms; lhs++)
    {
        derives[lhs] = rules + k;
        for (i = 0; i < nrules; i++)
        {
            if (rlhs[i] == lhs)
            {
                rules[k] = i;
                k++;
            }
        }
        rules[k] = -1;
        k++;
    }

#ifdef        DEBUG
    print_derives();
#endif
}

void
free_derives(void)
{
    FREE(derives[start_symbol]);
    FREE(derives);
}

#ifdef        DEBUG
void
print_derives(void)
{
    int i;
    Value_t *sp;

    printf("\nDERIVES\n\n");

    for (i = start_symbol; i < nsyms; i++)
    {
        printf("%s derives ", symbol_name[i]);
        for (sp = derives[i]; *sp >= 0; sp++)
        {
            printf("  %d", *sp);
        }
        putchar('\n');
    }

    putchar('\n');
}
#endif

void
set_nullable(void)
{
    int i, j;
    int empty;
    int done;

    nullable = MALLOC(nsyms);
    if (nullable == 0) no_space();

    for (i = 0; i < nsyms; ++i)
        nullable[i] = 0;

    done = 0;
    while (!done)
    {
        done = 1;
        for (i = 1; i < nitems; i++)
        {
            empty = 1;
            while ((j = ritem[i]) >= 0)
            {
                if (!nullable[j])
                    empty = 0;
                ++i;
            }
            if (empty)
            {
                j = rlhs[-j];
                if (!nullable[j])
                {
                    nullable[j] = 1;
                    done = 0;
                }
            }
        }
    }

#ifdef DEBUG
    for (i = 0; i < nsyms; i++)
    {
        if (nullable[i])
            printf("%s is nullable\n", symbol_name[i]);
        else
            printf("%s is not nullable\n", symbol_name[i]);
    }
#endif
}

void
free_nullable(void)
{
    FREE(nullable);
}

void
lr0(void)
{
    set_derives();
    set_nullable();
    generate_states();
}
