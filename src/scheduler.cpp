#include "lisp.h"

static void *WorkerThread_main(void *arg) {
	WorkerThread *wth = (WorkerThread *)arg;
	Scheduler *sche = wth->sche;
	while(true) {
		Task *task = sche->dequeue();
		assert(task != NULL && task->stat == TASK_RUN);
		vmrun(sche->ctx, wth, task);
	}
}

//------------------------------------------------------
Scheduler::Scheduler(Context *ctx) {
	this->ctx = ctx;
	dummyTask.next = NULL;
	tl_head = tl_tail = &dummyTask;
	pthread_mutex_init(&tl_lock, NULL);
	pthread_cond_init(&tl_cond, NULL);

	// init task list
	taskpool = new Task[TASK_MAX];
	freelist = &taskpool[0];
	for(int i=0; i<TASK_MAX; i++) {
		taskpool[i].next = &taskpool[i+1];
	}
	taskpool[TASK_MAX + 1].next = NULL;

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

//------------------------------------------------------
void Scheduler::enqueue(Task *task) {
	task->next = NULL;
	pthread_mutex_lock(&tl_lock);
	tl_tail->next = task;
	tl_tail = task;
	pthread_cond_signal(&tl_cond); /* notify a thread in dequeue */
	pthread_mutex_unlock(&tl_lock);
}

Task *Scheduler::dequeue() {
	pthread_mutex_lock(&tl_lock);
	while(tl_head->next == NULL) {
		pthread_cond_wait(&tl_cond, &tl_lock); /* wait task enqueue */
	}
	Task *task = tl_head->next;
	tl_head->next = task->next;
	if(task->next == NULL) tl_tail = tl_head;
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

