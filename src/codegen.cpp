#include "lisp.h"

static void genIntValue(Context *ctx, Cons *cons, CodeBuilder *cb) {
	if(cons->type == CONS_INT) {
		cb->addInst(INS_ICONST);
		cb->addInt(cb->sp);
		cb->addInt(cons->i);
	} else if(cons->type == CONS_STR && cb->func != NULL) {
		Func *func = cb->func;
		for(int i=0; i<func->argc; i++) {
			if(strcmp(cons->str, func->args[i]) == 0) {
				cb->addInst(INS_IARG);
				cb->addInt(cb->sp);
				cb->addInt(i);
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

static void genAdd(Context *ctx, Cons *cons, CodeBuilder *cb) {
	if(cons == NULL) return;
	genIntValue(ctx, cons, cb);
	cons = cons->cdr;

	int sp = cb->sp++;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(ctx, cons, cb);
		cb->addInst(INS_IADD);
		cb->addInt(sp);
		cb->addInt(sp+1);
	}
	cb->sp--;
}

static Func *addfunc(Context *ctx, const char *name, Cons *args) {
	Func *f = new Func();
	f->name = name;
	f->argc = 0;
	for(; args != NULL; args = args->cdr) {
		printf("%d: %s\n", f->argc, args->str);
		f->args[f->argc++] = args->str;
	}
	ctx->funcs[ctx->funcLen++] = f;
	return f;
}

static void genDefun(Context *ctx, Cons *cons, CodeBuilder *) {
	cons->println();
	const char *name = cons->str;
	cons = cons->cdr;
	cons->println();

	Cons *args = cons->car;
	cons = cons->cdr;
	cons->println();

	Func *func = addfunc(ctx, name, args);
	CodeBuilder *cb = new CodeBuilder(func);
	codegen(ctx, cons, cb);
	cb->accept(func);
	delete cb;
}

static void genCall(Context *ctx, Func *func, Cons *cons, CodeBuilder *cb) {
	int sp = cb->sp;
	for(; cons != NULL; cons = cons->cdr) {
		genIntValue(ctx, cons, cb);
		cb->sp++;
	}
	cb->addInst(INS_CALL);
	cb->addFunc(func);
}

void codegen(Context *ctx, Cons *cons, CodeBuilder *cb) {
	cons->println();
	if(cons->type == CONS_CAR) {
		return codegen(ctx, cons->car, cb);
	} else if(cons->type == CONS_STR) {
		if(strcmp(cons->str, "+") == 0) {
			genAdd(ctx, cons->cdr, cb);		
		} else if(strcmp(cons->str, "defun") == 0) {
			genDefun(ctx, cons->cdr, cb);
		} else {
			for(int i=0; i<ctx->funcLen; i++) {
				if(strcmp(cons->str, ctx->funcs[i]->name) == 0) {
					genCall(ctx, ctx->funcs[i], cons->cdr, cb);
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

void CodeBuilder::addInst(int inst) {
	printf("inst %d\n", inst);
}

void CodeBuilder::addInt(int n) {
	printf("%d\n", n);
}

void CodeBuilder::addFunc(Func *func) {
	printf("%s\n", func->name);
}

void CodeBuilder::accept(Func *func) {
	func->code = code;
}

