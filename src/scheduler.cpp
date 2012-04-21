#include "lisp.h"

static void *WorkerThread_main(void *arg) {
	WorkerThread *wth = (WorkerThread *)arg;
	Scheduler *sche = wth->sche;
	int myid = wth->id;
	Task *task;

	while(!sche->dead_flag) {
		//task = wth->joinwait ? NULL : wth->queue.popBottom();
		if(myid == 0 && (task = sche->firsttask) != NULL) {
			sche->firsttask = NULL;
			vmrun(sche->ctx, wth, task);
			continue;
		}
		task = wth->queue->popBottom();
		//wth->joinwait = false;
		if(task != NULL) {
			vmrun(sche->ctx, wth, task);
		} else {
			// steal task
			for(int i=1; i<WORKER_MAX+1; i++) {
				int id = (myid + i) % WORKER_MAX;
				WorkerThread *w = &sche->wthpool[id];
				if(!w->queue->isEmpty()) {
					if((task = w->queue->popTop()) != NULL) {
						//printf("steal id=%d from %d task=%d\n", wth->id, w->id, task->sp[0]);
						vmrun(sche->ctx, wth, task);
						break;
					}
				}
			}
		}
	}
	return NULL;
}

//------------------------------------------------------
Scheduler::Scheduler(Context *ctx) {
	this->ctx = ctx;
	this->dead_flag = false;
	this->firsttask = NULL;
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
		wth->joinwait = false;
		wth->queue = new UnboundedDEQueue();
	}
	for(int i=0; i<WORKER_MAX; i++) {
		WorkerThread *wth = &wthpool[i];
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

