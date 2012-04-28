#ifndef SCHEDULER_H
#define SCHEDULER_H

//------------------------------------------------------
// task

typedef void (*TaskMethod)(Task *task, WorkerThread *wth);

enum TaskStat {
	TASK_RUN,
	TASK_END,
};

struct Task {
	volatile TaskStat stat;
	Task *next;
	Code  *pc;
	Value *sp;
	TaskMethod dest;
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
	int taskEnqIndex;
	int taskDeqIndex;
	int queuemask;
	pthread_mutex_t tl_lock;
	pthread_cond_t  tl_cond;
	Task *taskpool;
	Task *freelist;
	volatile bool dead_flag;
	WorkerThread *wthpool;

public:
	Scheduler(Context *ctx);
	~Scheduler();
	void initWorkers();
	void enqueue(Task *task);
	Task *dequeue();
	Task *newTask(Func *func, Value *args, TaskMethod dest);
	void deleteTask(Task *task);
	bool isTaskEmpty() { return freelist == NULL; }
	Context *getCtx() { return ctx; }
};

#endif

