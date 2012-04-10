#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

void *jmptable[256];

void Cons::print(FILE *fp) {
	bool b = false;
	Cons *self = this;
	while(self != NULL) {
		if(b) printf(" ");
		b = true;
		switch(self->type) {
		case CONS_INT: printf("%d", self->i); break;
		case CONS_STR: printf("%s", self->str); break;
		case CONS_CAR:
			fprintf(fp, "(");
			self->car->print(fp);
			fprintf(fp, ")");
			break;
		}
		self = self->cdr;
	}
}

void Cons::println(FILE *fp) {
	this->print(fp);
	fprintf(fp, "\n");
}

static void runCons(Context *ctx, Cons *cons) {
	if(cons->type == CONS_CAR) {
		for(; cons != NULL; cons = cons->cdr) {
			cons->println();
			runCons(ctx, cons->car);
		}
		return;
	}
	CodeBuilder *cb = new CodeBuilder();
	codegen(ctx, cons, cb);
	cb->addInst(INS_EXIT);

	WorkerThread *wth = new WorkerThread();
	wth->pc = cb->code;
	wth->fp = wth->frame;
	wth->sp = wth->stack;
	vmrun(wth);
	printf("%d\n", wth->stack[0]);
	delete wth;
	delete cb;
}

#define HISTFILE ".history"

static void interactive(Context *ctx) {
	read_history(HISTFILE);
	while(true) {
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			break;
		}
		add_history(in);
		Cons *c = parseExpr(in).value;
		runCons(ctx, c);
		free(in);
		write_history(HISTFILE);
	}
}

static void runFromFile(const char *filename, Context *ctx) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) return;
	char is[256];
	int size = fread(is, 1, 256, fp);
	fclose(fp);
	is[size] = '\0';

	const char *src = is;
	while(true) {
		ParseResult<Cons *> res = parseExpr(src);
		if(res.success) {
			src = res.src;
			runCons(ctx, res.value);
		} else {
			break;
		}
	}
}

int main(int argc, char **argv) {
	vmrun(NULL);
	Context *ctx = new Context();
	if(argc >= 2) {
		runFromFile(argv[1], ctx);
	} else {
		interactive(ctx);
	}
	delete ctx;
	return 0;
}

