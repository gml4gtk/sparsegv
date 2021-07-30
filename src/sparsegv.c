/* Copyright © International Business Machines Corp., 2006
 *              Adelard LLP, 2007
 *
 * Author: Josh Triplett <josh@freedesktop.org>
 *         Dan Sheridan <djs@adelard.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define SPARSEGV_VERSION "1.0.0"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"
#include "allocate.h"
#include "token.h"
#include "parse.h"
#include "symbol.h"
#include "expression.h"
#include "linearize.h"

/* all data in one file */
static FILE *fall = NULL;	/* dot */

static void sparse_output_bb(FILE *f, struct basic_block *bb, const char *fname, const char *sname)
{
	struct instruction *insn = NULL;
	const char *is = NULL;
	int c = 0;
	const char *p = NULL;
	char *q = NULL;
	char *buf = NULL;
	int cl = 0;
	fprintf(f, "%s() line %d file %s\\lbasic block number %d\\l%s\\l", fname, bb->pos.line, sname, bb->nr, show_label(bb));

	FOR_EACH_PTR(bb->insns, insn) {
		if (!insn->bb) {
			continue;
		}
		is = show_instruction(insn);
		/* the instruction can have "" to set as escape */
		if (strchr(is, '"')) {
			/* how many " in s */
			c = 0;
			p = is;
			while (*p) {
				if ((*p) == '"') {
					c++;
				}
				p++;
			}
			/* how many space for new string */
			cl = strlen(is) + 1 + (2 * c);
			buf = calloc(1, cl);
			p = is;
			q = buf;
			/* change " into \" */
			/* change \ into \\ */
			while (*p) {
				if ((*p) == '"') {
					*q = '\\';
					q++;
					*q = '"';
				} else if ((*p) == '\\') {
					*q = '\\';
					q++;
					*q = '\\';
				} else {
					*q = *p;
				}
				q++;
				p++;
			}
			is = buf;
		} else {
			/* no changes needed */
		}
		fprintf(f, "  %s\\l", is);
		if (buf) {
			free(buf);
			buf = NULL;
		}
	}
	END_FOR_EACH_PTR(insn);

	return;
}

/* Draw the subgraph for a given entrypoint. Includes details of loads
 * and stores for globals, and marks return bbs */
static void graph_ep(struct entrypoint *ep)
{
	struct basic_block *bb=NULL;
	struct instruction *insn=NULL;
	const char *fname=NULL;
	const char  *sname;
	const char *fn=NULL;
	const char *p=NULL;
size_t tl=0;
	int nedges = 0;
FILE *f=NULL;

	fname = show_ident(ep->name->ident);
	sname = stream_name(ep->entry->bb->pos.stream);

	fprintf(fall, "subgraph cluster%p {\n"
		"    color=blue;\n"
		"    label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\">\n"
		"             <TR><TD>%s</TD></TR>\n"
		"             <TR><TD><FONT POINT-SIZE=\"21\">%s()</FONT></TD></TR>\n"
		"           </TABLE>>;\n"
		"    file=\"%s\";\n" "    fun=\"%s\";\n" "    ep=bb%p;\n", ep, sname, fname, sname, fname, ep->entry->bb);

	FOR_EACH_PTR(ep->bbs, bb) {
		struct basic_block *child = NULL;
		int ret = 0;
		const char *s = ", ls=\"[";

		/* Node for the bb */
		fprintf(fall, "    bb%p [shape=ellipse,label=%d,line=%d,col=%d", bb, bb->pos.line, bb->pos.line, bb->pos.pos);

		/* List loads and stores */
		FOR_EACH_PTR(bb->insns, insn) {
			if (!insn->bb) {
				continue;
			}

			/* */
			switch (insn->opcode) {
			case OP_STORE:
				if (insn->src->type == PSEUDO_SYM) {
					fprintf(fall, "%s store(%s)", s, show_ident(insn->src->sym->ident));
					s = ",";
				}
				break;

			case OP_LOAD:
				if (insn->src->type == PSEUDO_SYM) {
					fprintf(fall, "%s load(%s)", s, show_ident(insn->src->sym->ident));
					s = ",";
				}
				break;

			case OP_RET:
				ret = 1;
				break;

			}
		}
		END_FOR_EACH_PTR(insn);

		/* */
		if (s[1] == 0) {
			fprintf(fall, "]\"");
		}

		/* */
		if (ret) {
			fprintf(fall, ",op=ret");
		}

		/* */
		fprintf(fall, "];\n");

		/* Edges between bbs; lower weight for upward edges */
		FOR_EACH_PTR(bb->children, child) {
nedges++;
			fprintf(fall, "    bb%p -> bb%p [op=br, %s];\n", bb, child,
				(bb->pos.line > child->pos.line) ? "weight=5" : "weight=10");
		}
		END_FOR_EACH_PTR(child);
	}
	END_FOR_EACH_PTR(bb);

	fprintf(fall, "}\n");

 /* */
if(nedges==0){return;}

 /* create new file name */
tl=strlen(fname)+ strlen(".") + strlen( sname)+strlen(".gv");

fn=calloc(1,(tl+1));

if(fn==NULL){return;}

p=fn;
p=strcpy(p,sname); /* file name */
p=strcat(p,".");
p=strcat(p,fname); /* function name */
p=strcat(p,".gv");

f=fopen(fn,"wb");

if(f==NULL){
 free(fn);
return;
}

	fprintf(f, "/* Generated by sparsegv version %s running with sparse version %s */\n", SPARSEGV_VERSION, sparse_version);

	fprintf(f, "digraph \"cluster%p\" {\n"
		"    color=blue;\n"
		"    label=\"%s\\l%s()\\l"
		"    file=%s;\\l" "    fun=%s;\\l" "    ep=bb%p;\\l\"\n", ep, sname, fname, sname, fname, ep->entry->bb);

	FOR_EACH_PTR(ep->bbs, bb) {
		struct basic_block *child=NULL;
		int ret = 0;
		const char *s = ", ls=\"[";

		/* Node for the bb */
		fprintf(f, "    bb%p [shape=ellipse,label=\"", bb);

		sparse_output_bb(f,bb, fname, sname);

		fprintf(f, "\",line=%d,col=%d", bb->pos.line, bb->pos.pos);

		/* List loads and stores */
		FOR_EACH_PTR(bb->insns, insn) {
			if (!insn->bb)
{				continue;}

 /* */
			switch (insn->opcode) {
			case OP_STORE:
				if (insn->src->type == PSEUDO_SYM) {
					fprintf(f, "%s store(%s)", s, show_ident(insn->src->sym->ident));
					s = ",";
				}
				break;

			case OP_LOAD:
				if (insn->src->type == PSEUDO_SYM) {
					fprintf(f, "%s load(%s)", s, show_ident(insn->src->sym->ident));
					s = ",";
				}
				break;

			case OP_RET:
				ret = 1;
				break;

			}
		}
		END_FOR_EACH_PTR(insn);


 /* */
		if (s[1] == 0)
{			fprintf(f, "]\""); }


 /* */
		if (ret)
{ 			fprintf(f, ",op=ret"); }


		fprintf(f, "];\n");

		/* Edges between bbs; lower weight for upward edges */
		FOR_EACH_PTR(bb->children, child) {
			fprintf(f, "    bb%p -> bb%p [op=br, %s];\n", bb, child,
				(bb->pos.line > child->pos.line) ? "weight=5" : "weight=10");
		}
		END_FOR_EACH_PTR(child);
	}
	END_FOR_EACH_PTR(bb);

	fprintf(f, "}\n");

fclose (f);
free(fn);

	return;
}

/* Insert edges for intra- or inter-file calls, depending on the value
 * of internal. Bold edges are used for calls with destinations;
 * dashed for calls to external functions */
static void graph_calls(struct entrypoint *ep, int internal)
{
	struct basic_block *bb;
	struct instruction *insn;

	show_ident(ep->name->ident);
	stream_name(ep->entry->bb->pos.stream);

	FOR_EACH_PTR(ep->bbs, bb) {
		if (!bb) {
			continue;
		}

		/* */
		if (!bb->parents && !bb->children && !bb->insns && verbose < 2) {
			continue;
		}

		/* */
		FOR_EACH_PTR(bb->insns, insn) {
			if (!insn->bb) {
				continue;
			}

			/* */
			if (insn->opcode == OP_CALL && internal == !(insn->func->sym->ctype.modifiers & MOD_EXTERN)) {

				/* Find the symbol for the callee's definition */
				struct symbol *sym = NULL;
				if (insn->func->type == PSEUDO_SYM) {
					for (sym = insn->func->sym->ident->symbols; sym; sym = sym->next_id) {
						if (sym->namespace & NS_SYMBOL && sym->ep) {
							break;
						}
					}

					if (sym) {
						fprintf(fall, "bb%p -> bb%p"
							"[label=%d,line=%d,col=%d,op=call,style=bold,weight=30];\n",
							bb, sym->ep->entry->bb, insn->pos.line, insn->pos.line, insn->pos.pos);
					} else {
						fprintf(fall, "bb%p -> \"%s\" "
							"[label=%d,line=%d,col=%d,op=extern,style=dashed];\n",
							bb, show_pseudo(insn->func), insn->pos.line, insn->pos.line, insn->pos.pos);
					}
				}
			}
		}
		END_FOR_EACH_PTR(insn);
	}
	END_FOR_EACH_PTR(bb);

	return;
}

int main(int argc, char **argv)
{
	struct string_list *filelist = NULL;
	char *file = NULL;
	struct symbol *sym = NULL;
	struct symbol_list *fsyms = NULL;
	struct symbol_list *all_syms = NULL;
	int i = 0;

	if (argc < 2) {
		printf("Usage: sparsegv file.c\n");
		return (0);
	}

	fall = fopen("all.gv", "wb");

	if (fall == NULL) {
		printf("cannot open output files\n");
		return (0);
	}

	fprintf(fall, "/* Generated by sparsegv version %s running with sparse version %s\n", SPARSEGV_VERSION, sparse_version);
	fprintf(fall, " * Commandline:");
	for (i = 0; i < argc; i++) {
		fprintf(fall, " %s", argv[i]);
	}
	fprintf(fall, "\n */\ndigraph call_graph {\n");

	fsyms = sparse_initialize(argc, argv, &filelist);
	concat_symbol_list(fsyms, &all_syms);

	/* Linearize all symbols, graph internal basic block
	 * structures and intra-file calls */
	FOR_EACH_PTR(filelist, file) {

		fsyms = sparse(file);
		concat_symbol_list(fsyms, &all_syms);

		FOR_EACH_PTR(fsyms, sym) {
			expand_symbol(sym);
			linearize_symbol(sym);
		}
		END_FOR_EACH_PTR(sym);

		FOR_EACH_PTR(fsyms, sym) {
			if (sym->ep) {
				graph_ep(sym->ep);
				graph_calls(sym->ep, 1);
			}
		}
		END_FOR_EACH_PTR(sym);

	}
	END_FOR_EACH_PTR(file);

	/* Graph inter-file calls */
	FOR_EACH_PTR(all_syms, sym) {
		if (sym->ep)
			graph_calls(sym->ep, 0);
	}
	END_FOR_EACH_PTR(sym);

	fprintf(fall, "}\n/* end. */\n");
	return 0;
}
