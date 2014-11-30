#include <math.h>

#include "incremental_stats.h"

// ====================== functions ===================================
// ====================================================================


void init_stats(incr_stats* st)
{
	st->countmin=0;
	st->countmax=0;
	st->sum = 0.0;
	st->sqrsum = 0.0;
	st->n = 0;
	st->just_reset = 1;
}

void add(incr_stats* st, double item)
{
	add_k(st, item, 1);
}

void add_k(incr_stats* st, double item, int k)
{
	if (st->just_reset == 1)
	{
		st->min = item;
		st->countmin = 0;
	}
	else
	{
		if (item < st->min)
		{
			st->min = item;
			st->countmin = 0;
		}
	}
	if (item == st->min)
		st->countmin+=k;
	if (st->just_reset == 1)
	{
		st->max = item;
		st->countmax = 0;
		st->just_reset = 0;
	}
	else
	{
		if (item > st->max)
		{
			st->max = item;
			st->countmax = 0;
		}
	}
	if (item == st->max)
		st->countmax+=k;
	st->n+=k;
	if (k == 1)
	{
		st->sum += item;
		st->sqrsum += item*item;
	}
	else
	{
		st->sum += item*k;
		st->sqrsum += item*item*k;
	}
}

int get_n(incr_stats* st)
{
	return st->n;
}

double get_max(incr_stats* st)
{
	return st->max;
}

double get_min(incr_stats* st)
{
	return st->min;
}

int get_max_count(incr_stats* st)
{
	return st->countmax;
}

int get_min_count(incr_stats* st)
{
	return st->countmin;
}

double get_sum(incr_stats* st)
{
	return st->sum;
}

double get_sqr_sum(incr_stats* st)
{
	return st->sqrsum;
}

double get_avg(incr_stats* st)
{
	return st->sum / st->n;
}

double get_var(incr_stats* st)
{

	double var=(((double)st->n) / (st->n - 1)) * (st->sqrsum / st->n
			- get_avg(st)*get_avg(st));
	return (var >= 0.0 ? var : 0.0);
}

double get_std_dev(incr_stats* st)
{
	return sqrt(get_var(st));
}
