#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

//------------------------------------------------------
void Cons::print(FILE *fp) {
	bool b = false;
	Cons *self = this;
	while(self != NULL) {
		if(b) printf(" ");
		b = true;
		switch(self->type) {
		case CONS_INT: printf("%d", self->i); break;
		case CONS_STR: printf("%s", self->str); break;
		case CONS_CAR:
			fprintf(fp, "(");
			self->car->print(fp);
			fprintf(fp, ")");
			break;
		}
		self = self->cdr;
	}
}

void Cons::println(FILE *fp) {
	this->print(fp);
	fprintf(fp, "\n");
}


static pthread_mutex_t g_lock;
static pthread_cond_t  g_cond;

static void end_script(Task *task, WorkerThread *wth) {
	pthread_mutex_lock(&g_lock);
	pthread_cond_signal(&g_cond);
	pthread_mutex_unlock(&g_lock);
}

//------------------------------------------------------
static void runCons(Context *ctx, Cons *cons) {
	if(cons->type == CONS_CAR) {
		for(; cons != NULL; cons = cons->cdr) {
			cons->println();
			runCons(ctx, cons->car);
		}
		return;
	}
	Func *func = new Func();
	func->name = "__script";
	func->argc = 0;
	CodeBuilder *cb = new CodeBuilder(ctx, func);
	codegen(cb, cons, 0);
	cb->createRet();
	cb->accept(func);

	Scheduler *sche = new Scheduler(ctx);
	Task *task = sche->newTask(func, NULL, end_script);

	pthread_mutex_lock(&g_lock);
	sche->enqueue(task);
	pthread_cond_wait(&g_cond, &g_lock);
	pthread_mutex_unlock(&g_lock);

	printf("%d\n", task->stack[0].i);
	delete cb;
}

//------------------------------------------------------
#define HISTFILE ".history"
static void runInteractive(Context *ctx) {
	read_history(HISTFILE);
	while(true) {
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			break;
		}
		add_history(in);
		Cons *c = parseExpr(in).value;
		runCons(ctx, c);
		free(in);
		write_history(HISTFILE);
	}
}

static void runFromFile(Context *ctx, const char *filename) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) return;
	char is[256];
	int size = fread(is, 1, 256, fp);
	fclose(fp);
	is[size] = '\0';

	const char *src = is;
	while(true) {
		ParseResult<Cons *> res = parseExpr(src);
		if(res.success) {
			src = res.src;
			runCons(ctx, res.value);
		} else {
			break;
		}
	}
}

//------------------------------------------------------
int main(int argc, char **argv) {
	Context *ctx = new Context();
	pthread_mutex_init(&g_lock, NULL);
	pthread_cond_init(&g_cond, NULL);
	if(argc >= 2) {
		runFromFile(ctx, argv[1]);
	} else {
		runInteractive(ctx);
	}
	delete ctx;
	return 0;
}

