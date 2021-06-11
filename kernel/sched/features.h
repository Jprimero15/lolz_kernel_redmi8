/*
 * Only give sleepers 50% of their service deficit. This allows
 * them to run sooner, but does not allow tons of sleepers to
 * rip the spread apart.
 */
#define SCHED_FEAT_GENTLE_FAIR_SLEEPERS 0

/*
 * Place new tasks ahead so that they do not starve already running
 * tasks
 */
#define SCHED_FEAT_START_DEBIT 1

/*
 * Prefer to schedule the task we woke last (assuming it failed
 * wakeup-preemption), since its likely going to consume data we
 * touched, increases cache locality.
 */
#define SCHED_FEAT_NEXT_BUDDY 1

/*
 * Prefer to schedule the task that ran last (when we did
 * wake-preempt) as that likely will touch the same data, increases
 * cache locality.
 */
#define SCHED_FEAT_LAST_BUDDY 1

/*
 * skip buddy i.e task called yield() is always skipped and the
 * next entity is selected to run irrespective of the vruntime
 */
#define SCHED_FEAT_STRICT_SKIP_BUDDY 0

/*
 * Consider buddies to be cache hot, decreases the likelyness of a
 * cache buddy being migrated away, increases cache locality.
 */
#define SCHED_FEAT_CACHE_HOT_BUDDY 1

/*
 * Allow wakeup-time preemption of the current task:
 */
#define SCHED_FEAT_WAKEUP_PREEMPTION 1

#define SCHED_FEAT_HRTICK 0
#define SCHED_FEAT_DOUBLE_TICK 0
#define SCHED_FEAT_LB_BIAS 1

/*
 * Decrement CPU capacity based on time not spent running tasks
 */
#define SCHED_FEAT_NONTASK_CAPACITY 0

/*
 * Queue remote wakeups on the target CPU and process them
 * using the scheduler IPI. Reduces rq->lock contention/bounces.
 */
#define SCHED_FEAT_TTWU_QUEUE 0

/*
 * When doing wakeups, attempt to limit superfluous scans of the LLC domain.
 */
#define SCHED_FEAT_SIS_AVG_CPU 0

#ifdef HAVE_RT_PUSH_IPI
/*
 * In order to avoid a thundering herd attack of CPUs that are
 * lowering their priorities at the same time, and there being
 * a single CPU that has an RT task that can migrate and is waiting
 * to run, where the other CPUs will try to take that CPUs
 * rq lock and possibly create a large contention, sending an
 * IPI to that CPU and let that CPU push the RT task to where
 * it should go may be a better scenario.
 */
#define SCHED_FEAT_RT_PUSH_IPI 1
#else
#define SCHED_FEAT_RT_PUSH_IPI 0
#endif

#define SCHED_FEAT_FORCE_SD_OVERLAP 0
#define SCHED_FEAT_RT_RUNTIME_SHARE 1
#define SCHED_FEAT_LB_MIN 0
#define SCHED_FEAT_ATTACH_AGE_LOAD 1

/*
 * UtilEstimation. Use estimated CPU utilization.
 */
#define SCHED_FEAT_UTIL_EST 1
#define SCHED_FEAT_UTIL_EST_FASTUP 1

/*
 * Energy aware scheduling. Use platform energy model to guide scheduling
 * decisions optimizing for energy efficiency.
 */
#ifdef CONFIG_DEFAULT_USE_ENERGY_AWARE
#define SCHED_FEAT_ENERGY_AWARE 1
#else
#define SCHED_FEAT_ENERGY_AWARE 0
#endif

/*
 * Enforce the priority of candidates selected by find_best_target()
 * ON: If the target CPU saves any energy, use that.
 * OFF: Use whichever of target or backup saves most.
 */
#define SCHED_FEAT_FBT_STRICT_ORDER 0
/*
 * Enforce schedtune.prefer_idle to take need_idle path.
 * ON: schedtune.prefer_idle is replaced with need_idle
 * OFF: schedtune.prefer_idle is honored as is.
 */
#define SCHED_FEAT_EAS_USE_NEED_IDLE 0

/*
 * If the sync flag is set but ignored, prefer to
 * select cpu in the same cluster as current. So
 * if current is a big cpu and sync is set, indicate
 * that the selection algorithm for a boosted task
 * should be used.
 */
#define SCHED_FEAT_SYNC_BOOST 1
