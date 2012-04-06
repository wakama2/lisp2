#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

void *jmptable[256];

Cons *newConsI(int i, Cons *cdr) {
	Cons *c = new Cons();
	c->type = CONS_INT;
	c->i = i;
	c->cdr = cdr;
	return c;
}

Cons *newConsS(const char *str, Cons *cdr) {
	Cons *c = new Cons();
	c->type = CONS_STR;
	c->str = str;
	c->cdr = cdr;
	return c;
}

Cons *newConsCar(Cons *car, Cons *cdr) {
	Cons *c = new Cons();
	c->type = CONS_CAR;
	c->car = car;
	c->cdr = cdr;
	return c;
}

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

static void interactive(Context *ctx) {
	while(true) {
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			break;
		}
		add_history(in);
		Cons *c = parseExpr(in);
		if(c != NULL) {
			CodeBuilder *cb = new CodeBuilder();
			codegen(ctx, c, cb);
			cb->addInst(INS_EXIT);

			WorkerThread *wth = new WorkerThread();
			wth->pc = cb->code;
			vmrun(wth);
			printf("%d\n", wth->stack[0]);
			delete wth;
			delete cb;
		}
		free(in);
	}
}

static void runFromFile(const char *filename, Context *ctx) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) return;
	char is[256];
	fgets(is, 256, fp);
	fclose(fp);

	//while(true) {
		//Cons *c = parseExpr(is);
		//if(c == NULL) break;
		//c->eval()->println();
	//}
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

