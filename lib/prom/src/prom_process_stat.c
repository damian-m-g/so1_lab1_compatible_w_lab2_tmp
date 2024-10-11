/**
 * Copyright 2019-2020 DigitalOcean Inc.
 * Copyright 2021 Jens Elkner <jel+libprom@cs.uni-magdeburg.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __sun
	#define _STRUCTURED_PROC 1
	#include <sys/procfs.h>
	#include <sys/resource.h>
#else
	// assume Linux
	#include <sys/sysinfo.h>
#endif

// Public
#include "../include/prom_alloc.h"
#include "../include/prom_counter.h"
#include "../include/prom_gauge.h"
#include "../include/prom_log.h"

// Private
#include "prom_assert.h"
#include "prom_process_collector_t.h"
#include "prom_process_stat_t.h"

/**
 * @brief Initializes each gauge metric
 */
int
ppc_stats_new(prom_metric_t *m[]) {
	if (m == NULL)
		return 0;

	// /proc/self/stat Field 10
	m[PM_MINFLT] = prom_counter_new("process_minor_pagefaults",
		"Number of minor faults of the process "
		"not caused a page load from disk", 0, NULL);
	// /proc/self/stat Field 12
	m[PM_MAJFLT] = prom_counter_new("process_major_pagefaults",
		"Number of major faults of the process "
		"caused a page load from disk", 0, NULL);
#ifdef __sun
	m[PM_CPU_UTIL] = prom_gauge_new("process_cpu_utilization_percent",
		"Percent of recent cpu time used by all lwps", 0, NULL);
	m[PM_MEM_UTIL] = prom_gauge_new("process_mem_utilization_percent",
		"Percent of system memory used by process", 0, NULL);
#else	// assume Linux
	// /proc/self/stat Field 11
	m[PM_CMINFLT] = prom_counter_new("process_children_minor_pagefaults",
		"Number of minor faults of the process waited-for children "
		"not caused a page load from disk", 0, NULL);
	// /proc/self/stat Field 13
	m[PM_CMAJFLT] = prom_counter_new("process_children_major_pagefaults",
		"Number of major faults of the process's waited-for children "
		"caused a page load from disk", 0, NULL);
#endif

	// /proc/self/stat Field 14
	m[PM_UTIME] = prom_counter_new("process_user_cpu_seconds",
		"Total CPU time the process spent in user mode in seconds", 0, NULL);
	// /proc/self/stat Field 15
	m[PM_STIME] = prom_counter_new("process_system_cpu_seconds",
		"Total CPU time the process spent in kernel mode in seconds", 0, NULL);
	// /proc/self/stat Field 14 + 15
	m[PM_TIME] = prom_counter_new("process_total_cpu_seconds",
		"Total CPU time the process spent in user and kernel mode in seconds",
		0, NULL);
	// /proc/self/stat Field 16
	m[PM_CUTIME] = prom_counter_new("process_children_user_cpu_seconds",
		"Total CPU time the process's waited-for children spent in user mode "
	    "in seconds", 0, NULL);
	// /proc/self/stat Field 17
	m[PM_CSTIME] = prom_counter_new("process_children_system_cpu_seconds",
		"Total CPU time the process's waited-for children spent in kernel mode "
	    "in seconds", 0, NULL);
	// /proc/self/stat Field 16 + 17
	m[PM_CTIME] = prom_counter_new("process_children_total_cpu_seconds",
		"Total CPU time the process's waited-for children spent in user and "
		"in kernel mode in seconds", 0, NULL);

	// /proc/self/stat Field 20
	m[PM_NUM_THREADS] = prom_gauge_new("process_threads_total",
		"Number of threads in this process", 0, NULL);

	// now - /proc/uptime + /proc/self/stat Field 22
	m[PM_STARTTIME] = prom_counter_new("process_start_time_seconds",
		"The time the process has been started in seconds elapsed since Epoch",
		0, NULL);

	// /proc/self/stat Field 23
	m[PM_VSIZE] = prom_gauge_new("process_virtual_memory_bytes",
			"Virtual memory size in bytes", 0, NULL);
	// /proc/self/stat Field 24
	m[PM_RSS] = prom_gauge_new("process_resident_memory_bytes",
		"Resident set size of memory in bytes", 0, NULL);

#ifdef __sun
	m[PM_VCTX] = prom_counter_new("process_voluntary_ctxsw_total",
		"Number of voluntary context switches", 0, NULL);
	m[PM_ICTX] = prom_counter_new("process_involuntary_ctxsw_total",
		"Number of involuntary context switches", 0, NULL);
#else // assume Linux
	// /proc/self/stat Field 25
	m[PM_BLKIO] = prom_counter_new("process_delayacct_blkio_ticks",
		"Aggregated block I/O delays, measured in clock ticks (centiseconds)",
		0, NULL);
#endif

	int res = 0;
	for (int i = PM_MINFLT; i < PM_COUNT; i++)
		if (m[i] != NULL)
			res |= 1 << i;
	return res;
}

#ifdef __sun

#define TIME_TO_DBL(x)		(x).tv_sec + (x).tv_nsec * 1e-9

int
ppc_stats_update(int fd[], prom_metric_t *m[], const char **lvals) {
	pstatus_t	status;
	psinfo_t	psinfo;
	prusage_t	usage;

	int res = 0;
	double a, b;

	if (fd[FD_STAT] < 0
		|| (pread(fd[FD_STAT], &status, sizeof(pstatus_t), 0)) == -1)
	{
		perror("status");
		res |= gup(PM_NUM_THREADS, NaN);
		res |= cup(PM_UTIME, NaN);
		res |= cup(PM_STIME, NaN);
		res |= cup(PM_TIME, NaN);
		res |= cup(PM_CUTIME, NaN);
		res |= cup(PM_CSTIME, NaN);
		res |= cup(PM_CTIME, NaN);
	} else {
#ifdef CREATE_TESTFILES
		FILE *f = fopen("/tmp/status", "w+");
		fwrite(&status, sizeof(pstatus_t), 1, f);
		fclose(f);
#endif
		res |= gup(PM_NUM_THREADS, status.pr_nlwp + status.pr_nzomb);	// (20)
		a = TIME_TO_DBL(status.pr_utime);
		b = TIME_TO_DBL(status.pr_stime);
		res |= cup(PM_UTIME, a);										// (14)
		res |= cup(PM_STIME, b);										// (15)
		res |= cup(PM_TIME, a + b);
		a = TIME_TO_DBL(status.pr_cutime);
		b = TIME_TO_DBL(status.pr_cstime);
		res |= cup(PM_CUTIME, a);										// (16)
		res |= cup(PM_CSTIME, b);										// (17)
		res |= cup(PM_CTIME, a + b);
	}

	if (fd[FD_PSINFO] < 0
		|| (pread(fd[FD_PSINFO], &psinfo, sizeof(psinfo_t), 0)) == -1)
	{
		perror("psinfo");
		res |= gup(PM_VSIZE, NaN);
		res |= gup(PM_RSS, NaN);
		res |= gup(PM_CPU_UTIL, NaN);
		res |= gup(PM_MEM_UTIL, NaN);
		res |= cup(PM_STARTTIME, NaN);
	} else {
#ifdef CREATE_TESTFILES
		FILE *f = fopen("/tmp/psinfo", "w+");
		fwrite(&status, sizeof(psinfo), 1, f);
		fclose(f);
#endif
		// num_threads = psinfo.pr_nlwp + psinfo.pr_nzomb;	// (20)
		res |= gup(PM_VSIZE, psinfo.pr_size << 10);						// (23)
		res |= gup(PM_RSS, psinfo.pr_rssize << 10);						// (24)
		res |= gup(PM_CPU_UTIL, 100.0 * psinfo.pr_pctcpu / 0x8000);
		res |= gup(PM_MEM_UTIL, 100.0 * psinfo.pr_pctmem / 0x8000);
		res |= cup(PM_STARTTIME, psinfo.pr_start.tv_sec);				// (22)
	}
	if (fd[FD_USAGE] < 0
		|| (pread(fd[FD_USAGE], &usage, sizeof(prusage_t), 0)) == -1)
	{
		perror("usage");
		res |= cup(PM_MINFLT, NaN);
		res |= cup(PM_MAJFLT, NaN);
		res |= cup(PM_VCTX, NaN);
		res |= cup(PM_ICTX, NaN);
	} else {
#ifdef CREATE_TESTFILES
		FILE *f = fopen("/tmp/usage", "w+");
		fwrite(&status, sizeof(usage), 1, f);
		fclose(f);
#endif
		res |= cup(PM_MINFLT, usage.pr_minf);							// (10)
		res |= cup(PM_MAJFLT, usage.pr_majf);							// (12)
		res |= cup(PM_VCTX, usage.pr_vctx);
		res |= cup(PM_ICTX, usage.pr_ictx);
	}
	return res;
}

#else // assume linux

static int
fill_stats(stats_t *stats, int fd) {
	// 12 * 11 + 1 * 18 + 1 * 2 + 18 * 21 = 530 but nice,priority have < 5, so:
	char line[512];
	char comm[18];	// the manpage says max 16 chars incl. '\0' + ()

	static int PAGE_SZ = 0;
	static unsigned long TPS = 0;
	static unsigned long long last_starttime = 0;
	static unsigned long long timestamp = 0;

	if (PAGE_SZ == 0) {
		PAGE_SZ = sysconf(_SC_PAGE_SIZE);
		TPS = sysconf(_SC_CLK_TCK);
	}

	ssize_t len = pread(fd, line, sizeof(line) - 1, 0);
	line[len] = '\0';

	int n = sscanf((const char *) line,
"%d "	// (1) pid  %d
"%s "	// (2) comm  %s
"%c "	// (3) state  %c
"%d "	// (4) ppid  %d
"%d "	// (5) pgrp  %d
"%d "	// (6) session  %d
"%d "	// (7) tty_nr  %d
"%d "	// (8) tpgid  %d
"%u "	// (9) flags  %u
"%lu "	// (10) minflt  %lu
"%lu "	// (11) cminflt  %lu
"%lu "	// (12) majflt  %lu
"%lu "	// (13) cmajflt  %lu
"%lf "	// (14) utime  %lu
"%lf "	// (15) stime  %lu
"%lf "	// (16) cutime  %ld
"%lf "	// (17) cstime  %ld
"%ld "	// (18) priority  %ld
"%ld "	// (19) nice  %ld
"%ld "	// (20) num_threads  %ld
"%ld "	// (21) itrealvalue  %ld
"%llu "	// (22) starttime  %llu
"%lu "	// (23) vsize  %lu
"%ld "	// (24) rss  %ld
"%lu "	// (25) rsslim  %lu
"%lu "	// (26) startcode  %lu  [PT]
"%lu "	// (27) endcode  %lu  [PT]
"%lu "	// (28) startstack  %lu  [PT]
"%lu "	// (29) kstkesp  %lu  [PT]
"%lu "	// (30) kstkeip  %lu  [PT]
"%lu "	// (31) signal  %lu
"%lu "	// (32) blocked  %lu
"%lu "	// (33) sigignore  %lu
"%lu "	// (34) sigcatch  %lu
"%lu "	// (35) wchan  %lu  [PT]
"%lu "	// (36) nswap  %lu
"%lu "	// (37) cnswap  %lu
"%d "	// (38) exit_signal  %d  (since Linux 2.1.22)
"%d "	// (39) processor  %d  (since Linux 2.2.8)
"%u "	// (40) rt_priority  %u  (since Linux 2.5.19)
"%u "	// (41) policy  %u  (since Linux 2.5.19)
"%llu "	// (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
"%lu "	// (43) guest_time  %lu  (since Linux 2.6.24)
"%ld "	// (44) cguest_time  %ld  (since Linux 2.6.24)
"%lu "	// (45) start_data  %lu  (since Linux 3.3)  [PT]
"%lu "	// (46) end_data  %lu  (since Linux 3.3)  [PT]
"%lu "	// (47) start_brk  %lu  (since Linux 3.3)  [PT]
"%lu "	// (48) arg_start  %lu  (since Linux 3.5)  [PT]
"%lu "	// (49) arg_end  %lu  (since Linux 3.5)  [PT]
"%lu "	// (50) env_start  %lu  (since Linux 3.5)  [PT]
"%lu "	// (51) env_end  %lu  (since Linux 3.5)  [PT]
"%d ",	// (52) exit_code  %d  (since Linux 3.5)  [PT]

		&stats->pid,			// (1)
		comm,					// (2)
		&stats->state,			// (3)
		&stats->ppid,			// (4)
		&stats->pgrp,			// (5)
		&stats->session,		// (6)
		&stats->tty_nr,			// (7)
		&stats->tpgid,			// (8)
		&stats->flags,			// (9)
		&stats->minflt,			// (10)
		&stats->cminflt,		// (11)
		&stats->majflt,			// (12)
		&stats->cmajflt,		// (13)
		&stats->utime,			// (14)
		&stats->stime,			// (15)
		&stats->cutime,			// (16)
		&stats->cstime,			// (17)
		&stats->priority,		// (18)
		&stats->nice,			// (19)
		&stats->num_threads,	// (20)
		&stats->itrealvalue,	// (21)
		&stats->starttime,		// (22)
		&stats->vsize,			// (23)
		&stats->rss,			// (24)
		&stats->rsslim,			// (25)
		&stats->startcode,		// (26)
		&stats->endcode,		// (27)
		&stats->startstack,		// (28)
		&stats->kstkesp,		// (29)
		&stats->kstkeip,		// (30)
		&stats->signal,			// (31)
		&stats->blocked,		// (32)
		&stats->sigignore,		// (33)
		&stats->sigcatch,		// (34)
		&stats->wchan,			// (35)
		&stats->nswap,			// (36)
		&stats->cnswap,			// (37)
		&stats->exit_signal,	// (38)
		&stats->processor,		// (39)
		&stats->rt_priority,	// (40)
		&stats->policy,			// (41)
		&stats->blkio,			// (42)
		&stats->guest_time,		// (43)
		&stats->cguest_time,	// (44)
		&stats->start_data,		// (45)
		&stats->end_data,		// (46)
		&stats->start_brk,		// (47)
		&stats->arg_start,		// (48)
		&stats->arg_end,		// (49)
		&stats->env_start,		// (50)
		&stats->env_end,		// (51)
		&stats->exit_code		// (52)
	);
	if (n < 42) {
		PROM_WARN("Incomplete /proc/self/stat line: %s", line);
		return 4;
	}

	stats->utime /= TPS;
	stats->stime /= TPS;
	stats->cutime /= TPS;
	stats->cstime /= TPS;
	stats->rss *= PAGE_SZ;

	if (last_starttime != stats->starttime || timestamp == 0) {
		// usually a constant value. However if this fn gets called
		// for several other processes ...
		struct sysinfo s_info;
		time_t now = time(NULL);
		long uptime = sysinfo(&s_info) ? now : s_info.uptime;
		last_starttime = stats->starttime;
		timestamp = now - uptime + (stats->starttime / TPS);
	}
	stats->starttime = timestamp;
	return 0;
}

int
ppc_stats_update(int fd[], prom_metric_t *m[], const char **lvals) {
	int c = 1;
	stats_t stats;

	if (fd != NULL && fd[FD_STAT] >= 0)
		c = fill_stats(&stats, fd[FD_STAT]);

	int res = 0;
	res |= cup(PM_MINFLT, c ? NaN : stats.minflt);
	res |= cup(PM_MAJFLT, c ? NaN : stats.majflt);
	res |= cup(PM_CMINFLT, c ? NaN : stats.cminflt);
	res |= cup(PM_CMAJFLT, c ? NaN : stats.cmajflt);
	res |= cup(PM_UTIME, c ? NaN : stats.utime);
	res |= cup(PM_STIME, c ? NaN : stats.stime);
	res |= cup(PM_TIME, c ? NaN : (stats.utime + stats.stime));
	res |= cup(PM_CUTIME, c ? NaN : stats.cutime);
	res |= cup(PM_CSTIME, c ? NaN : stats.cstime);
	res |= cup(PM_CTIME, c ? NaN : (stats.cutime + stats.cstime));
	res |= gup(PM_NUM_THREADS, c ? NaN : stats.num_threads);
	res |= cup(PM_STARTTIME, c ? NaN : stats.starttime);
	res |= gup(PM_VSIZE, c ? NaN : stats.vsize);
	res |= gup(PM_RSS, c ? NaN : stats.rss);
	res |= cup(PM_BLKIO, c ? NaN : stats.blkio);

	return res;
}
#endif
