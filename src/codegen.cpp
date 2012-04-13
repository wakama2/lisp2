#include "lisp.h"

static void genIntValue(Cons *cons, CodeBuilder *cb, int sp) {
	if(cons->type == CONS_INT) {
		cb->createIConst(sp, cons->i);
	} else if(cons->type == CONS_STR && cb->func != NULL) {
		Func *func = cb->func;
		for(int i=0; i<func->argc; i++) {
			if(strcmp(cons->str, func->args[i]) == 0) {
				cb->createMov(sp, i);
				break;
			}
		}
	} else if(cons->type == CONS_CAR) {
		codegen(cb, cons, sp);
	} else {
		fprintf(stderr, "integer require\n");
		exit(1);
	}
}

static void genAdd(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) return;
	genIntValue(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + 1);
		cb->createIAdd(sp, sp + 1);
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

static Func *newFunc(const char *name, int argc, char **argv,
		void (*gen)(Func *self, Cons *src, CodeBuilder *cb, int sp)) {
	Func *f = new Func();
	f->name = name;
	f->argc = 0;
	for(int i=0; i<argc; i++) f->args[i] = argv[i];
	f->code = NULL;
	f->codegen = gen;
	return f;
}

static void genCall(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	int n = 0;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + n);
		n++;
	}
	cb->createCall(func, sp, sp);
}

static void genDefun(Func *, Cons *cons, CodeBuilder *_cb, int sp) {
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *args = cons->car;
	cons = cons->cdr;
	Func *func = newFunc(name, -1, NULL, genCall);
	CodeBuilder cb(_cb->ctx, func);
	codegen(&cb, cons, 0);
	cb.createRet();
	cb.accept(func);
}

void addDefaultFuncs(Context *ctx) {
	ctx->putFunc(newFunc("+", -1, NULL, genAdd));
	ctx->putFunc(newFunc("-", -1, NULL, genSub));
	//ctx->putFunc(newFunc("*", -1, NULL, genAdd));
	//ctx->putFunc(newFunc("/", -1, NULL, genAdd));
	//ctx->putFunc(newFunc("%", -1, NULL, genAdd));
	ctx->putFunc(newFunc("<", -1, NULL, genAdd));
	ctx->putFunc(newFunc(">", -1, NULL, genAdd));
	//ctx->putFunc(newFunc("<=", -1, NULL, genAdd));
	//ctx->putFunc(newFunc(">=", -1, NULL, genAdd));
	//ctx->putFunc(newFunc("==", -1, NULL, genAdd));
	//ctx->putFunc(newFunc("!=", -1, NULL, genAdd));
	ctx->putFunc(newFunc("if", -1, NULL, genAdd));
	//ctx->putFunc(newFunc("setq", -1, NULL, genAdd));
	ctx->putFunc(newFunc("defun", -1, NULL, genDefun));
}

void codegen(CodeBuilder *cb, Cons *cons, int sp) {
	assert(cons->type == CONS_STR);
	const char *name = cons->str;
	Cons *args = cons->cdr;
	Func *func = cb->ctx->getFunc(name);
	assert(func != NULL);
	func->codegen(func, args, cb, sp);
}

