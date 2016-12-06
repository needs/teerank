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
void wait_until_next_schedule(void);
int have_schedule(void);

#endif /* SCHEDULER_H */
