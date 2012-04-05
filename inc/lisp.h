#ifndef LISP_H
#define LISP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct Worker {
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
	Cons *eval();
};

Cons *parseExpr(const char *src);
Cons *newConsI(int i, Cons *cdr);
Cons *newConsS(const char *str, Cons *cdr);
Cons *newConsCar(Cons *car, Cons *cdr);
void freeCons(Cons *cons);

#define MAP_MAX 256
class HashMap {
private:
	const char *keys[MAP_MAX];
	void *values[MAP_MAX];
public:
	HashMap();
	void put(const char *key, void *value);
	void *get(const char *key);
};

struct Func {
	const char *name;
	Cons *args;
	Cons *expr;
};

void eval_init();
extern HashMap *funcMap;
extern HashMap *varMap;

#endif

