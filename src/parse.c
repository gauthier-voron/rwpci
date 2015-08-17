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


#include <stdint.h>
#include <stdlib.h>

#include "pci.h"


static uint8_t parse_print_flag(const char **rem, const char *str)
{
	uint8_t flag = 0;
	
	if (*str == ':') {
		str++;
		flag = PCI_COMMAND_PRI10;
	}

	if (*str == ':') {
		str++;
		flag = PCI_COMMAND_PRI16;
	}

	if (rem)
		*rem = str;
	return flag;
}

static uint64_t parse_mask(const char **rem, const char *str)
{
	uint64_t mask = (uint64_t) -1;
	uint64_t i, start, end;
	char *err = (char *) str - 1;

	if (*str != '[')
		goto out;

	str++;
	mask = 0;

	start = strtol(str, &err, 10);
	if (start > 63)
		goto err;

	if (*err == '-') {
		str = err + 1;
		end = strtol(str, &err, 10);
		if (end > 63)
			goto err;

		if (start < end)
			for (i=start; i<=end; i++)
				mask |= (1ul << i);
		else
			for (i=end; i<=start; i++)
				mask |= (1ul << i);
	} else {
		mask |= (1ul << start);
	}

	if (*err == ']')
		goto out;

 err:
	if (rem)
		*rem = NULL;
	return 0;
 out:
	if (rem)
		*rem = err + 1;
	return mask;
}


static uint8_t parse_pci_id8(const char **rem, const char *str)
{
	uint64_t id;
	char *end;

	id = strtol(str, &end, 16);
	if (id > 255)
		end = NULL;
	if (rem)
		*rem = end;
	return id;
}

static uint16_t parse_pci_id16(const char **rem, const char *str)
{
	uint64_t id;
	char *end;

	id = strtol(str, &end, 16);
	if (id > 65535)
		end = NULL;
	if (rem)
		*rem = end;
	return id;
}


static uint64_t parse_value(const char **rem, const char *str)
{
	uint64_t value = 0;
	uint8_t base = 10;
	char *end;

	if (*str == '=') {
		str++;

		if (*str == 'x') {
			base = 16;
			str++;
		}

		if (*str == '0' && *(str+1) == 'x') {
			base = 16;
			str += 2;
		}
		
		value = strtol(str, &end, base);
		str = end;
	}

	if (rem)
		*rem = str;
	return value;
}


int8_t parse_pci_command(struct pci_command *dest, const char *str)
{
	const char *err;
	uint64_t shift;
	
	dest->flags = parse_print_flag(&str, str);
	
	dest->bus = parse_pci_id8(&str, str);
	if (!str || *str++ != '.')
		return -1;

	dest->device = parse_pci_id8(&str, str);
	if (!str || *str++ != '.')
		return -1;

	dest->function = parse_pci_id8(&str, str);
	if (!str || *str++ != '.')
		return -1;

	dest->offset = parse_pci_id16(&str, str);
	if (!str)
		return -1;

	dest->mask = parse_mask(&str, str);
	if (!str)
		return -1;
	shift = 0;
	while (((dest->mask >> shift) & 1) == 0)
		shift++;
	dest->shift = shift;

	dest->value = parse_value(&err, str);
	if (err != str) {
		dest->value <<= shift;
		if ((dest->value & dest->mask) != dest->value)
			return -1;
		
		dest->flags |= PCI_COMMAND_WRITE;
		str = err;
	}

	if (*str == '\0')
		return 0;
	return -1;
}
