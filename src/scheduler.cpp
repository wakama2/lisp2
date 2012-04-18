#include "lisp.h"

static void *WorkerThread_main(void *arg) {
	WorkerThread *wth = (WorkerThread *)arg;
	Scheduler *sche = wth->sche;
	int myid = wth->id;
	Task *task;

	while(!sche->dead_flag) {
		task = wth->joinwait ? NULL : dequeueBottom(wth);
		wth->joinwait = false;
		if(task != NULL) {
			vmrun(sche->ctx, wth, task);
		} else {
			// steal task
			for(int i=1; i<WORKER_MAX; i++) {
				int id = (myid + i) % WORKER_MAX;
				WorkerThread *w = &sche->wthpool[id];
				if(!isEmpty(w)) {
					if((task = dequeueTop(w)) != NULL) {
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
		wth->top.i = 0;
		wth->top.stamp = 0;
		wth->joinwait = false;
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

bool isEmpty(WorkerThread *wth) {
	int top = wth->top.i;
	int btm = wth->bottom;
	return btm <= top;
}

void enqueue(WorkerThread *wth, Task *task) {
	assert(wth->bottom < QUEUE_MAX);
	//printf("enqueue id=%d task=%d\n", wth->id, task->sp[0]);
	wth->tasks[wth->bottom] = task;
	wth->bottom++;
}

Task *dequeueTop(WorkerThread *wth) {
	Stamp oldStamp = wth->top;
	Stamp newStamp;
	newStamp.i = oldStamp.i + 1; /* top index */
	newStamp.stamp = oldStamp.stamp + 1;
	if(wth->bottom <= oldStamp.i) return NULL;

	Task *task = wth->tasks[oldStamp.i];
	if(CAS(wth->top.i32, oldStamp.i32, newStamp.i32)) {
		return task;
	}
	return NULL;
}

Task *dequeueBottom(WorkerThread *wth) {
	if(wth->bottom == 0) return NULL;
	wth->bottom--;
	Task *task = wth->tasks[wth->bottom];
	
	Stamp oldStamp = wth->top;
	Stamp newStamp;
	newStamp.i = 0; /* top index */
	newStamp.stamp = oldStamp.stamp + 1;

	if(wth->bottom > oldStamp.i) return task;
	if(wth->bottom == oldStamp.i) {
		wth->bottom = 0;
		if(CAS(wth->top.i32, oldStamp.i32, newStamp.i32)) {
			return task;
		}
	}
	wth->top = newStamp;
	return NULL;
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

