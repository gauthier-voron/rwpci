#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pci.h"


static uint8_t compare_bdf(const char *dname, const struct pci_command *cmd)
{
	uint64_t val;
	char *err;

	(void) strtol(dname, &err, 16);
	if (*err != ':')
		return 1;
	dname = err + 1;

#define CHECK_BDF(term, field)			\
	val = strtol(dname, &err, 16);		\
	if (*err != term)			\
		return -1;			\
	if (val != cmd->field)			\
		return -1;			\
	dname = err + 1;
	CHECK_BDF( ':', bus);
	CHECK_BDF( '.', device);
	CHECK_BDF('\0', function);
#undef CHECK_BDF
	
	return 0;
}

static uint8_t compare_pci(const char *dname, const struct pci_command *cmd)
{
	uint64_t bus;
	char *err;
	
	if (dname[0] != 'p' || dname[1] != 'c' || dname[2] != 'i')
		return 1;
	dname += 3;

	(void) strtol(dname, &err, 16);
	if (*err != ':')
		return 1;
	dname = err + 1;

	bus = strtol(dname, &err, 16);
	if (*err != '\0')
		return 1;
	if (bus != cmd->bus)
		return 1;

	return 0;
}


static int open_bdf_config(const struct pci_command *cmd, char *path,
			   size_t off, size_t len)
{
	int flags;
	int fd;
	
	if (len - off < 7)
		return -1;
	strcpy(path + off, "config");

	if (cmd->flags & PCI_COMMAND_WRITE)
		flags = O_RDWR;
	else
		flags = O_RDONLY;
	
	fd = open(path, flags);
	if (fd == -1)
		elog("cannot open '%s'", path);

	return fd;
}

static int open_pci_config(const struct pci_command *cmd, char *path,
			   size_t off, size_t len)
{
	struct dirent *entry;
	uint8_t found = 0;
	int fd = -1;
	size_t add;
	DIR *dir;

	dir = opendir(path);
	if (!dir) {
		elog("cannot list '%s'", path);
		goto err;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (compare_bdf(entry->d_name, cmd))
			continue;
		found = 1;

		add = strlen(entry->d_name);
		if (add + 2 >= len - off)
			goto out;

		strcpy(path + off, entry->d_name);
		path[off + add] = '/';
		path[off + add + 1] = '\0';

		fd = open_bdf_config(cmd, path, off + add + 1, len);
		if (fd != -1)
			goto out;
	}	

	if (found == 0)
		vlog("cannot find pci %x.%x.%x",
		     cmd->bus, cmd->device, cmd->function);
	
 out:
	closedir(dir);
 err:
	return fd;
}

static int open_sysdev_config(const struct pci_command *cmd, char *path,
			      size_t off, size_t len)
{
	struct dirent *entry;
	uint8_t found = 0;
	int fd = -1;
	size_t add;
	DIR *dir;

	dir = opendir(path);
	if (!dir) {
		elog("cannot list '%s'", path);
		goto err;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (compare_pci(entry->d_name, cmd))
			continue;
		found = 1;

		add = strlen(entry->d_name);
		if (add + 2 >= len - off)
			goto out;
		
		strcpy(path + off, entry->d_name);
		path[off + add] = '/';
		path[off + add + 1] = '\0';
		
		fd = open_pci_config(cmd, path, off + add + 1, len);
		if (fd != -1)
			goto out;
	}

	if (found == 0)
		vlog("cannot find bus %x", cmd->bus);

 out:
	closedir(dir);
 err:
	return fd;
}

int8_t execute_pci_command(const struct pci_command *cmd)
{
	char path[PATH_MAX];
	const char *format = NULL;
	int8_t ret = -1;
	uint64_t val;
	ssize_t len;
	size_t off;
	int fd;

	off = snprintf(path, sizeof (path), "/sys/devices/");
	if (off + 1 >= sizeof (path))
		return -1;

	fd = open_sysdev_config(cmd, path, off, sizeof (path));
	if (fd == -1)
		return -1;

	len = pread(fd, &val, sizeof (val), cmd->offset);
	if (len != sizeof (val)) {
		vlog("cannot read io space: Permission denied");
		goto out;
	}
	
	if (cmd->flags & (PCI_COMMAND_PRINT)) {
		if (cmd->flags & PCI_COMMAND_PRI10)
			format = "%lu";
		else if (cmd->flags & PCI_COMMAND_PRI16)
			format = "%lx";

		printf(format, (val & cmd->mask) >> cmd->shift);
	}

	if (cmd->flags & PCI_COMMAND_WRITE) {
		val &= ~(cmd->mask);
		val |= cmd->value;
		
		len = pwrite(fd, &val, sizeof (val), cmd->offset);
		if (len != sizeof (val)) {
			vlog("cannot write io space: "
			     "Permission denied");
			goto out;
		}
	}

	ret = 0;
 out:
	close(fd);
	return ret;
}
