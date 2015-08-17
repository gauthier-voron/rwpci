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


#include <getopt.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pci.h"


static const char *progname = PROGNAME;
static uint8_t listing = 0;
static uint8_t verbose = 0;


void vlog(const char *format, ...)
{
	va_list ap;

	if (!verbose)
		return;

	fprintf(stderr, "%s: ", progname);
	
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\n");
}

void elog(const char *format, ...)
{
	va_list ap;

	if (!verbose)
		return;

	fprintf(stderr, "%s: ", progname);
	
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, ": ");
	perror("");
}

void error(const char *format, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", progname);
	
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\nPlease type '%s --help' for more informations\n",
		progname);

	exit(EXIT_FAILURE);
}


static void usage(void)
{
	printf("Usage: %s [-h | --help] [-V | --version]\n"
	       "       %s [-l | --list] <key>\n"
	       "       %s [-v | --verbose] <commands...>\n"
	       "Read and write PCI configuration space.\n"
	       "Allow the user to read and write on devices PCI configuration "
	       "space from\n"
	       "command line throught a set of commands. Each command is in "
	       "the following form:\n\n", progname, progname, progname);
	printf("  command ::= [':'[':']] <location> ['=' <value>]\n\n"
	       "The <location> indicates the PCI space location of the data "
	       "to read or write\n"
	       "and is in the following form:\n\n"
	       "  location ::= <bus> '.' <device> '.' <function> '.' <offset> "
	       "['[' <bits> ']']\n\n"
	       "  bits     ::= <bit>\n"
	       "           |   <bit> '-' <bit>\n\n");
	printf("The <bus>, <device> and <function> have the same meaning than "
	       "in the PCI\n"
	       "specification. The user can type '%s --list' or 'lspci' to "
	       "print a list of\n"
	       "the available BDF and their description.\n\n", progname);
	printf("A keyword can also be specified int the listing mode to "
	       "filter devices having\n"
	       "this keyword in their description.\n\n");
	printf("The <offset> indicates the first byte to work with in the "
	       "specified BDF.\n"
	       "Additionally, the user can specify either a <bit> or a bit "
	       "range to work with\n"
	       "starting with this offset. The specified bit cannot be "
	       "greater than 63.\n\n");
	printf("The leading ':' or '::' characters indicate the specified "
	       "bit range has to be\n"
	       "printed, respectively in decimal or hexadecimal form.\n"
	       "The tailing <value> indicates what to be written in the "
	       "configuration space at\n"
	       "the specified location.\n"
	       "If a same command indicates a print and a write, the print "
	       "is performed before\n"
	       "and thus indicates the old value.\n\n");
	printf("Example: in this example, we want to read the device ID and "
	       "vendor ID of our\n"
	       "SATA controller. We start by listing the available "
	       "devices.\n\n"
	       "  > %s --list SATA\n"
	       "  00:1f.2 SATA controller: Interl Corporation 7 Series "
	       "Chipset Family ...\n\n", progname);
	printf("Now we know for each PCI function, the bits 0 to 15 indicate "
	       "the vendor ID and\n"
	       "the bits 16 to 31 indicate the device ID.\n\n"
	       "  > %s ::0.1f.2.0[0-15] ::1.1f.2.0[16-31]\n"
	       "  ::0.1f.2.0[0-15] => 8086\n"
	       "  ::0.1f.2.0[16-31] => 1e03\n\n", progname);
	printf("Options:\n"
	       "  -h, --help                  print this help message then "
	       "exit\n\n"
	       "  -V, --version               print the program version then "
	       "exit\n\n"
	       "  -l, --list <key>            list the available PCI devices "
	       "then exit\n"
	       "                              filter devices mathing the "
	       "<key> if any\n\n"
	       "  -v, --verbose               be more verbose about potential "
	       "errors\n");
}

static void version(void)
{
	printf("%s %s\n%s\n%s\n", PROGNAME, VERSION, AUTHOR, MAILTO);
}


static void parse_options(int *_argc, char *const **_argv)
{
	int c, idx, argc = *_argc;
	char * const*argv = *_argv;
	static struct option options[] = {
		{"help",      no_argument,       0, 'h'},
		{"version",   no_argument,       0, 'V'},
		{"list",      no_argument,       0, 'l'},
		{"verbose",   no_argument,       0, 'v'},
		{ NULL,       0,                 0,  0}
	};

	opterr = 0;

	while (1) {
		c = getopt_long(argc, argv, "h" "V" "l" "v", options, &idx);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'V':
			version();
			exit(EXIT_SUCCESS);
		case 'l':
			listing = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			error("unknown option '%s'", argv[optind-1]);
		}
	}

	*_argc -= optind;
	*_argv += optind;
}

size_t parse_arguments(struct pci_command *dest, int argc, char *const *argv)
{
	size_t num = 0;
	int8_t err;
	int i;

	for (i=0; i<argc; i++) {
		err = parse_pci_command(&dest[num++], argv[i]);
		if (err)
			error("invalid command '%s'", argv[i]);
	}

	return num;
}

int main(int argc, char *const *argv)
{
	struct pci_command *commands;
	size_t i, size;
	uint8_t ret;
	
	progname = argv[0];
	parse_options(&argc, &argv);

	if (listing) {
		if (argc > 0)
			ret = list_pci(argv[0]);
		else
			ret = list_pci(NULL);
		
		if (ret)
			return EXIT_SUCCESS;
		return EXIT_FAILURE;
	}

	commands = alloca(argc * sizeof (struct pci_command));
	
	size = parse_arguments(commands, argc, argv);

	for (i=0; i<size; i++) {
		if (commands[i].flags & PCI_COMMAND_PRINT)
			printf("%s = ", argv[i]);
		ret = execute_pci_command(&commands[i]);
		if (commands[i].flags & PCI_COMMAND_PRINT)
			printf("\n");

		if (ret)
			error("cannot execute '%s'", argv[i]);
	}
	
	return EXIT_SUCCESS;
}
