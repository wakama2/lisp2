#ifndef LISP_H
#define LISP_H

//------------------------------------------------------
// configuration

#define USING_THCODE
#define INLINE_DEPTH 4
#define WORKER_MAX 8
#define TASK_MAX   (WORKER_MAX * 3 / 2)
#define TASKQUEUE_MAX   32 /* must be 2^n */
#define TASK_STACKSIZE 1024

//------------------------------------------------------
// includes and structs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>

struct Code;
struct Value;
struct Func;
struct Variable;
struct Cons;
struct WorkerThread;
struct Task;
class Scheduler;
class Context;
class CodeBuilder;

//------------------------------------------------------
// atomic function

#define ATOMIC_ADD(p, v) __sync_fetch_and_add(&(p), v)
#define ATOMIC_SUB(p, v) __sync_fetch_and_sub(&(p), v)
#define CAS(a, ov, nv) __sync_bool_compare_and_swap(&(a), ov, nv)

#include "array.h"
//------------------------------------------------------
// instruction, code, value

enum Instructions {
#define I(a) INS_##a,
#include "inst"
#undef I
	INS_COUNT,
};

struct Code {
	union {
		int64_t i;
		void *ptr;
		Func *func;
		Variable *var;
	};
};

struct Value {
	union {
		int64_t i;
		double d;
		const char *str;
		Task *task;
		Code *pc;
		Value *sp;
	};
};

//------------------------------------------------------
// function, variable

enum ValueType {
	VT_INT,
	VT_FLOAT,
	VT_BOOLEAN, // T or NIL
	VT_FUTURE,
	VT_VOID,
};

typedef ValueType (*CodeGenFunc)(Func *, Cons *, CodeBuilder *, int sp);

struct Func {
	Code *code;
	const char *name;
	int argc;
	const char **args;
	CodeGenFunc codegen;
	Func *next;
};

struct Variable {
	Value value;
	ValueType type;
	const char *name;
	Variable *next;
};

#include "scheduler.h"
#include "parse.h"
#include "codegen.h"

//------------------------------------------------------
// context

class Context {
private:
	Func *funclist;
	Variable *varlist;

public:
	Scheduler *sche;
	bool flagShowIR;

	Context();
	~Context();
	void putFunc(Func *func);
	Func *getFunc(const char *name);
	void putVar(Variable *var);
	Variable *getVar(const char *name);
	const char *getInstName(int ins);
#ifdef USING_THCODE
	void *jmptable[INS_COUNT];
	void *getDTLabel(int ins); /* direct threaded code label */
#endif
};

void addDefaultFuncs(Context *ctx);

//------------------------------------------------------
// cons cell

enum ConsType {
	CONS_INT,
	CONS_STR,
	CONS_CAR,
};

struct Cons {
	ConsType type;
	union {
		int i;
		const char *str;
		Cons *car;
	};
	Cons *cdr;

	Cons(ConsType type) { this->type = type; car = NULL; cdr = NULL; }
};

void cons_free(Cons *cons);
void cons_print(Cons *cons, FILE *fp = stdout);
void cons_println(Cons *cons, FILE *fp = stdout);

bool parseCons(Tokenizer *tk, Cons **res);

#endif

