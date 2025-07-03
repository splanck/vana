#ifndef IDLE_H
#define IDLE_H

struct task;
struct process;

int idle_task_init();
struct task* idle_task_get();
struct process* idle_process_get();

#endif
