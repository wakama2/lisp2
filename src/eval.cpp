#include "lisp.h"

static void genIntValue(Cons *cons, CodeBuilder *cb) {
	if(cons->type == CONS_INT) {
		cb->addInst(INS_ICONST);
		cb->addInt(cb->sp);
		cb->addInt(cons->i);
	} else if(cons->type == CONS_CAR) {
		codegen(cons->car, cb);
	} else {
		fprintf(stderr, "integer require\n");
		exit(1);
	}
}

static void genAdd(Cons *cons, CodeBuilder *cb) {
	if(cons == NULL) return;
	genIntValue(cons, cb);
	cons = cons->cdr;

	int sp = cb->sp++;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb);
		cb->addInst(INS_IADD);
		cb->addInt(sp);
		cb->addInt(sp+1);
	}
	cb->sp--;
}

void codegen(Cons *cons, CodeBuilder *cb) {
	if(cons->type == CONS_CAR) {
		return codegen(cons->car, cb);
	} else if(cons->type == CONS_STR) {
		if(strcmp(cons->str, "+") == 0) {
			genAdd(cons->cdr, cb);		
		} else if(strcmp(cons->str, "defun") == 0) {

		} else {
			fprintf(stderr, "eval error: \n");
			exit(1);
		}
	} else {
		fprintf(stderr, "eval error: \n");
		exit(1);
	}
}

void CodeBuilder::addInst(int inst) {
	printf("inst %d\n", inst);
}

void CodeBuilder::addInt(int n) {
	printf("%d\n", n);
}

