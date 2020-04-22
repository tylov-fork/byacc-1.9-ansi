DEST	      = .

HDRS	      = defs.h

CFLAGS	      = -Wall -O2 -DNDEBUG

LDFLAGS	      =

LIBS	      =

CC            = gcc
LINKER	      = $(CC)
EXE	          = .exe

MAKEFILE      = Makefile

OBJS	      = closure.o \
		error.o \
		lalr.o \
		lr0.o \
		main.o \
		mkpar.o \
		output.o \
		reader.o \
		skeleton.o \
		symtab.o \
		verbose.o \
		warshall.o

PRINT	      = pr -f -l88


PROGRAM	      = yacc$(EXE)

SRCS	      = closure.c \
		error.c \
		lalr.c \
		lr0.c \
		main.c \
		mkpar.c \
		output.c \
		reader.c \
		skeleton.c \
		symtab.c \
		verbose.c \
		warshall.c

all:		$(PROGRAM)


$(PROGRAM):     $(OBJS) $(LIBS)
		@echo -n "Loading $(PROGRAM) ... "
		@$(LINKER) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LIBS)
		strip $(PROGRAM)
		@echo "done"

clean:;		@rm -f $(OBJS)

clobber:;	@rm -f $(OBJS) $(PROGRAM)

depend:;	@mkmf -f $(MAKEFILE) PROGRAM=$(PROGRAM) DEST=$(DEST)

index:;		@ctags -wx $(HDRS) $(SRCS)

install:	$(PROGRAM)
		@echo Installing $(PROGRAM) in $(DEST)
		@install -s $(PROGRAM) $(DEST)

listing:;	@$(PRINT) Makefile $(HDRS) $(SRCS) | lpr

lint:;		@lint $(SRCS)

program:        $(PROGRAM)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

# DO NOT DELETE

closure.o: defs.h warshall.h closure.h
error.o: defs.h error.h
lalr.o: defs.h error.h lr0.h lalr.h
lr0.o: defs.h closure.h error.h lr0.h
main.o: defs.h error.h reader.h lr0.h lalr.h mkpar.h verbose.h output.h
mkpar.o: defs.h error.h lr0.h lalr.h mkpar.h
output.o: defs.h error.h skeleton.h lr0.h lalr.h mkpar.h reader.h output.h
reader.o: defs.h error.h skeleton.h symtab.h reader.h
skeleton.o: defs.h
symtab.o: defs.h error.h
verbose.o: defs.h error.h lr0.h lalr.h mkpar.h verbose.h
warshall.o: defs.h warshall.h
