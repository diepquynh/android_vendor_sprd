/*
 * Author: Jan Edler
 * copyright?:  I recommend one.  
 *
 * Try to measure and report memcpy bandwidth of parallel processes.
 * Synchronization is coarse, to be as portable as possible,
 * at the expense of some timing accuracy.
 */

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

char usage[] = "Usage: %s [-v] [-c chunk_size] [-n nchunks] [-p nprocesses]\n";
int chunk_size = 4*1024*1024, nchunks = 10, nprocesses = 1;
int verbose = 0;

#ifndef do_memcpy
#define do_memcpy(d,s,c)	memcpy(d,s,c)
#endif

int
main (int argc, char **argv)
{
	int c, n;
	char *src, *dst;
	int pipe_barrier[2];
	int pipe_results[2];
	char junk;
	struct timeval tv_start, tv_end;
	double *all_times, *next_time;
	double min_start = DBL_MAX, max_start = DBL_MIN,
		min_end = DBL_MAX, max_end = DBL_MIN;

	while ((c = getopt (argc, argv, "vc:n:p:")) != EOF)
		switch (c) {
		default:	fprintf (stderr, usage, argv[0]); return 1;
		case 'v':	verbose = 1; break;
		case 'c':	chunk_size = atoi(optarg); break;
		case 'n':	nchunks = atoi(optarg); break;
		case 'p':	nprocesses = atoi(optarg); break;
		}
	if (optind < argc) {
		fprintf (stderr, usage, argv[0]);
		return 1;
	}
	printf ("%s: chunk_size = %d, nchunks = %d, nprocesses = %d\n",
		argv[0], chunk_size, nchunks, nprocesses);
	if (pipe (pipe_barrier) < 0) {
		perror ("pipe pipe_barrier");
		return 2;
	}
	if (pipe (pipe_results) < 0) {
		perror ("pipe pipe_results");
		return 2;
	}
	src = malloc (chunk_size);
	if (src == NULL) {
		fprintf (stderr, "%s: malloc(%d) failed for src\n",
			argv[0], chunk_size);
		return 2;
	}
	dst = malloc (chunk_size);
	if (dst == NULL) {
		fprintf (stderr, "%s: malloc(%d) failed for dst\n",
			argv[0], chunk_size);
		return 2;
	}
	all_times = malloc (nprocesses * 2 * sizeof(double));
	if (all_times == NULL) {
		fprintf (stderr, "%s: malloc(%d) failed for all_times\n",
			argv[0], nprocesses * 2 * sizeof(double));
		return 2;
	}
	for (c = nprocesses;  c > 1;  c--) {
		switch (fork()) {
		case -1:	perror ("fork"); return 2;
		case 0:		break;		/* child: start waiting */
		default:	continue;	/* parent: continue forking */
		}
		break;				/* child */
	}

	memset (src, 0, chunk_size);	/* initialize and break copy on write */
	memset (dst, 0, chunk_size);

	if (close (pipe_barrier[1]) < 0) {
		perror ("close pipe_barrier[1]");
		return 2;
	}
	/* this read will get EOF when all the processes have started */
	if (read (pipe_barrier[0], &junk, 1) < 0) {
		perror ("pipe read");
		return 2;
	}
	if (gettimeofday (&tv_start, NULL) < 0) {
		perror ("gettimeofday 0");
		return 2;
	}
	/* this is the stuff being measured */
	for (n = nchunks;  n > 0;  n--)
		do_memcpy (dst, src, chunk_size);

	if (gettimeofday (&tv_end, NULL) < 0) {
		perror ("gettimeofday 1");
		return 2;
	}
	all_times[0] = (double)tv_start.tv_sec + tv_start.tv_usec / 1000000.;
	all_times[1] = (double)tv_end.tv_sec + tv_end.tv_usec / 1000000.;
	if (verbose)
		printf ("p%d: %d bytes copied in %f secs = %f MB/s\n",
			c, chunk_size * nchunks, all_times[1] - all_times[0],
			(double)(chunk_size * nchunks) /
			(all_times[1] - all_times[0]) / (1024.*1024.));

	if (c>1) {	/* write time values back to parent */
		errno = 0;	/* should check return value instead XXX */
		if (write (pipe_results[1], all_times, 2*sizeof(double)) !=
		    2*sizeof(double)) {
			perror ("write pipe_results[1]");
			return 2;
		}
	}
	if (close (pipe_results[1]) < 0) {
		perror ("close pipe_results[1]");
		return 2;
	}
	if (c==1) {	/* collect and summarize the results */
		next_time = &all_times[2];
		/* should be more careful about errors & overrun XXX */
		while (read (pipe_results[0], next_time,
			     2 * sizeof(double)) == 2 * sizeof(double))
			next_time += 2;
		if (next_time != &all_times[2*nprocesses]) {
			fprintf (stderr, "%s: next_time=%d is wrong\n",
				argv[0], next_time - all_times);
			return 2;
		}
		while (wait(NULL) >= 0)
			continue;	/* should check for errors XXX */
		/*
		 * Use earliest start time and latest stop time.
		 * Also warn about non-overlap.
		 */
		for (n = 0;  n < nprocesses;  n++) {
			if (min_start > all_times[n*2+0])
				min_start = all_times[n*2+0];
			if (max_start < all_times[n*2+0])
				max_start = all_times[n*2+0];
			if (min_end > all_times[n*2+1])
				min_end = all_times[n*2+1];
			if (max_end < all_times[n*2+1])
				max_end = all_times[n*2+1];
		}
		printf ("%d*%d bytes copied in %f secs = %d*%f MB/s = %f MB/s",
			nprocesses, chunk_size * nchunks, max_end - min_start,
			nprocesses, ((double)chunk_size * nchunks) /
			(max_end - min_start) / (1024.*1024.),
			((double)nprocesses * chunk_size * nchunks) /
			(max_end - min_start) / (1024.*1024.));
		if (max_start >= min_end)
			printf (" (some non-overlap occured)");
		printf ("\n");
	}
	return 0;
}
