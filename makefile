# Name: Ilia Benson
# Comp 610 2SAT

2SAT                     = 2sat

CC                       = gcc

LINKER                   = gcc -o

CFLAGS                   = -O3 -Wextra -Wall

LFLAGS                   = -O3 -Wextra -Wall

LIBS                     = -lm

SRCDIR                   = src
OBJDIR                   = obj
BINDIR                   = bin

2SAT_SRC                := $(wildcard $(SRCDIR)/*.c)
2SAT_INC                := $(wildcard $(SRCDIR)/*.h)
2SAT_OBJ                := $(2SAT_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

rm                       = rm -rf
mkdir                    = mkdir -p

.PHONEY: all
all: 2sat

################################################################################

.PHONEY: 2sat
2sat: $(BINDIR)/$(2SAT)

$(BINDIR)/$(2SAT): $(2SAT_OBJ)
	@$(LINKER) $@ $(LFLAGS) $(2SAT_OBJ) $(LIBS)
	@echo "2SAT Linking complete!"

$(2SAT_OBJ): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

$(2SAT_OBJ): | $(OBJDIR)

$(BINDIR)/$(2SAT): | $(BINDIR)

$(OBJDIR):
	@$(mkdir) $(OBJDIR)

$(BINDIR):
	@$(mkdir) $(BINDIR)

################################################################################

.PHONEY: clean
clean:
	@$(rm) $(2SAT_OBJ)
	@$(rm) $(BINDIR)/*.dSYM
	@echo "Cleanup complete!"

################################################################################

.PHONEY: remove
remove: clean
	@$(rm) $(BINDIR)/$(2SAT)
	@echo "Executable removed!"
