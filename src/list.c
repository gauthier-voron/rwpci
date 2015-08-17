/*
 * Copyright 2015 Gauthier Voron
 * This file is part of rwpci.
 *
 * Rwpci is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Rwpci is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rwpci. If not, see <http://www.gnu.org/licenses/>.
 */


#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "pci.h"


#define BUFMINLEN  32


int8_t read_line(int fd, char **buffer, size_t *len)
{
	size_t off = 0, l = *len;
	char *new, *buf = *buffer;
	int8_t ret = -1;
	ssize_t rd;

	while (1) {
		if (off >= l) {
			new = realloc(buf, BUFMINLEN + (l << 1));
			if (!new) {
				elog("cannot read line");
				goto out;
			}

			buf = new;
			l = BUFMINLEN + (l << 1);
		}

		rd = read(fd, buf + off, 1);
		if (rd == EAGAIN)
			continue;
		
		if (rd == 0) {
			buf[off] = '\0';
			break;
		}
		
		if (buf[off] == '\n') {
			buf[off] = '\0';
			break;
		}

		off++;
	}

	ret = 0;
 out:
	*buffer = buf;
	*len = l;
	return ret;
}

uint8_t process_lines(int fd, const char *key)
{
	char *line = NULL, *ptr;
	size_t len = 0;
	uint8_t acc = 0;
	int8_t ret;

	while (1) {
		ret = read_line(fd, &line, &len);
		if (ret == -1) {
			acc = 0;
			break;
		}

		if (!strcmp(line, ""))
			break;

		if (key && strcasestr(line, key) == NULL)
			continue;

		ptr = line;
		while (*ptr != '\0' && *ptr != ' ' && *ptr != ':')
			ptr++;
		if (*ptr == ':')
			*ptr = '.';

		printf("%s\n", line);
		acc++;
	}

	free(line);
	return acc;
}

uint8_t list_pci(const char *key)
{
	pid_t pid;
	uint8_t count = 0;
	int status, fds[2];

	if (pipe(fds)) {
		elog("cannot create pipe");
		return 0;
	}

	if ((pid = fork())) {
		close(fds[1]);
		count = process_lines(fds[0], key);
		close(fds[0]);
		
		waitpid(pid, &status, 0);
		if (WEXITSTATUS(status) != 0)
			count = 0;
	} else {
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		execlp("lspci", "lspci", NULL);
		elog("cannot exec 'lspci'");
		exit(EXIT_FAILURE);
	}

	return count;
}
