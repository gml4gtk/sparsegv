VERSION=0.6.3
SPARSEGV_VERSION=1.0.0

########################################################################
# The following variables can be overwritten from the command line
OS = linux


CC = gcc
CXX = g++
LD = $(CC)
AR = ar

CFLAGS ?= -O2 -g -Wall -Wextra

DESTDIR ?=
PREFIX ?= $(HOME)
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man


CHECKER_FLAGS ?= -Wno-vla

# Allow users to override build settings without dirtying their trees
# For debugging, put this in local.mk:
#
#     CFLAGS += -O0 -DDEBUG -g3 -gdwarf-2
#
SPARSE_LOCAL_CONFIG ?= local.mk
-include ${SPARSE_LOCAL_CONFIG}
########################################################################


LIB_OBJS :=
LIB_OBJS += allocate.o
LIB_OBJS += builtin.o
LIB_OBJS += char.o
LIB_OBJS += compat-$(OS).o
LIB_OBJS += cse.o
LIB_OBJS += dissect.o
LIB_OBJS += dominate.o
LIB_OBJS += evaluate.o
LIB_OBJS += expand.o
LIB_OBJS += expression.o
LIB_OBJS += flow.o
LIB_OBJS += flowgraph.o
LIB_OBJS += inline.o
LIB_OBJS += ir.o
LIB_OBJS += lib.o
LIB_OBJS += linearize.o
LIB_OBJS += liveness.o
LIB_OBJS += memops.o
LIB_OBJS += opcode.o
LIB_OBJS += optimize.o
LIB_OBJS += options.o
LIB_OBJS += parse.o
LIB_OBJS += predefine.o
LIB_OBJS += pre-process.o
LIB_OBJS += ptrlist.o
LIB_OBJS += ptrmap.o
LIB_OBJS += scope.o
LIB_OBJS += show-parse.o
LIB_OBJS += simplify.o
LIB_OBJS += sort.o
LIB_OBJS += ssa.o
LIB_OBJS += stats.o
LIB_OBJS += storage.o
LIB_OBJS += symbol.o
LIB_OBJS += target.o
LIB_OBJS += target-alpha.o
LIB_OBJS += target-arm.o
LIB_OBJS += target-arm64.o
LIB_OBJS += target-bfin.o
LIB_OBJS += target-default.o
LIB_OBJS += target-h8300.o
LIB_OBJS += target-m68k.o
LIB_OBJS += target-microblaze.o
LIB_OBJS += target-mips.o
LIB_OBJS += target-nds32.o
LIB_OBJS += target-nios2.o
LIB_OBJS += target-openrisc.o
LIB_OBJS += target-ppc.o
LIB_OBJS += target-riscv.o
LIB_OBJS += target-s390.o
LIB_OBJS += target-sh.o
LIB_OBJS += target-sparc.o
LIB_OBJS += target-x86.o
LIB_OBJS += target-xtensa.o
LIB_OBJS += tokenize.o
LIB_OBJS += unssa.o
LIB_OBJS += utils.o
LIB_OBJS += version.o

PROGRAMS :=
PROGRAMS += sparsegv

all:

########################################################################
# common flags/options/...

cflags = -fno-strict-aliasing
cflags += -Wall -Wwrite-strings

GCC_BASE := $(shell $(CC) --print-file-name=)
cflags += -DGCC_BASE=\"$(GCC_BASE)\"

MULTIARCH_TRIPLET := $(shell $(CC) -print-multiarch 2>/dev/null)
cflags += -DMULTIARCH_TRIPLET=\"$(MULTIARCH_TRIPLET)\"


bindir := $(DESTDIR)$(BINDIR)
man1dir := $(DESTDIR)$(MANDIR)/man1

########################################################################
# target specificities


# Can we use GCC's generated dependencies?
HAVE_GCC_DEP:=$(shell touch .gcc-test.c && 				\
		$(CC) -c -Wp,-MP,-MMD,.gcc-test.d .gcc-test.c 2>/dev/null && \
		echo 'yes'; rm -f .gcc-test.d .gcc-test.o .gcc-test.c)
ifeq ($(HAVE_GCC_DEP),yes)
cflags += -Wp,-MP,-MMD,$(@D)/.$(@F).d
endif


########################################################################
LIBS := libsparse.a
OBJS := $(LIB_OBJS) $(EXTRA_OBJS) $(PROGRAMS:%=%.o)

# Pretty print
V := @
Q := $(V:1=)

########################################################################
all: $(PROGRAMS)

ldflags += $($(@)-ldflags) $(LDFLAGS)
ldlibs  += $($(@)-ldlibs)  $(LDLIBS)
$(PROGRAMS): % : %.o $(LIBS)
	@echo "  LD      $@"
	$(Q)$(LD) $(ldflags) $^ $(ldlibs) -o $@

libsparse.a: $(LIB_OBJS)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^


cflags   += $($(*)-cflags) $(CPPFLAGS) $(CFLAGS)
%.o: %.c
	@echo "  CC      $@"
	$(Q)$(CC) $(cflags) -c -o $@ $<

%.sc: %.c sparse
	@echo "  CHECK   $<"
	$(Q)CHECK=./sparse ./cgcc -no-compile $(CHECKER_FLAGS) $(cflags) -c $<

selfcheck: $(OBJS:.o=.sc)

SPARSE_VERSION:=$(shell git describe --dirty 2>/dev/null || echo '$(VERSION)')
version.o: version.h
version.h: FORCE
	@echo '#define SPARSE_VERSION "$(SPARSE_VERSION)"' > version.h.tmp
	@if cmp -s version.h version.h.tmp; then \
		rm version.h.tmp; \
	else \
		echo "  GEN     $@"; \
		mv version.h.tmp version.h; \
	fi


clean:
	@rm -f *.[oa] .*.d $(PROGRAMS)
	@rm -f O *.gv *.gml *.c~ *.h~

indent:
	./lindent.sh *.c *.h

.PHONY: FORCE

# GCC's dependencies
-include $(OBJS:%.o=.%.o.d)
