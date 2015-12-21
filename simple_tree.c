/*    Copyright (C) 2015 - Wilfried Lucha */

/*    This program is free software: you can redistribute it and/or modify*/
/*    it under the terms of the GNU General Public License as published by*/
/*    the Free Software Foundation, either version 2 of the License, or*/
/*    (at your option) any later version.*/

/*    This program is distributed in the hope that it will be useful,*/
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of*/
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the*/
/*    GNU General Public License for more details.*/

/*    You should have received a copy of the GNU General Public License*/
/*    along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

/* Copyright (c) 1983, 1991 The Regents of the University of California.*/
/* And Copyright (C) 2011 Guillem Jover <guillem@hadrons.org>*/
/* And Copyright (C) 2006, 2014 Michael Kerrisk*/
/* All rights reserved.*/

/* Redistribution and use in source and binary forms, with or without */
/* modification, are permitted provided that the following conditions */
/* are met: */
/* 1. Redistributions of source code must retain the above copyright*/
/*    notice, this list of conditions and the following disclaimer.*/
/* 2. Redistributions in binary form must reproduce the above copyright*/
/*    notice, this list of conditions and the following disclaimer in the*/
/*    documentation and/or other materials provided with the distribution.*/
/* 3. All advertising materials mentioning features or use of this software*/
/* must display the following acknowledgement:*/
/* This product includes software developed by the University of*/
/* California, Berkeley and its contributors.*/
/* 4. Neither the name of the University nor the names of its contributors*/
/*    may be used to endorse or promote products derived from this software*/
/*    without specific prior written permission.*/

/* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND */
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE */
/* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE */
/* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS */
/* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) */
/* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT */
/* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF */
/* SUCH DAMAGE. */

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <getopt.h>

#define MAX_SIZE 1024

/* colours for a beautiful coloured output */
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KWHT  "\x1B[37m"
#define KCYN  "\x1B[36m"
#define KMAG  "\x1B[35m"
#define RESET "\033[0m"

/* print usage/help */
void usage(int nbl, char *argv[])
{
	printf("\nUsage: %s [-i] [-l] [-e] [-n value] [-t seconds]\n", argv[0]);
	printf("-i --inode	Display the inode of the directory/file\n");
	printf("-l --link	Mark symbolic links\n");
	printf("-h --help	Print this help screen\n");
	printf("-e --empty	Mark empty files\n");
	printf("-s --size	Print file size in bytes/kilobytes\n");
	printf("-n --nice	Set niceness range from %d to 19\n", nbl);
	printf("-t --time	Set maximum execution time in seconds\n");
	printf("-v --version	Print version info and license\n\n");
}

/* print version and license */
void version(void)
{
	printf("\nsimple_tree 0.0.1 - (C) 2015 Wilfried Lucha\n");
	printf("Released under GNU GPL\n\n");


	printf("    This program is free software:\n"
	"    you can redistribute it and/or modify\n"
	"    it under the terms of the GNU General Public License as published by\n"
	"    the Free Software Foundation, either version 2 of the License, or\n"
	"    (at your option) any later version.\n"
	"\n"
	"    This program is distributed in the hope that it will be useful,\n"
	"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	"    GNU General Public License for more details.\n"
	"\n"
	"    You should have received a copy of the GNU General Public License\n"
	"    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
	"\n    --------------------------------------------------------------\n"
	"    This product includes software developed by the University of\n"
	"    California, Berkeley and its contributors.\n\n");
}

/* signal handling (CTRL-C) */
void sig_handler(int sig)
{
	char signal;

	if (sig == SIGINT) {
		fprintf(stderr, "\nProgram stopped!\n");
		exit(EXIT_FAILURE);
	} else
		return;
}

void timer(time)
{
	/* set maximum execution time in seconds */
	if (time > 0) {

		printf("Maximum execution time: %d seconds\n\n", time);
		struct itimerval timer;

		timer.it_value.tv_sec = time;
		timer.it_value.tv_usec = 0;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
	}
}
/* output inode and name of directory + mark as a directory */
void output_directories(struct dirent *d, int tier, char d_type,
				long d_ino, char d_name, int iflag)
{
	printf("%*s", tier+1, "");

	/* print inode and directory name */
	if (iflag)
		printf(KBLU "[%5ld] " RESET, d->d_ino);

	/* marking of file types */
	printf("%s", (!iflag) ? KCYN "( dir)" RESET : "");

	if (tier == 0 && iflag)
		printf(" %s/\n", d->d_name);
	else
	printf(" %s\n", d->d_name);
}

/* print inode, print filename, mark filetype */
void output_files(struct dirent *d, int tier, char d_type,
		long d_ino, char d_name, int lflag, int iflag)
{
	printf("%*s", tier+1, "");

	/* print inode and file name */
	if (iflag)
		printf(KBLU "[%5ld] " RESET, d->d_ino);

	/* mark file types */
	printf((d->d_type == DT_REG && !iflag) ? KGRN "(file) " RESET :
	(d->d_type == DT_LNK && lflag && !iflag) ? KYEL "(link) " RESET :
	(d->d_type == DT_UNKNOWN) ? KRED "(file type unknown!) " RESET : "");

	printf("%s", d->d_name);
}

/* Copyright (c) 1983, 1991 The Regents of the University of California.*/
/* And Copyright (C) 2011 Guillem Jover <guillem@hadrons.org>*/
/* And Copyright (C) 2006, 2014 Michael Kerrisk*/
/* All rights reserved.*/

/* mark empty files and print target of symblic links */
void filesize_linktarget(struct dirent *d, const char *name,
		char d_type, char d_name, char path[MAX_SIZE],
			int lflag, int eflag, int iflag, int sflag)
{
	struct stat sb;
	char *linkname;
	ssize_t r;
	int size;

	/* determine absolute path of a file */
	strcpy(path, name);
	strcat(path, "/");
	strcat(path, d->d_name);
	lstat(path, &sb);

	size = sb.st_size;

	if (lstat(path, &sb) == -1) {
		perror("lstat");
		exit(EXIT_FAILURE);
	}

	/* print target of symbolic link */
	if (d->d_type == DT_LNK && lflag && !iflag) {

		/* Initial memory allocation */
		linkname = malloc(sb.st_size + 1);

		if (linkname == NULL) {
			fprintf(stderr, "insufficient memory\n");
			exit(EXIT_FAILURE);
			}

		r = readlink(path, linkname, sb.st_size + 1);

		if (r == (ssize_t)-1) {
			free(linkname);
			perror("readlink");
			exit(EXIT_FAILURE);
		}

		if (r > (ssize_t)sb.st_size) {
			fprintf(stderr, "symlink increased in size");
				printf("between lstat() and readlink()\n");
			exit(EXIT_FAILURE);
		}

		linkname[sb.st_size] = '\0';
		printf(KYEL " -> '%s'" RESET, linkname);
		free(linkname);
	}

	/* mark empty files */
	printf("%s", (size == 0 && eflag && !iflag) ? KRED " (empty)" RESET :
	(size == -1 && eflag && !iflag) ?
		KRED "Error checking file size!" RESET :  "");

	/* print filesize in bytes/kilobytes */
	if (sflag == 1) {
		if (size >= 1024)
			printf(KMAG" [%3dK]" RESET, size / 1024);

		else
			printf(KMAG" [%4d]" RESET, size);
	}
	printf("\n");
}

int tree(const char *name, int tier,
			int iflag, int lflag, int eflag, int sflag)
{
	DIR *dir;
	struct dirent *d = (struct dirent *)malloc(sizeof(struct dirent *));
	char d_type;
	long d_ino;
	char d_name;
	int length;
	char path[MAX_SIZE];

	dir = opendir(name);
	d = readdir(dir);

	if (dir == NULL) {
		perror(KRED "directory" RESET);
		return;
	}

	if (d == NULL)
		return;

	errno = 0;

	/* Loop through directory entries. */
	while ((d = readdir(dir)) != NULL) {
		if (d->d_type == DT_DIR) {

			length = snprintf(path, sizeof(path)-1,
					"%s/%s", name, d->d_name);

			if (length >= MAX_SIZE) {
				perror(KRED "Path length got too long!" RESET);
				return EXIT_FAILURE;
			}
			/* skip if entries for dot and dot-dot exist */
			if (strcmp(d->d_name, ".") == 0 ||
			strcmp(d->d_name, "..") == 0)
				continue;

			output_directories(d, tier, d_type,
					d_ino, d_name, iflag);

			/* recursion */
			tree(path, tier+6, iflag, lflag, eflag, sflag);
		} else {

			output_files(d, tier, d_type, d_ino,
						d_name, lflag, iflag);
			filesize_linktarget(d, name, d_type, d_name,
					path, lflag, eflag, iflag, sflag);
			}
		}
	if (errno != 0)
		perror(KRED "error reading directory" RESET);

	if (closedir(dir) == -1) {
		perror(KRED "closedir" RESET);
		return EXIT_FAILURE;
	}
	free(d);
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int option;
	int nbl = 0;
	int time = 0;
	int iflag = 0;
	int lflag = 0;
	int eflag = 0;
	int sflag = 0;
	int nflag = 0;
	int niceness = 0;

	/* check effective user id and set nbl as niceness bottom limit */
	if (geteuid() == 0)
		nbl = -20;
	else
		nbl = 0;

	int this_option_optind = optind ? optind : 1;
	const char *short_opt = "hilest:n:v";

	/* argument handling via getopt and getopt_long */
	static struct option long_opt[] = {
		{"help",	no_argument,	NULL, 'h'},
		{"inode",	no_argument,	NULL, 'i'},
		{"link",	no_argument,	NULL, 'l'},
		{"empty",	no_argument,	NULL, 'e'},
		{"size",	no_argument,	NULL, 's'},
		{"time",	required_argument,	NULL, 't'},
		{"nice",	required_argument,	NULL, 'n'},
		{"version",	no_argument,	NULL, 'v'},
		{NULL,		0,	NULL, 0  }
		};

	while ((option = getopt_long(argc, argv,
		short_opt, long_opt, NULL)) != -1) {
		switch (option) {
		case 'h':
			usage(nbl, argv);
			exit(-1);
		case 'i':
			iflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 't':
			time = atoi(optarg);
			if (time < 1) {
				fprintf(stderr, KRED "\nError: Execution");
				printf("timeout must be equal");
				printf(" to or greater then 1 second!\n" RESET);
				exit(EXIT_FAILURE);
				}
			break;
		case 'n':
			niceness = atoi(optarg);
			if (niceness < nbl || niceness > 19) {
				fprintf(stderr, KRED "\n Error:");
				fprintf(stderr, " Niceness value");
				fprintf(stderr, "must be between ");
				fprintf(stderr, "%d and 19\n" RESET, nbl);
				usage(nbl, argv);
				exit(EXIT_FAILURE);
			} else {
				nice(niceness);
				nflag = 1;
			}

			if (nice(niceness) == -1) {
				perror("nice");
				exit(EXIT_FAILURE);
			}
			break;
		case 'e':
			eflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'v':
			version();
			exit(EXIT_FAILURE);
		default:
			usage(nbl, argv);
			exit(EXIT_FAILURE);
			}
	}
	 /* check input parameters */
	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
		usage(nbl, argv);
		exit(EXIT_FAILURE);
	}

	timer(time);

	/* call signal handling function */
	signal(SIGINT, sig_handler);

	/* call tree function with root folder */
	tree(".", 0, iflag, lflag, eflag, sflag);

	/* print niceness value */
	if (nice(niceness) != -1 && nflag == 1)
		printf("\nNiceness: %d\n", niceness);

	exit(EXIT_SUCCESS);
}
