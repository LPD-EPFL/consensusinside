typedef struct
{
	double min;
	double max;
	double sum;
	double sqrsum;
	int n;
	int countmin;
	int countmax;
	int just_reset;
} incr_stats;

void init_stats(incr_stats* st);

// Updates the statistics according to this element. It calls
// add(item,1)
void add(incr_stats* st, double item);

// Updates the statistics assuming element item is added
// k times
void add_k(incr_stats* st, double item, int k);

// The number of data items processed so far 
int get_n(incr_stats* st);

// The maximum of the data items
double get_max(incr_stats* st);

// The minimum of the data items 
double get_min(incr_stats* st);

// Returns the number of data items whose value equals the maximum
int get_max_count(incr_stats* st);

// Returns the number of data items whose value equals the minimum
int get_min_count(incr_stats* st);

// The sum of the data items 
double get_sum(incr_stats* st);

// The sum of the squares of the data items 
double get_sqr_sum(incr_stats* st);

// The average of the data items
double get_avg(incr_stats* st);

// The empirical variance of the data items. Guaranteed to be larger or
// equal to 0.0. If due to rounding errors the value becomes negative,
// it returns 0.0
double get_var(incr_stats* st);

// The empirical standard deviation of the data items 
double get_std_dev(incr_stats* st);
