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
		const char *name = cons->str;
		if(strcmp(name, "t") == 0) {
			cb->createIConst(sp, 1);
			return;
		}
		if(strcmp(name, "nil") == 0) {
			cb->createIConst(sp, 0);
			return;
		}
		if(cb->getFunc() != NULL) {
			int n = getArgIndex(cb->getFunc(), name);
			if(n != -1) {
				cb->createMov(sp, n);
				return;
			}
		}
		Variable *var = cb->getCtx()->getVar(name);
		if(var != NULL) {
			cb->createLoadGlobal(var, sp);
			return;
		}
		fprintf(stderr, "symbol not found: %s\n", cons->str);
		exit(1);
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
		Func *func = cb->getCtx()->getFunc(name);
		assert(func != NULL);
		//if(func->code != NULL) {
			genSpawn(func, cons->car->cdr, cb, sp);
			return true;
		//}
	}
	genIntValue(cons, cb, sp);
	return false;
}

static void genAdd(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) return;
	bool tf = genIntTask(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		if(cons->type == CONS_INT) {
			cb->createIAddC(sp, cons->i);
		} else {
			int sft = tf ? 3 : 1;
			genIntValue(cons, cb, sp + sft);
			if(tf) {
				cb->createJoin(sp);
				tf = false;
			}
			cb->createIAdd(sp, sp + sft);
		}
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
		if(cons->type == CONS_INT) {
			cb->createISubC(sp, cons->i);
		} else {
			genIntValue(cons, cb, sp + 1);
			cb->createISub(sp, sp + 1);
		}
	}
}

static void genMul(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	genIntValue(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + 1);
		cb->createIMul(sp, sp + 1);
	}
}

static void genDiv(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	genIntValue(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + 1);
		cb->createIDiv(sp, sp + 1);
	}
}

static void genMod(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	genIntValue(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(cons, cb, sp + 1);
		cb->createIMod(sp, sp + 1);
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
	Cons *cond = cons;
	cons = cons->cdr;
	Cons *thenCons = cons;
	cons = cons->cdr;
	Cons *elseCons = cons;
	// cond
	int op;
	if(cond->type == CONS_CAR && (op = toOp(cond->car->str)) != -1) {
		genIntValue(cond->car->cdr, cb, sp);
		genIntValue(cond->car->cdr->cdr, cb, sp + 1);
	} else {
		genIntValue(cond, cb, sp);
		cb->createIConst(sp + 1, 0); /* nil */
		op = INS_IJMPEQ;
	}
	int label = cb->createCondOp(op, sp, sp+1);

	// then expr
	genIntValue(thenCons, cb, sp);
	int merge = cb->createJmp();

	// else expr
	cb->setLabel(label);
	genIntValue(elseCons, cb, sp);

	cb->setLabel(merge);
}

static const char *newStr(const char *ss) {
	int len = strlen(ss);
	char *s = new char[len + 1];
	strcpy(s, ss);
	return s;
}

static Func *newFunc(const char *name, Cons *args, CodeGenFunc gen) {
	int argc = 0;
	for(Cons *c=args; c!=NULL; c=c->cdr) {
		argc++;
	}
	Func *f = new Func();
	f->name = newStr(name);
	f->argc = argc;
	f->args = argc != 0 ? new const char *[argc] : NULL;
	int i = 0;
	for(Cons *c=args; c!=NULL; c=c->cdr) {
		f->args[i++] = newStr(c->str);
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

static void genSetq(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	assert(cons->type == CONS_STR);
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *expr = cons;

	Variable *v = new Variable();
	v->name = newStr(name);
	v->value.i = 0;
	cb->getCtx()->putVar(v);

	genIntValue(expr, cb, sp);
	cb->createStoreGlobal(v, sp);
}

static void genDefun(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *args = cons->car;
	cons = cons->cdr;
	Func *func = newFunc(name, args, genCall);

	Context *ctx = cb->getCtx();
	ctx->putFunc(func);

	CodeBuilder newCb(ctx, func);
	newCb.codegen(cons, func->argc);
	newCb.createRet();
	func->code = newCb.getCode();
}

void addDefaultFuncs(Context *ctx) {
	ctx->putFunc(newFunc("+", NULL, genAdd));
	ctx->putFunc(newFunc("-", NULL, genSub));
	ctx->putFunc(newFunc("*", NULL, genMul));
	ctx->putFunc(newFunc("/", NULL, genDiv));
	ctx->putFunc(newFunc("%", NULL, genMod));
	//ctx->putFunc(newFunc("<", NULL, genAdd));
	//ctx->putFunc(newFunc(">", NULL, genAdd));
	//ctx->putFunc(newFunc("<=", NULL, genAdd));
	//ctx->putFunc(newFunc(">=", NULL, genAdd));
	//ctx->putFunc(newFunc("==", NULL, genAdd));
	//ctx->putFunc(newFunc("!=", NULL, genAdd));
	ctx->putFunc(newFunc("if", NULL, genIf));
	ctx->putFunc(newFunc("setq", NULL, genSetq));
	ctx->putFunc(newFunc("defun", NULL, genDefun));
}

