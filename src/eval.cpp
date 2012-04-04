#include "lisp.h"

static int evalToInt(Cons *cons);

typedef Cons *(*EvalFunc)(Cons *cons);

// funcs
static Cons *evalAdd(Cons *cons) {
	int res = 0;
	for(; cons != NULL; cons = cons->cdr) {
		res += evalToInt(cons);
	}
	return newConsI(res, NULL);
}

static Cons *evalMul(Cons *cons) {
	int res = 1;
	for(; cons != NULL; cons = cons->cdr) {
		res *= evalToInt(cons);
	}
	return newConsI(res, NULL);
}

static Cons *evalSub(Cons *cons) {
	if(cons == NULL) {
		fprintf(stderr, "eval error: argument require\n");
		exit(1);
	}
	int res = evalToInt(cons);
	cons = cons->cdr;
	if(cons == NULL) {
		return newConsI(-res, NULL);
	} else {
		for(; cons != NULL; cons = cons->cdr) {
			res -= evalToInt(cons);
		}
		return newConsI(res, NULL);
	}
}

static Cons *evalDiv(Cons *cons) {
	if(cons == NULL) {
		fprintf(stderr, "eval error: argument require\n");
		exit(1);
	}
	int res = evalToInt(cons);
	cons = cons->cdr;
	for(; cons != NULL; cons = cons->cdr) {
		int n = evalToInt(cons);
		if(n == 0) {
			fprintf(stderr, "Zero division error\n");
			exit(1);
		}
		res /= n;
	}
	return newConsI(res, NULL);
}

//static Cons *evalIf(Cons *cons) {
//
//}

static Cons *evalDefun(Cons *cons) {
	if(cons->type != CONS_STR) {
		fprintf(stderr, "eval error: wrong type %d\n", cons->type);
		exit(1);
	}
	const char *name = cons->str;
	cons = cons->cdr;
	if(cons->type != CONS_CAR) {
		fprintf(stderr, "eval error: wrong type %d\n", cons->type);
		exit(1);
	}
	Cons *args = cons->car;
	cons = cons->cdr;
	if(cons->type != CONS_CAR) {
		fprintf(stderr, "eval error: wrong type %d\n", cons->type);
		exit(1);
	}
	Cons *expr = cons->car;

	Func *func = new Func();
	func->name = name;
	func->args = args;
	func->expr = expr;
	return newConsI(0, NULL);
}

// int value
static int evalToInt(Cons *cons) {
	if(cons->type == CONS_INT) {
		return cons->i;
	} else if(cons->type == CONS_CAR) {
		return evalToInt(cons->car->eval());
	} else {
		fprintf(stderr, "eval error: wrong type %d\n", cons->type);
		exit(1);
	}
}

void eval_init() {
	funcMap = new HashMap();
	varMap = new HashMap();

	funcMap->put("+", (void *)evalAdd);
	funcMap->put("-", (void *)evalSub);
	funcMap->put("*", (void *)evalMul);
	funcMap->put("/", (void *)evalDiv);
}

Cons *Cons::eval() {
	if(type == CONS_CAR) {
		return car->eval();
	} else if(type == CONS_STR) {
		EvalFunc f = (EvalFunc)funcMap->get(str);
		if(f != NULL) {
			return f(cdr);
		} else {
			fprintf(stderr, "operation not found: %s\n", str);
			exit(1);
		}
	} else {
		fprintf(stderr, "eval error: type is %d\n", type);
		exit(1);
	}
}

