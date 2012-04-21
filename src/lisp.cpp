#include "lisp.h"
#include <readline/readline.h>
#include <readline/history.h>

//------------------------------------------------------
void cons_free(Cons *cons) {
	if(cons->type == CONS_CAR && cons->car != NULL) {
		cons_free(cons->car);
	} else if(cons->type == CONS_STR) {
		delete [] cons->str;
	}
	if(cons->cdr != NULL) {
		cons_free(cons->cdr);
	}
	delete cons;
}

void cons_print(Cons *cons, FILE *fp) {
	bool b = false;
	for(; cons != NULL; cons = cons->cdr) {
		if(b) printf(" ");
		b = true;
		switch(cons->type) {
		case CONS_INT: printf("%d", cons->i); break;
		case CONS_STR: printf("%s", cons->str); break;
		case CONS_CAR:
			fprintf(fp, "(");
			cons_print(cons->car, fp);
			fprintf(fp, ")");
			break;
		}
	}
}

void cons_println(Cons *cons, FILE *fp) {
	cons_print(cons, fp);
	fprintf(fp, "\n");
}

//------------------------------------------------------
static pthread_mutex_t g_lock;
static pthread_cond_t  g_cond;

static void notify_runCons(Task *task, WorkerThread *wth) {
	pthread_mutex_lock(&g_lock);
	pthread_cond_signal(&g_cond);
	pthread_mutex_unlock(&g_lock);
}

//------------------------------------------------------
static void runCons(Context *ctx, Cons *cons) {
	Func *func = new Func();
	func->name = "__script";
	func->argc = 0;
	CodeBuilder *cb = new CodeBuilder(ctx, func);
	cb->codegen(cons, 0);
	cb->createRet();
	func->code = cb->getCode();

	Scheduler *sche = ctx->sche;
	Task *task = sche->newTask(func, NULL, notify_runCons);
	assert(task != NULL);
	pthread_mutex_lock(&g_lock);
	//sche->enqueue(task);
	//sche->wthpool[0].queue->pushBottom(task);
	assert(sche->firsttask == NULL);
	sche->firsttask = task;
	pthread_cond_wait(&g_cond, &g_lock);
	pthread_mutex_unlock(&g_lock);

	printf("%ld\n", task->stack[0].i);
	sche->deleteTask(task);
	delete [] func->code;
	delete func;
	delete cb;
}

//------------------------------------------------------
static void compileAndRun(Context *ctx, Reader r, void *rp) {
	Tokenizer tk(r, rp);
	Cons *res;
	while(!tk.isEof()) {
		if(parseCons(&tk, &res)) {
			res->cdr = NULL;
			cons_println(res);
			runCons(ctx, res);
			cons_free(res);
		} else {
			break;
		}
	}
}

//------------------------------------------------------
#define HISTFILE "history"
struct CharReader {
	char *in;
	int index;
	int length;
	bool isEOF;
};

static int readChar(void *p) {
	CharReader *cr = (CharReader *)p;
	if(cr->isEOF) return EOF;
	while(cr->in == NULL || cr->index >= cr->length) {
		if(cr->in != NULL) {
			if(cr->index <= cr->length + 2) {
				cr->index++;
				return '\n';
			}
			free(cr->in);
		}
		char *in = readline(">>");
		if(in == NULL || strcmp(in, "exit") == 0 || strcmp(in, "quit") == 0) {
			cr->isEOF = true;
			return EOF;
		} else {
		}
		cr->in = in;
		cr->index = 0;
		cr->length = strlen(in);
		if(cr->length > 0) {
			add_history(in);
			write_history(HISTFILE);
		}
	}
	return cr->in[cr->index++];
}

static void runInteractive(Context *ctx) {
	// init readline
	read_history(HISTFILE);
	// start
	CharReader cr;
	cr.in = NULL;
	cr.isEOF = false;
	while(true) {
		compileAndRun(ctx, readChar, &cr);
		if(cr.in != NULL) {
			free(cr.in);
			cr.in = NULL;
		}
		if(cr.isEOF) break;
	}
}

//------------------------------------------------------
static int reader_file(void *p) {
	FILE *fp = (FILE *)p;
	return fgetc(fp);
}

static void runFromFile(Context *ctx, const char *filename) {
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "file open error: %s\n", filename);
		return;
	}
	compileAndRun(ctx, reader_file, fp);
	fclose(fp);
}

//------------------------------------------------------
int main(int argc, char **argv) {
	Context *ctx = new Context();
	pthread_mutex_init(&g_lock, NULL);
	pthread_cond_init(&g_cond, NULL);
	if(argc >= 2) {
		int i = 1;
		for(; i<argc; i++) {
			if(strcmp(argv[i], "-i") == 0) ctx->flagShowIR = true;
			else break;
		}
		runFromFile(ctx, argv[i]);
	} else {
		runInteractive(ctx);
	}
	delete ctx;
	return 0;
}

