#include "lisp.h"

const char *getInstName(int inst) {
	switch(inst) {
#define I(a) case a: return #a;
#include "inst"
#undef I
		default: return "";
	}
}

static void genIntValue(Context *ctx, Cons *cons, CodeBuilder *cb) {
	if(cons->type == CONS_INT) {
		cb->createIConst(cb->sp, cons->i);
	} else if(cons->type == CONS_STR && cb->func != NULL) {
		Func *func = cb->func;
		for(int i=0; i<func->argc; i++) {
			if(strcmp(cons->str, func->args[i]) == 0) {
				cb->createMov(cb->sp, i);
				break;
			}
		}
	} else if(cons->type == CONS_CAR) {
		codegen(ctx, cons->car, cb);
	} else {
		fprintf(stderr, "integer require\n");
		exit(1);
	}
}

static void genAdd(Context *ctx, Func *func/*unused*/, Cons *cons, CodeBuilder *cb) {
	if(cons == NULL) return;
	genIntValue(ctx, cons, cb);
	cons = cons->cdr;
	int sp = cb->sp++;
	for(; cons != NULL; cons = cons->cdr) {
		// TODO
		if(cb->stype[sp] == TYPE_FUTURE) cb->createJoin(sp);
		genIntValue(ctx, cons, cb);
		if(cb->stype[sp + 1] == TYPE_FUTURE) cb->createJoin(sp + 1);
		cb->createIAdd(sp, sp+1);
	}
	cb->sp--;
}

static void genSub(Context *ctx, Func *func/*unused*/, Cons *cons, CodeBuilder *cb) {
	if(cons == NULL) return;
	genIntValue(ctx, cons, cb);
	cons = cons->cdr;
	if(cons == NULL) {
		cb->createINeg(cb->sp);
		return;
	}
	int sp = cb->sp++;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(ctx, cons, cb);
		if(cb->stype[sp] == TYPE_FUTURE) cb->createJoin(sp);
		if(cb->stype[sp + 1] == TYPE_FUTURE) cb->createJoin(sp + 1);
		cb->createISub(sp, sp + 1);
	}
	cb->sp--;
}

static Func *addfunc(Context *ctx, const char *name, Cons *args) {
	Func *f = new Func();
	f->name = name;
	f->argc = 0;
	for(; args != NULL; args = args->cdr) {
		f->args[f->argc++] = args->str;
	}
	ctx->funcs[ctx->funcLen++] = f;
	return f;
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

static void genIf(Context *ctx, Func *func/*unused*/, Cons *cons, CodeBuilder *cb) {
	Cons *cond = cons->car;
	cons = cons->cdr;
	Cons *thenCons = cons;
	cons = cons->cdr;
	Cons *elseCons = cons;

	// cond
	int op = toOp(cond->str);
	assert(op != -1);
	genIntValue(ctx, cond->cdr, cb);
	cb->sp++;
	genIntValue(ctx, cond->cdr->cdr, cb);
	cb->sp--;
	int label = cb->createCondOp(op, cb->sp, cb->sp+1);

	// then expr
	genIntValue(ctx, thenCons, cb);
	int merge = cb->createJmp();

	// else expr
	cb->setLabel(label);
	genIntValue(ctx, elseCons, cb);

	cb->setLabel(merge);
}

static void genDefun(Context *ctx, Func * /*unused*/, Cons *cons, CodeBuilder *) {
	const char *name = cons->str;
	cons = cons->cdr;
	Cons *args = cons->car;
	cons = cons->cdr;
	Func *func = addfunc(ctx, name, args);
	CodeBuilder *cb = new CodeBuilder(ctx, func);
	codegen(ctx, cons, cb);
	//cb->createRet();
	cb->createExit();
	cb->accept(func);
	delete cb;
}

static void genCall(Context *ctx, Func *func, Cons *cons, CodeBuilder *cb) {
	int sp = cb->sp;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(ctx, cons, cb);
		cb->sp++;
	}
	cb->sp = sp;
	cb->createSpawn(func, sp, sp);
}

void codegen(Context *ctx, Cons *cons, CodeBuilder *cb) {
	if(cons->type == CONS_CAR) {
		codegen(ctx, cons->car, cb);
	} else if(cons->type == CONS_STR) {
		if(strcmp(cons->str, "+") == 0) {
			genAdd(ctx, NULL, cons->cdr, cb);		
		} else if(strcmp(cons->str, "-") == 0) {
			genSub(ctx, NULL, cons->cdr, cb);		
		} else if(strcmp(cons->str, "defun") == 0) {
			genDefun(ctx, NULL, cons->cdr, cb);
		} else if(strcmp(cons->str, "if") == 0) {
			genIf(ctx, NULL, cons->cdr, cb);
		} else {
			for(int i=0; i<ctx->funcLen; i++) {
				if(strcmp(cons->str, ctx->funcs[i]->name) == 0) {
					genCall(ctx, ctx->funcs[i], cons->cdr, cb);
					return;
				}
			}
			fprintf(stderr, "eval error: \n");
			exit(1);
		}
	} else {
		fprintf(stderr, "eval error: \n");
		exit(1);
	}
}

