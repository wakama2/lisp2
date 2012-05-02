#ifndef LISP_H
#define LISP_H

//------------------------------------------------------
// configuration

#define USING_THCODE
#define TASK_STACKSIZE 1024*4

//------------------------------------------------------
// includes and structs

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include "array.h"

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
// builtin function

#define ATOMIC_ADD(p, v) __sync_fetch_and_add(&(p), v)
#define ATOMIC_SUB(p, v) __sync_fetch_and_sub(&(p), v)
#define CAS(a, ov, nv) __sync_bool_compare_and_swap(&(a), ov, nv)

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#define prefetch(...)  __builtin_prefetch(__VA_ARGS__)

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
		Cons *cons;
	};
};

struct Value {
	union {
		int64_t i;
		double f;
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
#ifdef USING_THCODE
	Code *thcode;
#endif
	Code *code;
	int codeLength;
	const char *name;
	size_t argc;
	const char **args;
	ValueType rtype;
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
	int inlinecount;
	int workers;

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
	CONS_FLOAT,
	CONS_STR,
	CONS_CAR,
};

struct Cons {
	ConsType type;
	union {
		int i;
		double f;
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

