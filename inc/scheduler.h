#ifndef SCHEDULER_H
#define SCHEDULER_H

//------------------------------------------------------
// task

enum TaskStat {
	TASK_RUN,
	TASK_END,
};

struct Task {
	volatile TaskStat stat;
	Task *next;
	Code  *pc;
	Value *sp;
	Value stack[TASK_STACKSIZE];
};

//------------------------------------------------------
// worker thread

struct WorkerThread {
	Context *ctx;
	Scheduler *sche;
	int id;
	pthread_t pth;
};

void vmrun(Context *ctx, WorkerThread *wth, Task *task);

//------------------------------------------------------
// scheduler

class Scheduler {
private:
	Context *ctx;
	Task **taskq;
	volatile int taskEnqIndex;
	volatile int taskDeqIndex;
	volatile int waitCount;
	int queuemask;
	pthread_mutex_t tl_lock;
	pthread_cond_t  tl_cond;
	pthread_cond_t  tl_maincond;
	Task *taskpool;
	Task *freelist;
	volatile bool dead_flag;
	WorkerThread *wthpool;
	Code endcode;

public:
	Scheduler(Context *ctx);
	~Scheduler();
	void initWorkers();
	void enqueue(Task *task);
	Task *dequeue();
	void enqueueWaitFor(Task *task);
	Task *newTask(Func *func, Value *args);
	void deleteTask(Task *task);
	bool isTaskEmpty() { return freelist == NULL; }
	Context *getCtx() { return ctx; }
};

#endif

