#include "scheduler.h"

static struct job *jobs;

static int is_before(struct job *job, time_t date)
{
	return job && job->date < date;
}

void schedule(struct job *job, time_t date)
{
	struct job **pp = &jobs;

	while (is_before(*pp, date))
		pp = &(*pp)->next;

	job->date = date;
	job->next = *pp;
	*pp = job;
}

struct job *next_schedule(void)
{
	struct job *job;

	if (waiting_time() || !jobs)
		return NULL;

	job = jobs;
	jobs = jobs->next;

	return job;
}

time_t waiting_time(void)
{
	time_t now = time(NULL);

	if (!jobs || now > jobs->date)
		return 0;
	else
		return jobs->date - now;
}

int have_schedule(void)
{
	return jobs != NULL;
}
