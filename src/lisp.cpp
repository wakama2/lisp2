#include "lisp.h"

HashMap *funcMap;
HashMap *varMap;

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

int main(void) {
	eval_init();
	while(true) {
		printf(">>");
		char is[256];
		if(fgets(is, 256, stdin) == NULL) break;
		Cons *c = parseExpr(is);
		c->eval()->println();
	}
	return 0;
}

