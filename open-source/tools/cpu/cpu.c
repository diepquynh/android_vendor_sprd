/*
 *
 * This program gives a cpu usage recording with low effort.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>

/* Macros */
#define PROC_STAT "/proc/stat"
#define MAX_INTERVAL 60
#define MAX_NAME_SIZE 64
#define MAX_LINE_SIZE 1024

/* Global variable definitions */ 
struct tick_entry {
	pid_t pid, tid;
	unsigned long utime, stime;
	unsigned long old_utime, old_stime;
	unsigned long total;
	struct tick_entry *next;
	struct tick_entry *thread_next;
	char stat_path[MAX_NAME_SIZE];
	char thread_name[MAX_NAME_SIZE];
};

static struct tick_entry tick_entry_head;
static int verbose = 0;
static unsigned long pid_tick_total = 0;
static unsigned long cpu_tick_total = 0;

/* Helper functions */
static void get_cmdline(char *path, char *name, int len)
{
	FILE *fp;
	
	fp = fopen(path, "r");
	if (fp == NULL) {
		strcpy(name, "N/A");
		return;
	}
	if (fgets(name, len, fp) == NULL) {
		strcpy(name, "N/A");
		fclose(fp);
		return;
	}
	if(name[0] == 0) {
		strcpy(name, "N/A");
		fclose(fp);
		return;
	}
	fclose(fp);
	if(verbose)
		printf("quit get_cmdline()\n");
	return;
}

int path_exist(const char *path)
{
	struct stat st;
	return !stat(path, &st);
}

static int get_cpu_tick(unsigned long *value)
{
	char name[256], buffer[512];

	FILE *stat_fp;

	stat_fp = fopen(PROC_STAT, "r");
	if (stat_fp == NULL) {
		printf("open %s failed!\n", PROC_STAT);
		return -1;
	}
	if (fgets(buffer, sizeof(buffer), stat_fp) == NULL) {
		printf("read from %s failed!\n", PROC_STAT);
		fclose(stat_fp);
		return -1;
	}
	sscanf(buffer, "%s %lu %lu %lu %lu %lu %lu %lu",
		name, &value[0], &value[1], &value[2],
		&value[3], &value[4], &value[5], &value[6]);
	
	fclose(stat_fp);
	return (value[0] + value[1] + value[2] + value[3] +
				value[4] + value[5] + value[6]);
}

static void gen_timestamp(char *timestamp)
{
	time_t ltime;
	struct tm *tm_pt;
	struct timeval my_time;

	ltime=time(NULL);
	tm_pt=localtime(&ltime);
	gettimeofday(&my_time, NULL);

	sprintf(timestamp, "%02d:%02d:%02d",
	tm_pt->tm_hour,
	tm_pt->tm_min,
	tm_pt->tm_sec);
	return;
}

static int is_digit(char *input)
{
	char *pt = input;
	if(pt == NULL)
		return 0;
	while(*pt != 0) {
		if(*pt < '0' || *pt > '9')
			return 0;
		pt++;
	}
	return 1;
}

static struct tick_entry *insert_entry(pid_t pid, pid_t tid, struct tick_entry *head)
{
	struct tick_entry *entry;
	char buffer[MAX_NAME_SIZE];

	entry = (struct tick_entry *)malloc(sizeof(struct tick_entry));
	if(entry == NULL) {
		perror("malloc() failed!");
		exit(1);
	}
	memset(entry, 0, sizeof(struct tick_entry));
	entry->pid = pid;
	entry->tid = tid;
	if(tid == 0) {
		sprintf(entry->stat_path, "/proc/%d/stat", pid);
		sprintf(buffer, "/proc/%d/cmdline", pid);
		get_cmdline(buffer, entry->thread_name, MAX_NAME_SIZE);
		entry->next = head->next;
		head->next = entry;
	} else {
		sprintf(entry->stat_path, "/proc/%d/task/%d/stat", pid, tid);
		sprintf(buffer, "/proc/%d/task/%d/cmdline", pid, tid);
		get_cmdline(buffer, entry->thread_name, MAX_NAME_SIZE);
		entry->thread_next = head->thread_next;
		head->thread_next = entry;
	}
	return entry;
}

static unsigned long get_user_stat(struct tick_entry *entry, int first_time)
{
	FILE *fp;
	int i, utime, stime;
	char *pt, buffer[MAX_LINE_SIZE];

	if(!path_exist(entry->stat_path))
		return entry->total;

	if((fp = fopen(entry->stat_path, "r")) == NULL)
		return entry->total;

	if (fgets(buffer, sizeof(buffer), fp) == NULL) {
		fclose(fp);
		return entry->total;
	}

	for(i = 0, pt = buffer; i < 13; i++, pt++)
		pt = strchr(pt, ' ');

	sscanf(pt, "%d %d", &utime, &stime);
	if(!first_time) {
		entry->utime = utime - entry->old_utime;
		entry->stime = stime - entry->old_stime;
	}
	entry->old_utime = utime;
	entry->old_stime = stime;

	entry->total += (entry->utime + entry->stime);
	fclose(fp);
	return entry->total;
}

static void get_pid_tick(int first_time)
{
	struct tick_entry *entry = tick_entry_head.next;
	struct tick_entry *thread_entry;
	
	pid_tick_total = 0;

	while(entry) {
		pid_tick_total += get_user_stat(entry, first_time);
		thread_entry = entry->thread_next;
		while(thread_entry) {
			get_user_stat(thread_entry, first_time);
			thread_entry = thread_entry->thread_next;
		}
		entry = entry->next;
	}
	return;
}

static int parse_pid_list(const char *list, int name)
{
	pid_t pid = 0, tid;
	int size, num = 0;
	char *start, *tmp, buffer[MAX_NAME_SIZE], word[MAX_NAME_SIZE];
	struct tick_entry *entry;
	FILE *fp;
	DIR *dir;
	struct dirent *dir_entry;

	start = (char *)list;

	while(1) {

		if(!name) {
			pid = atoi(start);
		} else {
			dir = opendir("/proc");
			if(!dir)
				break;
			tmp = word;
			while(*start != ' ' && *start) {
				*tmp++ = *start++;
			}
			*tmp = 0;
			while((dir_entry = readdir(dir)) != NULL) {
				if(dir_entry->d_type != DT_DIR)
					continue;
				if(is_digit(dir_entry->d_name)) {
					pid = atoi(dir_entry->d_name);
					sprintf(buffer, "/proc/%d/cmdline", pid);
					fp = fopen(buffer, "r");
					if(!fp) {
						pid = 0;
						continue;
					}
					memset(buffer, 0, MAX_NAME_SIZE);
					fread(buffer, MAX_NAME_SIZE, 1, fp);
					fclose(fp);
					if(strstr(buffer, word)) {
						break;
					}
					pid = 0;
				}
			}
			closedir(dir);
		}

		if(pid == 0)
			break;

		num++;

		/* if process exist, then add to list */
		sprintf(buffer, "/proc/%d", pid);
		if(path_exist(buffer)) {
			entry = insert_entry(pid, 0, &tick_entry_head);
			sprintf(buffer, "/proc/%d/task", pid);
			dir = opendir(buffer);
			if(dir != NULL) {
				while((dir_entry = readdir(dir)) != NULL) {
					if(dir_entry->d_type != DT_DIR)
						continue;
					if(is_digit(dir_entry->d_name)) {
						tid = atoi(dir_entry->d_name);
						if(tid == pid)
							continue;
						insert_entry(pid, atoi(dir_entry->d_name), entry);
					}
				}
				closedir(dir);
			}
		}

		tmp = strchr(start, ' ');
		if(tmp == NULL)
			break;
		start = tmp + 1;
	}
	return num;
}

static void print_pid_tick(unsigned long total_diff)
{
	struct tick_entry *entry = tick_entry_head.next;
	struct tick_entry *thread_entry;
	unsigned long usage;

	while(entry) {
		printf("\t\t%lu\t%lu\t%lu%%\t%08lu(%02lu%% of %08lu)\t%s(%d)\n",
				entry->utime, entry->stime,
				(entry->utime + entry->stime) * 100 / total_diff,
				entry->total,
				entry->total * 100 / cpu_tick_total, cpu_tick_total,
				entry->thread_name, entry->pid);
		thread_entry = entry->thread_next;
		while(thread_entry) {
			if(thread_entry->utime + thread_entry->stime == 0) {
				thread_entry = thread_entry->thread_next;
				continue;
			}
			usage = thread_entry->total * 100 / cpu_tick_total;
				printf("\t\t%lu\t%lu\t%lu%%\t%08lu(%02lu%% of %08lu)\t%d\n",
					thread_entry->utime, thread_entry->stime,
					(thread_entry->utime + thread_entry->stime) * 100 / total_diff,
					thread_entry->total, usage, cpu_tick_total, thread_entry->tid);
			thread_entry = thread_entry->thread_next;
		}
		printf("\n");
		entry = entry->next;
	}
	return;
}

int main(int argc, char *argv[])
{
	FILE *fp;
	char name[1024];
	int i, c, interval = 1, check_process = 0;
	unsigned long total_diff, user_diff, system_diff, idle_diff;
	unsigned long nice_diff, iowait_diff, irq_diff, softirq_diff;
	/* value[0] ... value[6]: user, nice, system, idle, iowait, irq, softirq; */
	unsigned long value[7], last_value[7];

	/* parse arguments */
	while( ( c = getopt( argc, argv, "hi:p:n:v" ) ) != -1 ) {
		switch( c ) {
		case 'i':
			interval = atoi(optarg);
			if(interval < 1)
				interval = 1;
			if(interval > MAX_INTERVAL)
				interval = MAX_INTERVAL;
			break;
		case 'p':
			check_process = 1;
			if(parse_pid_list(optarg, 0) <= 0) {
				printf("Usage: %s [-v] [-i interval] [-p pid_list] [-n name_list]\n", argv[0]);
				exit(0);
			}
			break;
				
		case 'n':
			check_process = 1;
			if(parse_pid_list(optarg, 1) <= 0) {
				printf("Usage: %s [-v] [-i interval] [-p pid_list] [-n name_list]\n", argv[0]);
				exit(0);
			}
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("Usage: %s [-v] [-i interval] [-p pid_list] [-n name_list]\n", argv[0]);
			exit(0);
		}
	}
	
	/* only specify certain process */
	if(check_process)
		get_pid_tick(1);

	/* get global cpu tick first time */
	get_cpu_tick(last_value);

	for(i = 0, c = 0; ;i++)
	{
		/* print header */
		if(!check_process) {
			if ((i&0xf) == 0) {
				if (verbose)
					printf("\t\tusage\ttotal\tuser\tnice\tsystem\tidle\tirq\tsoft\tiowait\n");
				else
					printf("\t\tusage\ttotal\tuser\tsystem\n");
			}
		}

		sleep(interval);

		/* get process cpu tick */
		if(check_process)
			get_pid_tick(0);

		/* get global cpu tick */
		get_cpu_tick(value);

		user_diff = value[0] - last_value[0];
		nice_diff = value[1] - last_value[1];
		system_diff = value[2] - last_value[2];
		idle_diff = value[3] - last_value[3];
		iowait_diff = value[4] - last_value[4];
		irq_diff = value[5] - last_value[5];
		softirq_diff = value[6] - last_value[6];

		total_diff = user_diff + nice_diff + system_diff
				+ idle_diff + iowait_diff + irq_diff + softirq_diff;
		cpu_tick_total += total_diff;
		gen_timestamp(name);
		if(check_process) {
			printf("%s\t(usage %02lu%%, total %lu, user %lu, system %lu)\n",
				name, (total_diff - idle_diff - iowait_diff) * 100 / total_diff,
				total_diff, user_diff + nice_diff,
				system_diff + irq_diff + softirq_diff);
			printf("\t\tuser\tsystem\tusage\ttotal\t\t\t\tthread_name(pid)\n");

			/* print process cpu usage,  we seperate process cpu tick collection and presentation */
			print_pid_tick(total_diff);

			printf("\t\t\tSummary:\t%08lu(%02lu%% of %08lu)\n",
						pid_tick_total,
						pid_tick_total * 100 / cpu_tick_total,
						cpu_tick_total);
		} else {
			/* print global cpu usage */
			if (verbose)
				printf("%s\t%lu%%\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n",
					name, (total_diff - idle_diff - iowait_diff) * 100 / total_diff,
					total_diff, user_diff, nice_diff, system_diff,
					idle_diff, iowait_diff, irq_diff, softirq_diff);
			else
				printf("%s\t%02lu%%\t%lu\t%lu\t%lu\n",
					name, (total_diff - idle_diff - iowait_diff) * 100 / total_diff,
					total_diff, user_diff + nice_diff,
					system_diff + irq_diff + softirq_diff);
		}
		memcpy(last_value, value, sizeof(value));
		if ((i&0xf) == 0)
			i = 1;
		printf("\n\n\n");
	}
	fclose(fp);
	return 0;
}

