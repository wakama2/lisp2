#include "lisp.h"

static ValueType genSpawn(Func *func, Cons *cons, CodeBuilder *cb, int sp);

static int getArgIndex(Func *func, const char *name) {
	for(int i=0; i<(int)func->argc; i++) {
		if(strcmp(name, func->args[i]) == 0) {
			return i;
		}
	}
	return -1;
}

ValueType codegen(Cons *cons, CodeBuilder *cb, int sp, bool spawn) {
	if(cons->type == CONS_INT) {
		cb->createIConst(sp, cons->i);
		return VT_INT;
	} else if(cons->type == CONS_STR) {
		const char *name = cons->str;
		if(strcmp(name, "t") == 0) {
			cb->createIConst(sp, 1);
			return VT_BOOLEAN;
		}
		if(strcmp(name, "nil") == 0) {
			cb->createIConst(sp, 0);
			return VT_BOOLEAN;
		}
		if(cb->getFunc() != NULL) {
			int n = getArgIndex(cb->getFunc(), name);
			if(n != -1) {
				cb->createMov(sp, n);
				return VT_INT;
			}
		}
		Variable *var = cb->getCtx()->getVar(name);
		if(var != NULL) {
			cb->createLoadGlobal(sp, var);
			return var->type;
		}
		fprintf(stderr, "symbol not found: %s\n", cons->str);
		throw "";
	} else if(cons->type == CONS_CAR) {
		if(cons->car == NULL || cons->car->type != CONS_STR) {
			fprintf(stderr, "not function\n");
			throw "";
		}
		const char *name = cons->car->str;
		Func *func = cb->getCtx()->getFunc(name);
		if(func == NULL) {
			fprintf(stderr, "not function\n");
			throw "";
		}
		Cons *args = cons->car->cdr;
		if(spawn && func->args != NULL) {
			return genSpawn(func, cons->car->cdr, cb, sp);
		} else {
			return func->codegen(func, args, cb, sp);
		}
	} else {
		fprintf(stderr, "integer require\n");
		throw "";
	}
}

static ValueType genAdd(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) {
		cb->createIConst(sp, 0);
		return VT_INT;
	}
	ValueType vt = codegen(cons, cb, sp, cons->cdr != NULL);
	cons = cons->cdr;
	bool tf = vt == VT_FUTURE;
	for(; cons != NULL; cons = cons->cdr) {
		int sft = tf ? 2 : 1;
		if(codegen(cons, cb, sp + sft) != VT_INT) {
			fprintf(stderr, "not integer\n");
			throw "";
		}
		if(tf) {
			cb->createJoin(sp);
			tf = false;
		}
		cb->createIAdd(sp, sp + sft);
	}
	return VT_INT;
}

static ValueType genSub(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	if(cons == NULL) return VT_INT;
	codegen(cons, cb, sp);
	cons = cons->cdr;
	if(cons == NULL) {
		cb->createINeg(sp);
		return VT_INT;
	}
	for(; cons != NULL; cons = cons->cdr) {
		if(codegen(cons, cb, sp + 1) != VT_INT) {
			fprintf(stderr, "not integer\n");
			throw "";
		}
		cb->createISub(sp, sp + 1);
	}
	return VT_INT;
}

static ValueType genMul(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	codegen(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + 1);
		cb->createIMul(sp, sp + 1);
	}
	return VT_INT;
}

static ValueType genDiv(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	codegen(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + 1);
		cb->createIDiv(sp, sp + 1);
	}
	return VT_INT;
}

static ValueType genMod(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	codegen(cons, cb, sp);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + 1);
		cb->createIMod(sp, sp + 1);
	}
	return VT_INT;
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

#define genCondFunc(_fname, _op) \
static ValueType _fname(Func *, Cons *cons, CodeBuilder *cb, int sp) { \
	if(cons->cdr == NULL) { \
		cb->createIConst(sp, 1); \
		return VT_BOOLEAN; \
	} \
	codegen(cons, cb, sp); \
	codegen(cons->cdr, cb, sp + 1); \
	int l = cb->createCondOp(_op, sp, sp + 1); \
	cb->createIConst(sp, 1); \
	int m = cb->createJmp(); \
	cb->setLabel(l); \
	cb->createIConst(sp, 0); \
	cb->setLabel(m); \
	return VT_BOOLEAN; \
}

genCondFunc(genLT, INS_IJMPGE);
genCondFunc(genLE, INS_IJMPGT);
genCondFunc(genGT, INS_IJMPLE);
genCondFunc(genGE, INS_IJMPLT);
genCondFunc(genEQ, INS_IJMPNE);
genCondFunc(genNE, INS_IJMPEQ);

static ValueType genIf(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	Cons *cond = cons;
	cons = cons->cdr;
	Cons *thenCons = cons;
	cons = cons->cdr;
	Cons *elseCons = cons;
	// cond
	int op, label;
	if(cond->type == CONS_CAR && (op = toOp(cond->car->str)) != -1) {
		Cons *lhs = cond->car->cdr;
		Cons *rhs = lhs->cdr;
		codegen(lhs, cb, sp);
		codegen(rhs, cb, sp+1);
		label = cb->createCondOp(op, sp, sp+1);
	} else {
		ValueType cty = codegen(cond, cb, sp);
		if(cty == VT_BOOLEAN) {
			cb->createIConst(sp + 1, 0); /* nil */
			op = INS_IJMPEQ;
			label = cb->createCondOp(op, sp, sp+1);
		} else {
			label = -1;
		}
	}
	// then expr
	ValueType thentype = codegen(thenCons, cb, sp);
	int merge = cb->createJmp();
	// else expr
	ValueType elsetype;
	if(label != -1) cb->setLabel(label);
	if(elseCons != NULL) {
		elsetype = codegen(elseCons, cb, sp);
	} else {
		cb->createIConst(sp, 0); // NIL
		elsetype = VT_BOOLEAN;
	}
	cb->setLabel(merge);
	return thentype == elsetype ? thentype : VT_INT;
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
static ValueType genCall(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	ArrayBuilder<ValueType> vals;
	int n = 0;
	sp += RSFT;
	for(; cons != NULL; cons = cons->cdr) {
		ValueType v = codegen(cons, cb, sp + n, cons->cdr != NULL);
		vals.add(v);
		n += v == VT_FUTURE ? 2 : 1;
	}
	n = 0;
	for(int i=0, j=vals.getSize(); i<j; i++) {
		if(vals[i] == VT_FUTURE) {
			cb->createJoin(sp + n);
			if(n != i) cb->createMov(sp + i, sp + n);
			n += 2;
		} else {
			if(n != i) cb->createMov(sp + i, sp + n);
			n += 1;
		}
	}
	cb->createCall(func, sp);
	return VT_INT;
}

#define SRSFT 3
static ValueType genSpawn(Func *func, Cons *cons, CodeBuilder *cb, int sp) {
	int n = 0;
	for(; cons != NULL; cons = cons->cdr) {
		codegen(cons, cb, sp + SRSFT + n);
		n++;
	}
	cb->createSpawn(func, sp + SRSFT);
	return VT_FUTURE;
}

static ValueType genSetq(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	assert(cons->type == CONS_STR);
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *expr = cons;

	Variable *v = new Variable();
	v->name = newStr(name);
	v->value.i = 0;
	cb->getCtx()->putVar(v);

	v->type = codegen(expr, cb, sp);
	cb->createStoreGlobal(sp, v);
	return v->type;
}

static ValueType genDefun(Func *, Cons *cons, CodeBuilder *cb, int sp) {
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *args = cons->car;
	cons = cons->cdr;
	Func *func = newFunc(name, args, genCall);

	Context *ctx = cb->getCtx();
	ctx->putFunc(func);

	CodeBuilder newCb(ctx, func, false, true);
	codegen(cons, &newCb, func->argc);
	newCb.createRet(func->argc);
	newCb.createEnd();
	func->code = newCb.getCode();
	func->codeLength = newCb.getCodeLength();
	codeopt(ctx, func);
	return VT_VOID;
}

void addDefaultFuncs(Context *ctx) {
	ctx->putFunc(newFunc("+" , NULL, genAdd));
	ctx->putFunc(newFunc("-" , NULL, genSub));
	ctx->putFunc(newFunc("*" , NULL, genMul));
	ctx->putFunc(newFunc("/" , NULL, genDiv));
	ctx->putFunc(newFunc("mod" , NULL, genMod));
	ctx->putFunc(newFunc("<" , NULL, genLT));
	ctx->putFunc(newFunc(">" , NULL, genGT));
	ctx->putFunc(newFunc("<=", NULL, genLE));
	ctx->putFunc(newFunc(">=", NULL, genGE));
	ctx->putFunc(newFunc("=", NULL, genEQ));
	ctx->putFunc(newFunc("eq", NULL, genEQ));
	ctx->putFunc(newFunc("equal", NULL, genEQ));
	ctx->putFunc(newFunc("!=", NULL, genNE));
	ctx->putFunc(newFunc("if", NULL, genIf));
	ctx->putFunc(newFunc("setq", NULL, genSetq));
	ctx->putFunc(newFunc("defun", NULL, genDefun));
}

