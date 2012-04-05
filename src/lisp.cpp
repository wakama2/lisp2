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

static void interactive() {
	while(true) {
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			break;
		}
		add_history(in);
		Cons *c = parseExpr(in);
		if(c != NULL) {
			CodeBuilder *cb = new CodeBuilder();
			codegen(c, cb);
			delete cb;
		}
		free(in);
	}
}

static void runFromFile(const char *filename) {
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
	//eval_init();
	if(argc >= 2) {
		runFromFile(argv[1]);
	} else {
		interactive();
	}
	return 0;
}

