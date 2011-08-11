/*
* drivers/cpufreq/cpufreq_smartass.c
*
* Copyright (C) 2010 Google, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* Author: Erasmux
*
* Based on the interactive governor By Mike Chan (mike@android.com)
* which was adaptated to 2.6.29 kernel by Nadlabak (pavel@doshaska.net)
*
* requires to add
* EXPORT_SYMBOL_GPL(nr_running);
* at the end of kernel/sched.c
*
*/

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include <asm/cputime.h>
#include <linux/earlysuspend.h>

static void (*pm_idle_old)(void);
static atomic_t active_count = ATOMIC_INIT(0);

struct smartass_info_s {
struct cpufreq_policy *cur_policy;
struct timer_list timer;
u64 time_in_idle;
u64 idle_exit_time;
unsigned int force_ramp_up;
unsigned int enable;
};
static DEFINE_PER_CPU(struct smartass_info_s, smartass_info);

/* Workqueues handle frequency scaling */
static struct workqueue_struct *up_wq;
static struct workqueue_struct *down_wq;
static struct work_struct freq_scale_work;

static u64 freq_change_time;
static u64 freq_change_time_in_idle;

static cpumask_t work_cpumask;
static unsigned int suspended;


/*
* The minimum amount of time to spend at a frequency before we can ramp down,
* default is 45ms.
*/
#define DEFAULT_RAMP_DOWN_RATE_NS 70000;
static unsigned long ramp_down_rate_ns;

/*
* When ramping up frequency jump to at least this frequency.
*/

#define DEFAULT_UP_MIN_FREQ (800*1000)
static unsigned int up_min_freq;

/*
* When sleep_max_freq>0 the frequency when suspended will be capped
* by this frequency. Also will wake up at max frequency of policy
* to minimize wakeup issues.
* Set sleep_max_freq=0 to disable this behavior.
*/
#define DEFAULT_SLEEP_MAX_FREQ (400*1000)
static unsigned int sleep_max_freq;

/*
* Sampling rate, I highly recommend to leave it at 2.
*/
#define DEFAULT_SAMPLE_RATE_JIFFIES 2
static unsigned int sample_rate_jiffies;

/*
* Max freqeuncy delta when ramping up.
*/

#define DEFAULT_MAX_RAMP_UP (400 * 1000)
static unsigned int max_ramp_up;

/*
* CPU freq will be increased if measured load > max_cpu_load;
*/
#define DEFAULT_MAX_CPU_LOAD 50
static unsigned long max_cpu_load;

/*
* CPU freq will be decreased if measured load < min_cpu_load;
*/
#define DEFAULT_MIN_CPU_LOAD 35
static unsigned long min_cpu_load;

//Leave this zero by default, people can tweak it if they so wish.
#define DEFAULT_RAMP_UP_RATE_NS 0
static unsigned long ramp_up_rate_ns;


static int cpufreq_governor_smartass(struct cpufreq_policy *policy,
unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTASS
static
#endif
struct cpufreq_governor cpufreq_gov_smartass = {
.name = "smartass",
.governor = cpufreq_governor_smartass,
.max_transition_latency = 9000000,
.owner = THIS_MODULE,
};

static void cpufreq_smartass_timer(unsigned long data)
{
u64 delta_idle;
u64 update_time;
u64 now_idle;
struct smartass_info_s *this_smartass = &per_cpu(smartass_info, data);
struct cpufreq_policy *policy = this_smartass->cur_policy;

now_idle = get_cpu_idle_time_us(data, &update_time);

if (update_time == this_smartass->idle_exit_time)
return;

delta_idle = cputime64_sub(now_idle, this_smartass->time_in_idle);
//printk(KERN_INFO "smartass: t=%llu i=%llu\n",cputime64_sub(update_time,this_smartass->idle_exit_time),delta_idle);

/* Scale up if there were no idle cycles since coming out of idle */
if (delta_idle == 0 && cputime64_sub(update_time, freq_change_time) > ramp_up_rate_ns) {
if (policy->cur == policy->max)
return;

if (nr_running() < 1)
return;

this_smartass->force_ramp_up = 1;
cpumask_set_cpu(data, &work_cpumask);
queue_work(up_wq, &freq_scale_work);
return;
}

/*
* There is a window where if the cpu utlization can go from low to high
* between the timer expiring, delta_idle will be > 0 and the cpu will
* be 100% busy, preventing idle from running, and this timer from
* firing. So setup another timer to fire to check cpu utlization.
* Do not setup the timer if there is no scheduled work.
*/
if (!timer_pending(&this_smartass->timer) && nr_running() > 0) {
this_smartass->time_in_idle = get_cpu_idle_time_us(
data, &this_smartass->idle_exit_time);
mod_timer(&this_smartass->timer, jiffies + sample_rate_jiffies);
}

if (policy->cur == policy->min)
return;

/*
* Do not scale down unless we have been at this frequency for the
* minimum sample time.
*/
if (cputime64_sub(update_time, freq_change_time) < ramp_down_rate_ns)
return;


cpumask_set_cpu(data, &work_cpumask);
queue_work(down_wq, &freq_scale_work);
}

static void cpufreq_idle(void)
{
struct smartass_info_s *this_smartass = &per_cpu(smartass_info, smp_processor_id());
struct cpufreq_policy *policy = this_smartass->cur_policy;

pm_idle_old();

if (!cpumask_test_cpu(smp_processor_id(), policy->cpus))
return;

/* Timer to fire in 1-2 ticks, jiffie aligned. */
if (timer_pending(&this_smartass->timer) == 0) {
this_smartass->time_in_idle = get_cpu_idle_time_us(
smp_processor_id(), &this_smartass->idle_exit_time);
mod_timer(&this_smartass->timer, jiffies + sample_rate_jiffies);
}
}

/*
* Choose the cpu frequency based off the load. For now choose the minimum
* frequency that will satisfy the load, which is no
t always the lower power.
*/
static unsigned int cpufreq_smartass_calc_freq(unsigned int cpu, struct cpufreq_policy *policy)
{
unsigned int delta_time;
unsigned int idle_time;
unsigned int cpu_load;
unsigned int new_freq;
u64 current_wall_time;
u64 current_idle_time;


current_idle_time = get_cpu_idle_time_us(cpu, &current_wall_time);

idle_time = (unsigned int)( current_idle_time - freq_change_time_in_idle );
delta_time = (unsigned int)( current_wall_time - freq_change_time );

cpu_load = 100 * (delta_time - idle_time) / delta_time;
if (cpu_load < min_cpu_load) {
//if the current frequency is below 1.2ghz, everything is 200mhz steps
if(policy->cur <= 1200000 && policy->cur >= 400000) {
/* catch the extra 200mhz gap between 400 and 800 when scaling down -netarchy */
if(policy->cur == 800000) {
new_freq = policy->cur - 400000;
return new_freq;
}
else {
new_freq = policy->cur - 200000;
return new_freq;
}
}
//above 1.2ghz though, everything is 200mhz steps
else {
new_freq = policy->cur - 200000;
return new_freq;
}
}
if (cpu_load > max_cpu_load) {
if(policy->cur < 1200000 && policy->cur > 200000) {
/* catch the gap between 400 and 800 when scaling up -netarchy */
if(policy->cur == 400000) {
new_freq = policy->cur + 400000;
return new_freq;
}
else {
new_freq = policy->cur + 200000;
return new_freq;
}
}
else {
new_freq = policy->cur + 100000;
return new_freq;
}
}
return policy->cur;
}

/* We use the same work function to scale up and down */
static void cpufreq_smartass_freq_change_time_work(struct work_struct *work)
{
unsigned int cpu;
unsigned int new_freq;
struct smartass_info_s *this_smartass;
struct cpufreq_policy *policy;
cpumask_t tmp_mask = work_cpumask;
for_each_cpu(cpu, tmp_mask) {
this_smartass = &per_cpu(smartass_info, cpu);
policy = this_smartass->cur_policy;

if (this_smartass->force_ramp_up) {
this_smartass->force_ramp_up = 0;

if (nr_running() == 1) {
cpumask_clear_cpu(cpu, &work_cpumask);
return;
}

if (policy->cur == policy->max)
return;

new_freq = policy->cur + max_ramp_up;

if (suspended && sleep_max_freq) {
if (new_freq > sleep_max_freq)
new_freq = sleep_max_freq;
} else {
if (new_freq < up_min_freq)
new_freq = up_min_freq;
}

} else {
new_freq = cpufreq_smartass_calc_freq(cpu,policy);

// in suspend limit to sleep_max_freq and
// jump straight to sleep_max_freq to avoid wakeup problems
if (suspended && sleep_max_freq &&
(new_freq > sleep_max_freq || new_freq > policy->cur))
new_freq = sleep_max_freq;
}

if (new_freq > policy->max)
new_freq = policy->max;

if (new_freq < policy->min)
new_freq = policy->min;

__cpufreq_driver_target(policy, new_freq,
CPUFREQ_RELATION_L);

freq_change_time_in_idle = get_cpu_idle_time_us(cpu,
&freq_change_time);

cpumask_clear_cpu(cpu, &work_cpumask);

}


}

static ssize_t show_ramp_up_rate_ns(struct cpufreq_policy *policy, char *buf) {
return sprintf(buf, "%lu\n", ramp_up_rate_ns);
}

static ssize_t store_ramp_up_rate_ns(struct cpufreq_policy *policy, const char *buf, size_t count) {
ssize_t ret;
unsigned long input;
ret = strict_strtoul(buf, 0, &input);
if (ret >= 0 && input >= 0 && input <= 100000000)
ramp_up_rate_ns = input;
return ret;
}

static struct freq_attr ramp_up_rate_ns_attr = __ATTR(ramp_up_rate_ns, 0644,
show_ramp_up_rate_ns, store_ramp_up_rate_ns);

static ssize_t show_ramp_down_rate_ns(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%lu\n", ramp_down_rate_ns);
}

static ssize_t store_ramp_down_rate_ns(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input >= 1000 && input <= 100000000)
ramp_down_rate_ns = input;
return res;
}

static struct freq_attr ramp_down_rate_ns_attr = __ATTR(ramp_down_rate_ns, 0644,
show_ramp_down_rate_ns, store_ramp_down_rate_ns);

static ssize_t show_up_min_freq(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%u\n", up_min_freq);
}

static ssize_t store_up_min_freq(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input >= 0)
up_min_freq = input;
return res;
}

static struct freq_attr up_min_freq_attr = __ATTR(up_min_freq, 0644,
show_up_min_freq, store_up_min_freq);

static ssize_t show_sleep_max_freq(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%u\n", sleep_max_freq);
}

static ssize_t store_sleep_max_freq(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input >= 0)
sleep_max_freq = input;
return res;
}

static struct freq_attr sleep_max_freq_attr = __ATTR(sleep_max_freq, 0644,
show_sleep_max_freq, store_sleep_max_freq);

static ssize_t show_sample_rate_jiffies(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%u\n", sample_rate_jiffies);
}

static ssize_t store_sample_rate_jiffies(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input > 0 && input <= 1000)
sample_rate_jiffies = input;
return res;
}

static struct freq_attr sample_rate_jiffies_attr = __ATTR(sample_rate_jiffies, 0644,
show_sample_rate_jiffies, store_sample_rate_jiffies);

static ssize_t show_max_ramp_up(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%u\n", max_ramp_up);
}

static ssize_t store_max_ramp_up(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input > 10000)
max_ramp_up = input;
return res;
}

static struct freq_attr max_ramp_up_attr = __ATTR(max_ramp_up, 0644,
show_max_ramp_up, store_max_ramp_up);

static ssize_t show_max_cpu_load(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%lu\n", max_cpu_load);
}

static ssize_t store_max_cpu_load(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input > 0 && input <= 100)
max_cpu_load = input;
return res;
}

static struct freq_attr max_cpu_load_attr = __ATTR(max_cpu_load, 0644,
show_max_cpu_load, store_max_cpu_load);

static ssize_t show_min_cpu_load(struct cpufreq_policy *policy, char *buf)
{
return sprintf(buf, "%lu\n", min_cpu_load);
}

static ssize_t store_min_cpu_load(struct cpufreq_policy *policy, const char *buf, size_t count)
{
        ssize_t res;
unsigned long input;
res = strict_strtoul(buf, 0, &input);
if (res >= 0 && input > 0 && input < 100)
min_cpu_load = input;
return res;
}

static struct freq_attr min_cpu_load_attr = __ATTR(min_cpu_load, 0644,
show_min_cpu_load, store_min_cpu_load);

static struct attribute * smartass_attributes[] = {
&ramp_down_rate_ns_attr.attr,
&up_min_freq_attr.attr,
&sleep_max_freq_attr.attr,
&sample_rate_jiffies_attr.attr,
&max_ramp_up_attr.attr,
&max_cpu_load_attr.attr,
&min_cpu_load_attr.attr,
&ramp_up_rate_ns_attr.attr,
NULL,
};

static struct attribute_group smartass_attr_group = {
.attrs = smartass_attributes,
.name = "smartass",
};

static int cpufreq_governor_smartass(struct cpufreq_policy *new_policy,
unsigned int event)
{
unsigned int cpu = new_policy->cpu;
int rc;
struct smartass_info_s *this_smartass = &per_cpu(smartass_info, cpu);

switch (event) {
case CPUFREQ_GOV_START:
if ((!cpu_online(cpu)) || (!new_policy->cur))
return -EINVAL;

if (this_smartass->enable) /* Already enabled */
break;

/*
* Do not register the idle hook and create sysfs
* entries if we have already done so.
*/
if (atomic_inc_return(&active_count) > 1)
return 0;

rc = sysfs_create_group(&new_policy->kobj, &smartass_attr_group);
if (rc)
return rc;
pm_idle_old = pm_idle;
pm_idle = cpufreq_idle;

this_smartass->cur_policy = new_policy;
this_smartass->enable = 1;

// notice no break here!

case CPUFREQ_GOV_LIMITS:
if (this_smartass->cur_policy->cur != new_policy->max)
__cpufreq_driver_target(new_policy, new_policy->max, CPUFREQ_RELATION_H);

break;

case CPUFREQ_GOV_STOP:
this_smartass->enable = 0;

if (atomic_dec_return(&active_count) > 1)
return 0;
sysfs_remove_group(&new_policy->kobj,
&smartass_attr_group);

pm_idle = pm_idle_old;
del_timer(&this_smartass->timer);
break;
}

return 0;
}

static void smartass_suspend(int cpu, int suspend)
{
struct smartass_info_s *this_smartass = &per_cpu(smartass_info, smp_processor_id());
struct cpufreq_policy *policy = this_smartass->cur_policy;
unsigned int new_freq;

if (!this_smartass->enable || sleep_max_freq==0) // disable behavior for sleep_max_freq==0
return;

if (suspend) {
if (policy->cur > sleep_max_freq) {
new_freq = sleep_max_freq;
if (new_freq > policy->max)
new_freq = policy->max;
if (new_freq < policy->min)
new_freq = policy->min;
__cpufreq_driver_target(policy, new_freq,
CPUFREQ_RELATION_H);
}
} else { // resume at max speed:
__cpufreq_driver_target(policy, policy->max,
CPUFREQ_RELATION_H);
}

}

static void smartass_early_suspend(struct early_suspend *handler) {
int i;
suspended = 1;
for_each_online_cpu(i)
smartass_suspend(i,1);
}

static void smartass_late_resume(struct early_suspend *handler) {
int i;
suspended = 0;
for_each_online_cpu(i)
smartass_suspend(i,0);
}

static struct early_suspend smartass_power_suspend = {
.suspend = smartass_early_suspend,
.resume = smartass_late_resume,
};

static int __init cpufreq_smartass_init(void)
{
unsigned int i;
struct smartass_info_s *this_smartass;
ramp_down_rate_ns = DEFAULT_RAMP_DOWN_RATE_NS;
up_min_freq = DEFAULT_UP_MIN_FREQ;
sleep_max_freq = DEFAULT_SLEEP_MAX_FREQ;
sample_rate_jiffies = DEFAULT_SAMPLE_RATE_JIFFIES;
max_ramp_up = DEFAULT_MAX_RAMP_UP;
max_cpu_load = DEFAULT_MAX_CPU_LOAD;
min_cpu_load = DEFAULT_MIN_CPU_LOAD;
ramp_up_rate_ns = DEFAULT_RAMP_UP_RATE_NS;

suspended = 0;

/* Initalize per-cpu data: */
for_each_possible_cpu(i) {
this_smartass = &per_cpu(smartass_info, i);
this_smartass->enable = 0;
this_smartass->force_ramp_up = 0;
this_smartass->time_in_idle = 0;
this_smartass->idle_exit_time = 0;
// intialize timer:
init_timer_deferrable(&this_smartass->timer);
this_smartass->timer.function = cpufreq_smartass_timer;
this_smartass->timer.data = i;
}

/* Scale up is high priority */
up_wq = create_rt_workqueue("ksmartass_up");
down_wq = create_workqueue("ksmartass_down");

INIT_WORK(&freq_scale_work, cpufreq_smartass_freq_change_time_work);

register_early_suspend(&smartass_power_suspend);

return cpufreq_register_governor(&cpufreq_gov_smartass);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTASS
pure_initcall(cpufreq_smartass_init);
#else
module_init(cpufreq_smartass_init);
#endif

static void __exit cpufreq_smartass_exit(void)
{
cpufreq_unregister_governor(&cpufreq_gov_smartass);
destroy_workqueue(up_wq);
destroy_workqueue(down_wq);
}

module_exit(cpufreq_smartass_exit);

MODULE_AUTHOR ("Erasmux");
MODULE_DESCRIPTION ("'cpufreq_minmax' - A smart cpufreq governor optimized for the hero!");
MODULE_LICENSE ("GPL");
