#include "lisp.h"

static void *WorkerThread_main(void *arg) {
	WorkerThread *wth = (WorkerThread *)arg;
	Scheduler *sche = wth->sche;
	Task *task;
	while((task = sche->dequeue()) != NULL) {
		assert(task->stat == TASK_RUN);
		vmrun(sche->ctx, wth, task);
	}
	return NULL;
}

//------------------------------------------------------
Scheduler::Scheduler(Context *ctx) {
	this->ctx = ctx;
	this->taskq = new Task *[TASKQUEUE_MAX];
	this->taskEnqIndex = 0;
	this->taskDeqIndex = 0;
	this->dead_flag = false;
	pthread_mutex_init(&tl_lock, NULL);
	pthread_cond_init(&tl_cond, NULL);

	// init task list
	taskpool = new Task[TASK_MAX];
	freelist = &taskpool[0];
	for(int i=0; i<TASK_MAX; i++) {
		taskpool[i].next = &taskpool[i+1];
	}
	taskpool[TASK_MAX - 1].next = NULL;

	// start worker threads
	wthpool = new WorkerThread[WORKER_MAX];
	for(int i=0; i<WORKER_MAX; i++) {
		WorkerThread *wth = &wthpool[i];
		wth->ctx = ctx;
		wth->sche = this;
		wth->id = i;
		pthread_create(&wth->pth, NULL, WorkerThread_main, wth);
	}
}

Scheduler::~Scheduler() {
	pthread_mutex_lock(&tl_lock);
	dead_flag = true;
	pthread_cond_broadcast(&tl_cond);
	pthread_mutex_unlock(&tl_lock);
	for(int i=0; i<WORKER_MAX; i++) {
		WorkerThread *wth = &wthpool[i];
		pthread_join(wth->pth, NULL);
		pthread_detach(wth->pth);
	}
	delete [] wthpool;
	delete [] taskpool;
	delete [] taskq;
}

//------------------------------------------------------
void Scheduler::enqueue(Task *task) {
	pthread_mutex_lock(&tl_lock);
	int n = taskEnqIndex++ & (TASKQUEUE_MAX - 1);
	taskq[n] = task;
	pthread_cond_signal(&tl_cond); /* notify a thread in dequeue */
	pthread_mutex_unlock(&tl_lock);
}

Task *Scheduler::dequeue() {
	Task *task = NULL;
	pthread_mutex_lock(&tl_lock);
	{
		while(taskEnqIndex == taskDeqIndex) {
			if(dead_flag) goto L_FINAL;
			pthread_cond_wait(&tl_cond, &tl_lock); /* wait task enqueue */
		}
		int n = (taskDeqIndex++) & (TASKQUEUE_MAX - 1);
		task = taskq[n];
	}
L_FINAL:
	pthread_mutex_unlock(&tl_lock);
	return task;
}
	
//------------------------------------------------------
Task *Scheduler::newTask(Func *func, Value *args, TaskMethod dest) {
	Task *oldtop = freelist;
	if(oldtop != NULL) {
		Task *newtop = oldtop->next;
		if(CAS(freelist, oldtop, newtop)) {
			// init
			Task *task = oldtop;
			task->pc = func->code;
			task->sp = task->stack;
			task->dest = dest;
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

