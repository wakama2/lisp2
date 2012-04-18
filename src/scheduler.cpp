#include "lisp.h"

static void *WorkerThread_main(void *arg) {
	WorkerThread *wth = (WorkerThread *)arg;
	Scheduler *sche = wth->sche;
	int id = wth->id;
	Task *task;

	while(!sche->dead_flag) {
		task = dequeueTop(wth);
		if(task != NULL) {
			vmrun(sche->ctx, wth, task);
		} else {

		}
	}
	return NULL;
}

//------------------------------------------------------
Scheduler::Scheduler(Context *ctx) {
	this->ctx = ctx;
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
		wth->bottom = 0;
		wth->top.i32 = 0;
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
}

//------------------------------------------------------
//void Scheduler::enqueue(Task *task) {
//	pthread_mutex_lock(&tl_lock);
//	int n = taskEnqIndex++ & (TASKQUEUE_MAX - 1);
//	taskq[n] = task;
//	pthread_cond_signal(&tl_cond); /* notify a thread in dequeue */
//	pthread_mutex_unlock(&tl_lock);
//}
//
//Task *Scheduler::dequeue() {
//	Task *task = NULL;
//	pthread_mutex_lock(&tl_lock);
//	{
//		while(taskEnqIndex == taskDeqIndex) {
//			if(dead_flag) goto L_FINAL;
//			pthread_cond_wait(&tl_cond, &tl_lock); /* wait task enqueue */
//		}
//		int n = (taskDeqIndex++) & (TASKQUEUE_MAX - 1);
//		task = taskq[n];
//	}
//L_FINAL:
//	pthread_mutex_unlock(&tl_lock);
//	return task;
//}

void enqueue(WorkerThread *wth, Task *task) {
	int n = wth->bottom;
	wth->tasks[n] = task;
	wth->bottom++;
	assert(wth->bottom < QUEUE_MAX);
}

Task *dequeueTop(WorkerThread *wth) {
	Stamp old = wth->top;

	int oldTop = old.i, newTop = oldTop + 1;
	int oldStamp = old.stamp, newStamp = oldStamp + 1;
	if(wth->bottom <= oldTop) return NULL;
	Task *task = wth->tasks[oldTop];

	Stamp newst;
	newst.i = newTop;
	newst.stamp = newStamp;
	if(CAS(wth->top.i32, old.i32, newst.i32)) {
		return task;
	}
	return NULL;
}

Task *dequeueBottom(WorkerThread *wth) {
	if(wth->bottom == 0) return NULL;
	wth->bottom--;
	Task *task = wth->tasks[wth->bottom];
	
	Stamp old = wth->top;
	int oldTop = old.i, newTop = oldTop + 1;
	int oldStamp = old.stamp, newStamp = oldStamp + 1;
	if(wth->bottom > oldTop) return task;
	Stamp newst;
	newst.i = newTop;
	newst.stamp = newStamp;
	if(wth->bottom == oldTop) {
		wth->bottom = 0;
		if(CAS(wth->top.i32, old.i32, newst.i32)) {
			return task;
		}
	}
	wth->top.i32 = newst.i32;
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

