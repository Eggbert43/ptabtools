/*
   Parser library for the PowerTab (PTB) file format
   Parser for tuning dictionary files
   (c) 2005: Jelmer Vernooij <jelmer@samba.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include "dlinklist.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
typedef int ssize_t;
#endif

#define PTB_CORE
#include "ptb.h"

#define malloc_p(t,n) (t *) calloc(sizeof(t), n)

struct ptb_tuning_dict *ptb_read_tuning_dict(const char *f)
{
	struct ptb_tuning_dict *ptbf = malloc_p(struct ptb_tuning_dict, 1);
	char unknown[8];
	int i;
	int fd;

	fd = open(f, O_RDONLY
#ifdef O_BINARY
						| O_BINARY
#endif
				   );
	
	read(fd, &ptbf->nr_tunings, 1);
	read(fd, unknown, 7);
	read(fd, unknown, 7); /* Class name (CTuning) */

	ptbf->tunings = malloc_p(struct ptb_tuning, ptbf->nr_tunings);
	for (i = 0; i < ptbf->nr_tunings; i++) {
		uint8_t name_len;
		read(fd, &name_len, 1);

		ptbf->tunings[i].name = malloc_p(char, name_len+1);
		read(fd, ptbf->tunings[i].name, name_len);
		ptbf->tunings[i].name[name_len] = '\0';

		read(fd, &ptbf->tunings[i].capo, 1);
		read(fd, &ptbf->tunings[i].nr_strings, 1);
		ptbf->tunings[i].strings = malloc_p(uint8_t, ptbf->tunings[i].nr_strings);
		read(fd, ptbf->tunings[i].strings, ptbf->tunings[i].nr_strings);

		read(fd, unknown, 2);
	}

	close(fd);

	return ptbf;
}

int ptb_write_tuning_dict(const char *f, struct ptb_tuning_dict *t)
{
	/* FIXME */
	return 0;
}

void ptb_free_tuning_dict(struct ptb_tuning_dict *t)
{
	int i;
	for (i = 0; i < t->nr_tunings; i++) {
		free(t->tunings[i].name);
		free(t->tunings[i].strings);
	}
	free(t->tunings);
	free(t);
}