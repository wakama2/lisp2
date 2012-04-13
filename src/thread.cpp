#include "lisp.h"

static void *WorkerThread_main(void *ptr) {
	WorkerThread *wth = (WorkerThread *)ptr;
	wth->lock();
	pthread_cond_wait(&wth->cond, &wth->lock);
	wth->unlock();
	vmrun(wth->ctx, wth);
	return NULL;
}

//------------------------------------------------------
Context::Context() {
	funcLen = 0;
	threadCount = 0;
	freeThread = &wth[0];
	pthread_mutex_init(&mutex, NULL);
	// init worker thread
	for(int i=0; i<TH_MAX; i++) {
		WorkerThread *w = wth + i;
		w->ctx = this;
		w->next = this->wth + i + 1;
		w->parent = NULL;
		pthread_mutex_init(&w->lock, NULL);
		pthread_cond_init(&w->cond, NULL);
		pthread_create(&w->pth, NULL, WorkerThread_main, w);
	}
	wth[TH_MAX-1].next = NULL;
	// init jmptable
	vmrun(this, NULL);
}

Context::~Context() {

}

void Context::lock() { pthread_mutex_lock(&mutex); }
void Context::unlock() { pthread_mutex_unlock(&mutex); }

//------------------------------------------------------
static int eval2_getResult(Future *f) {
	WorkerThread *wth = f->wth;
	joinWorkerThread(wth);
	int res = wth->stack[0].i;
	deleteWorkerThread(wth);
	return res;
}

static int getThreadId(WorkerThread *wth) {
	return wth == NULL ? -1 : (wth - wth->ctx->wth);
}

WorkerThread *newWorkerThread(Context *ctx, WorkerThread *parent, Code *pc, int argc, Value *argv) {
	WorkerThread *wth = NULL;

	ctx->lock();
	//ATOMIC_ADD(&ctx->threadCount, 1);
	ctx->threadCount++;

	wth = ctx->freeThread;
	if(wth != NULL) {
		ctx->freeThread = wth->next;
	}
	ctx->unlock();
	if(wth == NULL) {
		//abort();
		return NULL;
	}
	wth->parent = parent;
	if(argc == 1) printf("[%d]: start %d p=%d\n", getThreadId(wth), argv[0].i, getThreadId(wth->parent));

	//WorkerThread *wth = new WorkerThread();
	wth->pc = pc;
	wth->fp = wth->frame;
	wth->sp = wth->stack;
	if(argc != 0) {
		memcpy(wth->stack, argv, sizeof(argv[0]) * argc);
	}
	Future *future = &wth->future;
	future->wth = wth;
	future->getResult = eval2_getResult;
	return wth;
}

void joinWorkerThread(WorkerThread *wth) {
	printf("[%d]: join\n", getThreadId(wth));
	pthread_join(wth->pth, NULL);
}

void deleteWorkerThread(WorkerThread *wth) {
	//delete wth;
	Context *ctx = wth->ctx;
	ctx->lock();
	ctx->threadCount--;
	//wth->next = ctx->freeThread;
	//ctx->freeThread = wth;
	//printf("[%d]: exit\n", getThreadId(wth));
	ctx->unlock();
}

void WorkerThread::lock() { pthread_mutex_lock(&mutex); }
void WorkerThread::unlock() { pthread_mutex_unlock(&mutex); }

