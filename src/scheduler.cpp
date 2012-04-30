#include "lisp.h"

static void *WorkerThread_main(void *arg) {
	WorkerThread *wth = (WorkerThread *)arg;
	Scheduler *sche = wth->sche;
	Context *ctx = sche->getCtx();
	Task *task;
	while((task = sche->dequeue()) != NULL) {
		assert(task->stat == TASK_RUN);
		vmrun(ctx, wth, task);
	}
	return NULL;
}

//------------------------------------------------------
Scheduler::Scheduler(Context *ctx) {
	this->ctx = ctx;
	this->taskEnqIndex = 0;
	this->taskDeqIndex = 0;
	this->dead_flag = false;
	this->waitCount = 0;
	pthread_mutex_init(&tl_lock, NULL);
	pthread_cond_init(&tl_cond, NULL);
	pthread_cond_init(&tl_maincond, NULL);
}

Scheduler::~Scheduler() {
	pthread_mutex_lock(&tl_lock);
	dead_flag = true;
	pthread_cond_broadcast(&tl_cond);
	pthread_mutex_unlock(&tl_lock);
	for(int i=0; i<ctx->workers; i++) {
		WorkerThread *wth = &wthpool[i];
		pthread_join(wth->pth, NULL);
		pthread_detach(wth->pth);
	}
	delete [] wthpool;
	delete [] taskpool;
	delete [] taskq;
}

void Scheduler::initWorkers() {
	int TASK_MAX = ctx->workers * 3 / 2;
	int TASKQUEUE_MAX = 1;
	for(int i=TASK_MAX; i!=0; i>>=1) {
		TASKQUEUE_MAX<<=1; // max must be 2^n
	}
	queuemask = TASKQUEUE_MAX-1;
	// init tasks
	taskq = new Task *[TASKQUEUE_MAX];
	taskpool = new Task[TASK_MAX];
	freelist = &taskpool[0];
	for(int i=0; i<TASK_MAX; i++) {
		taskpool[i].next = &taskpool[i+1];
	}
	taskpool[TASK_MAX - 1].next = NULL;

	// start worker threads
	wthpool = new WorkerThread[ctx->workers];
	for(int i=0; i<ctx->workers; i++) {
		WorkerThread *wth = &wthpool[i];
		wth->ctx = ctx;
		wth->sche = this;
		wth->id = i;
		pthread_create(&wth->pth, NULL, WorkerThread_main, wth);
	}
}

//------------------------------------------------------
void Scheduler::enqueue(Task *task) {
	pthread_mutex_lock(&tl_lock);
	int n = taskEnqIndex++;
	taskq[n & queuemask] = task;
	if(waitCount != 0) {
		pthread_cond_signal(&tl_cond); /* notify a thread in dequeue */
	}
	pthread_mutex_unlock(&tl_lock);
}

Task *Scheduler::dequeue() {
	Task *task = NULL;
	pthread_mutex_lock(&tl_lock);
	{
		while(taskEnqIndex == taskDeqIndex) {
			if(dead_flag) goto L_FINAL;
			waitCount++;
			if(waitCount == ctx->workers) {
				pthread_cond_broadcast(&tl_maincond);
			}
			pthread_cond_wait(&tl_cond, &tl_lock); /* wait task enqueue */
			waitCount--;
		}
		int n = taskDeqIndex++;
		task = taskq[n & queuemask];
	}
L_FINAL:
	pthread_mutex_unlock(&tl_lock);
	return task;
}
	
//------------------------------------------------------
void Scheduler::enqueueWaitFor(Task *task) {
	//enqueue(task);
	pthread_mutex_lock(&tl_lock);
	// FIXME: COPIPE from euqueue
	int n = taskEnqIndex++;
	taskq[n & queuemask] = task;
	pthread_cond_signal(&tl_cond);
	pthread_cond_wait(&tl_maincond, &tl_lock);
	pthread_mutex_unlock(&tl_lock);
}

//------------------------------------------------------
Task *Scheduler::newTask(Func *func, Value *args) {
	Task *oldtop = freelist;
	if(oldtop != NULL) {
		Task *newtop = oldtop->next;
		if(CAS(freelist, oldtop, newtop)) {
			// init
			Task *task = oldtop;
#ifdef USING_THCODE
			task->pc = func->thcode;
#else
			task->pc = func->code;
#endif
			task->sp = task->stack;
			task->stat = TASK_RUN;
			memcpy(task->sp, args, func->argc * sizeof(Value));
			return task;
		}
	}
	return NULL;
}

void Scheduler::deleteTask(Task *task) {
	while(true) {
		Task *oldtop = freelist;
		task->next = oldtop;
		if(CAS(freelist, oldtop, task)) break;
	}
}

