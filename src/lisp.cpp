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

//------------------------------------------------------
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
	cb->codegen(cons, 0);
	cb->createRet();
	cb->accept(func);

	Scheduler *sche = ctx->sche;
	Task *task = sche->newTask(func, NULL, end_script);
	assert(task != NULL);
	pthread_mutex_lock(&g_lock);
	sche->enqueue(task);
	pthread_cond_wait(&g_cond, &g_lock);
	pthread_mutex_unlock(&g_lock);

	printf("%d\n", task->stack[0].i);
	sche->deleteTask(task);
	delete cb;
}

//------------------------------------------------------
static void runLisp(Context *ctx, Reader r, void *rp) {
	Tokenizer tk(r, rp);
	Cons *res;
	while(!tk.isEof()) {
		if(parseCons(&tk, &res)) {
			res->cdr = NULL;
			res->println();
			runCons(ctx, res);
		} else {
			printf("error\n");
			break;
		}
	}
}

//------------------------------------------------------
struct CharReader {
	const char *in;
	int index;
};

static int reader_char(void *p) {
	CharReader *cr = (CharReader *)p;
	char ch = cr->in[cr->index];
	if(ch) {
		cr->index++;
		return ch;
	}
	return EOF;
}

static int reader_file(void *p) {
	FILE *fp = (FILE *)p;
	return fgetc(fp);
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
		CharReader cr = { in, 0 };
		runLisp(ctx, reader_char, &cr);
		free(in);
		write_history(HISTFILE);
	}
}

static void runFromFile(Context *ctx, const char *filename) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "file open error: %s\n", filename);
		return;
	}
	runLisp(ctx, reader_file, fp);
	fclose(fp);
}

//------------------------------------------------------
int main(int argc, char **argv) {
	Context *ctx = new Context();
	ctx->sche = new Scheduler(ctx);
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

