#include "lisp.h"

static void genSpawn(Func *func, Cons *cons, CodeBuilder *cb, int sp);

static int getArgIndex(Func *func, const char *name) {
	for(int i=0; i<func->argc; i++) {
		if(strcmp(name, func->args[i]) == 0) {
			return i;
		}
	}
	return -1;
}

static void genIntValue(Cons *cons, CodeBuilder *cb, int sp) {
	if(cons->type == CONS_INT) {
		cb->createIConst(sp, cons->i);
	} else if(cons->type == CONS_STR) {
		Func *func = cb->func;
		if(func != NULL) {
			int n = getArgIndex(func, cons->str);
			if(n != -1) {
				cb->createMov(sp, n);
			} else {
				fprintf(stderr, "symbol not found: %s\n", cons->str);
				exit(1);
			}
		} else {
			fprintf(stderr, "symbol not found: %s\n", cons->str);
			exit(1);
		}
	} else if(cons->type == CONS_CAR) {
		cb->codegen(cons, sp);
	} else {
		fprintf(stderr, "integer require\n");
		exit(1);
	}
}

static bool genIntTask(Cons *cons, CodeBuilder *cb, int sp) {
	if(cons->type == CONS_CAR) {
		const char *name = cons->car->str;
		Func *func = cb->ctx->getFunc(name);
		assert(func != NULL);
		genSpawn(func, cons->car->cdr, cb, sp);
		return true;
	} else {
		genIntValue(cons, cb, sp);
		return false;
	}
}

static void genAdd(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) return;
	bool tf = genIntTask(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		int sft = tf ? 3 : 1;
		genIntValue(cons, cb, sp + sft);
		if(tf) {
			cb->createJoin(sp);
			tf = false;
		}
		cb->createIAdd(sp, sp + sft);
	}
}

static void genSub(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) return;
	genIntValue(cons, cb, sp);
	cons = cons->cdr;
	if(cons == NULL) {
		cb->createINeg(sp);
		return;
	}
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + 1);
		cb->createISub(sp, sp + 1);
	}
}

static int toOp(const char *s) {
	if(strcmp(s, "<") == 0)  return INS_IJMPGE;
	if(strcmp(s, "<=") == 0) return INS_IJMPGT;
	if(strcmp(s, ">") == 0)  return INS_IJMPLE;
	if(strcmp(s, ">=") == 0) return INS_IJMPLT;
	if(strcmp(s, "==") == 0) return INS_IJMPNE;
	if(strcmp(s, "!=") == 0) return INS_IJMPEQ;
	return -1;
}

// FIXME
static void genIf(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	Cons *cond = cons->car;
	cons = cons->cdr;
	Cons *thenCons = cons;
	cons = cons->cdr;
	Cons *elseCons = cons;
	// cond
	int op = toOp(cond->str);
	assert(op != -1);
	genIntValue(cond->cdr, cb, sp);
	genIntValue(cond->cdr->cdr, cb, sp + 1);
	int label = cb->createCondOp(op, sp, sp+1);

	// then expr
	genIntValue(thenCons, cb, sp);
	int merge = cb->createJmp();

	// else expr
	cb->setLabel(label);
	genIntValue(elseCons, cb, sp);

	cb->setLabel(merge);
}

static Func *newFunc(const char *name, Cons *args, 
		void (*gen)(Func *self, Cons *src, CodeBuilder *cb, int sp)) {
	Func *f = new Func();
	f->name = name;
	f->argc = 0;
	for(Cons *c=args; c!=NULL; c=c->cdr) {
		f->args[f->argc++] = c->str;
	}
	f->code = NULL;
	f->codegen = gen;
	return f;
}

#define RSFT 2

static void genCall(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	int n = 0;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + RSFT + n);
		n++;
	}
	cb->createCall(func, sp + RSFT, 0/* unused */);
}

#define SRSFT 3
static void genSpawn(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	int n = 0;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + SRSFT + n);
		n++;
	}
	cb->createSpawn(func, sp + SRSFT, 0);
}

static void genDefun(Func *, Cons *cons, CodeBuilder *_cb, int sp) {
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *args = cons->car;
	cons = cons->cdr;
	Func *func = newFunc(name, args, genCall);
	_cb->ctx->putFunc(func);

	CodeBuilder cb(_cb->ctx, func);
	cb.codegen(cons, func->argc);
	cb.createRet();
	cb.accept(func);
}

void addDefaultFuncs(Context *ctx) {
	ctx->putFunc(newFunc("+", NULL, genAdd));
	ctx->putFunc(newFunc("-", NULL, genSub));
	//ctx->putFunc(newFunc("*", NULL, genAdd));
	//ctx->putFunc(newFunc("/", NULL, genAdd));
	//ctx->putFunc(newFunc("%", NULL, genAdd));
	//ctx->putFunc(newFunc("<", NULL, genAdd));
	//ctx->putFunc(newFunc(">", NULL, genAdd));
	//ctx->putFunc(newFunc("<=", NULL, genAdd));
	//ctx->putFunc(newFunc(">=", NULL, genAdd));
	//ctx->putFunc(newFunc("==", NULL, genAdd));
	//ctx->putFunc(newFunc("!=", NULL, genAdd));
	ctx->putFunc(newFunc("if", NULL, genIf));
	//ctx->putFunc(newFunc("setq", NULL, genAdd));
	ctx->putFunc(newFunc("defun", NULL, genDefun));
}


