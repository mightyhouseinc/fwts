/*
 * Copyright (C) 2010 Canonical
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "fwts.h"

static char *osilinux_headline(void)
{
	return "Disassemble DSDT to check for _OSI(\"Linux\")";
}

static fwts_list* disassembly;

static int osilinux_init(fwts_framework *fw)
{
	struct stat buffer;

	if (fwts_check_root_euid(fw))
		return 1;

        if (stat(fw->iasl ? fw->iasl : IASL, &buffer)) {
                fwts_log_error(fw, "Make sure iasl is installed");
                return 1;
        }

	disassembly = fwts_iasl_disassemble(fw, "DSDT", 0);
	if (disassembly == NULL) {
		fwts_log_error(fw, "Cannot disassemble with iasl");
		return 1;
	}

	return 0;
}

static int osilinux_deinit(fwts_framework *fw)
{
	if (disassembly)
		fwts_text_list_free(disassembly);

	return 0;
}

static int osilinux_test1(fwts_framework *fw)
{	
	fwts_list_element *item;
	fwts_list_element *dumpitem = NULL;
	int depth = 0;
	int dumpdepth = 0;
	int found = 0;

	if (disassembly == NULL)
		return 1;

	fwts_log_info(fw, 
		"Disassemble DSDT to check for _OSI(\"Linux\")\n"
		"This is not strictly a failure mode, it just alerts\n"
		"one that this has been defined in the DSDT and probably\n"
		"should be avoided since the Linux ACPI driver matches\n"
		"onto the Windows _OSI strings");

	for (item = disassembly->head; item != NULL; item = item->next) {
		char *line = fwts_text_list_text(item);
		char *str;

		if (strstr(line, "{")) {
			depth++;
			if (dumpdepth == 0)
				dumpitem = item;
		}

		if (strstr(line, "}")) {
			depth--;
			if (dumpdepth != 0 && dumpdepth != depth) {
				fwts_log_warning(fw, "WARNING: Found _OSI(\"Linux\")");
				found++;
				while (dumpitem != NULL && dumpitem != item->next) {
					fwts_log_warning(fw, fwts_text_list_text(dumpitem));
					dumpitem = dumpitem->next;
				}
				dumpdepth = 0;
				fwts_warning(fw, "DSDT implements a deprecated _OSI(\"Linux\") test");
			}
		}
		if ((str = strstr(line, "_OSI")) != NULL) {
			if (strstr(str + 4, "Linux") || strstr(str + 4, "linux"))
				dumpdepth = depth;
		}
	}

	if (!found)
		fwts_passed(fw, "DSDT does not implement a deprecated _OSI(\"Linux\") test");

	return 0;
}

static fwts_framework_tests osilinux_tests[] = {
	osilinux_test1,
	NULL
};

static fwts_framework_ops osilinux_ops = {
	osilinux_headline,
	osilinux_init,	
	osilinux_deinit,
	osilinux_tests
};

FRAMEWORK(osilinux, &osilinux_ops, TEST_ANYTIME);
