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


#ifndef PCI_H
#define PCI_H


#include <stdint.h>


void vlog(const char *format, ...);
void elog(const char *format, ...);


#define PCI_COMMAND_PRI10   (1 << 0)
#define PCI_COMMAND_PRI16   (1 << 1)
#define PCI_COMMAND_PRI10   (1 << 0)
#define PCI_COMMAND_PRINT   (PCI_COMMAND_PRI10 | PCI_COMMAND_PRI16)
#define PCI_COMMAND_WRITE   (1 << 2)

struct pci_command
{
	uint8_t   flags;
	uint8_t   bus;
	uint8_t   device;
	uint8_t   function;
	uint16_t  offset;
	uint64_t  mask;
	uint8_t   shift;
	uint64_t  value;
};


uint8_t list_pci(const char *key);

int8_t parse_pci_command(struct pci_command *dest, const char *str);

int8_t execute_pci_command(const struct pci_command *dest);


#endif
