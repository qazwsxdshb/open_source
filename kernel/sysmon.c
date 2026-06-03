#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel_stat.h>
#include <linux/mm.h>
#include <linux/sysinfo.h>
#include <linux/sched/signal.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

static struct proc_dir_entry *proc_dir;

static unsigned long long prev_idle = 0;
static unsigned long long prev_total = 0;
static int cpu_usage = 0;
static int cpu_usage;

static unsigned long total_mem_mb;
static unsigned long free_mem_mb;
static unsigned long used_mem_mb;

static int process_count;
static spinlock_t sysmon_lock;
static struct delayed_work sysmon_work;

static void update_cpu_usage(void)
{
    u64 user, nice, system, idle;
    u64 iowait, irq, softirq, steal;

    u64 total;
    u64 total_diff;
    u64 idle_diff;

    user    = kcpustat_cpu(0).cpustat[CPUTIME_USER];
    nice    = kcpustat_cpu(0).cpustat[CPUTIME_NICE];
    system  = kcpustat_cpu(0).cpustat[CPUTIME_SYSTEM];
    idle    = kcpustat_cpu(0).cpustat[CPUTIME_IDLE];

    iowait  = kcpustat_cpu(0).cpustat[CPUTIME_IOWAIT];
    irq     = kcpustat_cpu(0).cpustat[CPUTIME_IRQ];
    softirq = kcpustat_cpu(0).cpustat[CPUTIME_SOFTIRQ];
    steal   = kcpustat_cpu(0).cpustat[CPUTIME_STEAL];

    total =
        user + nice + system + idle +
        iowait + irq + softirq + steal;

    total_diff = total - prev_total;
    idle_diff = idle - prev_idle;

    if (total_diff > 0)
        cpu_usage =
            (100 * (total_diff - idle_diff))
            / total_diff;

    prev_total = total;
    prev_idle = idle;
}

static void update_mem_info(void)
{
    struct sysinfo info;

    si_meminfo(&info);

    spin_lock(&sysmon_lock);

    total_mem_mb =
        (info.totalram * info.mem_unit)
        / (1024 * 1024);

    free_mem_mb =
        (info.freeram * info.mem_unit)
        / (1024 * 1024);

    used_mem_mb =
        total_mem_mb - free_mem_mb;

    spin_unlock(&sysmon_lock);
}

static void update_process_info(void)
{
    struct task_struct *task;
    int count = 0;

    for_each_process(task)
        count++;

    spin_lock(&sysmon_lock);

    process_count = count;

    spin_unlock(&sysmon_lock);
}

static void sysmon_work_func(
    struct work_struct *work)
{
    update_cpu_usage();

    update_mem_info();

    update_process_info();

    schedule_delayed_work(
        &sysmon_work,
        msecs_to_jiffies(1000));
}

static int cpu_show(
    struct seq_file *m,
    void *v)
{
    spin_lock(&sysmon_lock);

    seq_printf(m,
        "CPU Usage: %d%%\n",
        cpu_usage);

    spin_unlock(&sysmon_lock);

    return 0;
}
static int mem_show(
    struct seq_file *m,
    void *v)
{
    spin_lock(&sysmon_lock);

    seq_printf(m,
        "Total Memory : %lu MB\n"
        "Free Memory  : %lu MB\n"
        "Used Memory  : %lu MB\n",
        total_mem_mb,
        free_mem_mb,
        used_mem_mb);

    spin_unlock(&sysmon_lock);

    return 0;
}

static int procs_show(
    struct seq_file *m,
    void *v)
{
    spin_lock(&sysmon_lock);

    seq_printf(m,
        "Process Count : %d\n",
        process_count);

    spin_unlock(&sysmon_lock);

    return 0;
}

static int cpu_open(struct inode *inode, struct file *file)
{
    return single_open(file, cpu_show, NULL);
}

static int mem_open(struct inode *inode, struct file *file)
{
    return single_open(file, mem_show, NULL);
}

static int procs_open(struct inode *inode,
                      struct file *file)
{
    return single_open(file,
                       procs_show,
                       NULL);
}

static const struct file_operations cpu_fops = {
    .owner = THIS_MODULE,
    .open = cpu_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct file_operations mem_fops = {
    .owner   = THIS_MODULE,
    .open    = mem_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static const struct file_operations procs_fops = {
    .owner   = THIS_MODULE,
    .open    = procs_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int __init sysmon_init(void)
{
    spin_lock_init(&sysmon_lock);
    printk(KERN_INFO "SysMon Loaded\n");
    proc_dir = proc_mkdir("sysmon", NULL);
    proc_create("cpu", 0444, proc_dir, &cpu_fops);
    proc_create("mem", 0444, proc_dir, &mem_fops);
    proc_create("procs", 0444, proc_dir, &procs_fops);
    INIT_DELAYED_WORK(
        &sysmon_work,
        sysmon_work_func);

    schedule_delayed_work(
        &sysmon_work,
        msecs_to_jiffies(1000));
    return 0;
}


static void __exit sysmon_exit(void)
{
    printk(KERN_INFO "SysMon Removed\n");
    remove_proc_entry("cpu", proc_dir);
    remove_proc_entry("mem", proc_dir);	
    remove_proc_entry("procs", proc_dir);
    cancel_delayed_work_sync(
        &sysmon_work);

    remove_proc_entry("sysmon", NULL);
	
}



module_init(sysmon_init);
module_exit(sysmon_exit);

MODULE_LICENSE("GPL");
