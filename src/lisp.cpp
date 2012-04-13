#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

//------------------------------------------------------
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

//------------------------------------------------------
static void runCons(Context *ctx, Cons *cons) {
	if(cons->type == CONS_CAR) {
		for(; cons != NULL; cons = cons->cdr) {
			cons->println();
			runCons(ctx, cons->car);
		}
		return;
	}
	CodeBuilder *cb = new CodeBuilder(ctx, NULL);
	cons->codegen(cb);
	if(cb->stype[0] == TYPE_FUTURE) cb->createJoin(0);
	cb->createExit();
	WorkerThread *wth = newWorkerThread(ctx, NULL, cb->code, 0, NULL);
	Future *f = &wth->future;
	printf("%d\n", f->getResult(f));
	delete cb;
}

//------------------------------------------------------
#define HISTFILE ".history"
static void runInteractive(Context *ctx) {
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

static void runFromFile(Context *ctx, const char *filename) {
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

//------------------------------------------------------
int main(int argc, char **argv) {
	Context *ctx = new Context();
	if(argc >= 2) {
		runFromFile(ctx, argv[1]);
	} else {
		runInteractive(ctx);
	}
	delete ctx;
	return 0;
}

