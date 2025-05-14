#ifndef SCHED_STATS_H
#define SCHED_STATS_H

struct SchedStats {
	int number_of_yields;
	int number_of_switches;
	int number_of_boosts;
};

#endif  // SCHED_STATS_H