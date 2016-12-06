#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <time.h>

struct job;
struct job {
	time_t date;
	struct job *next;
};

void schedule(struct job *job, time_t date);
struct job *next_schedule(void);
time_t waiting_time(void);
int have_schedule(void);

#endif /* SCHEDULER_H */
