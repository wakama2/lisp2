#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct Context;

struct WorkerThread {
	pthread_t pth;
};

enum {
	CONS_INT,
	CONS_STR,
	CONS_CAR,
};

struct Cons {
	int type;
	union {
		int i;
		const char *str;
		Cons *car;
	};
	Cons *cdr;

	//---------------------------------
	void print(FILE *fp = stdout);
	void println(FILE *fp = stdout);
	//Cons *eval();
};

Cons *parseExpr(const char *src);
Cons *newConsI(int i, Cons *cdr);
Cons *newConsS(const char *str, Cons *cdr);
Cons *newConsCar(Cons *car, Cons *cdr);
void freeCons(Cons *cons);

struct Func;
struct Code {
	union {
		int i;
		void *ptr;
		Func *func;
	};
};
extern void *jmptable[256];

enum {
	// [r1] = v2
	INS_ICONST,
	INS_IARG,
	// [r1] += [r2]
	INS_IADD,
	INS_ISUB,
	INS_IMUL,
	INS_IDIV,
	INS_IMOD,
	// [r1] < [r2] then jmp pc+[r3]
	INS_IJMPLT,
	INS_IJMPLE,
	INS_IJMPGT,
	INS_IJMPGE,
	INS_IJMPEQ,
	INS_IJMPNE,
	// call [func]
	INS_CALL,
	// ret [r1]
	INS_RET,
	// print [r1]
	INS_IPRINT, // for debug
	INS_TNILPRINT,
	INS_EXIT,
};

struct Func {
	const char *name;
	int argc;
	const char *args[64];
	Code *code;
};
void vmrun(WorkerThread *wth);

struct Context {
	Func *funcs[256];
	int funcLen;

	Context() {
		funcLen = 0;
	}
};

class CodeBuilder {
public:
	CodeBuilder(Func *f = NULL) {
		func = f;
		code = new Code[256];
		sp = 0;
	}
	Func *func;
	Code *code;
	int sp;
	void addInst(int inst);
	void addInt(int n);
	void addFunc(Func *func);
	void accept(Func *func);
};

void codegen(Context *ctx, Cons *cons, CodeBuilder *cb);

#endif

