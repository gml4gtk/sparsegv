#ifndef	DISSECT_H
#define	DISSECT_H

#include <stdio.h>
#include "parse.h"
#include "expression.h"
#include "scope.h"

#define	U_SHIFT		8

#define	U_R_AOF		0x01
#define	U_W_AOF		0x02

#define	U_R_VAL		0x04
#define	U_W_VAL		0x08

#define	U_R_PTR		(U_R_VAL << U_SHIFT)
#define	U_W_PTR		(U_W_VAL << U_SHIFT)

struct reporter
{
	void (*r_symdef)(struct symbol *);
	void (*r_memdef)(struct symbol *, struct symbol *);

	void (*r_symbol)(unsigned, struct position *, struct symbol *);
	void (*r_member)(unsigned, struct position *, struct symbol *, struct symbol *);
};

extern struct symbol *dissect_ctx;

static inline bool sym_is_local(struct symbol *sym)
{
	return !toplevel(sym->scope);
}

extern void dissect(struct reporter *, struct string_list *);

#endif
