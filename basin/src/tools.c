/*
 * craft.c
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#include "tools.h"
#include "network.h"
#include "block.h"
#include "item.h"
#include <fcntl.h>
#include "xstring.h"
#include <errno.h>
#include "util.h"
#include "json.h"
#include "player.h"

struct collection* tool_infos;

struct tool_info* getToolInfo(char* name) {
	for (size_t i = 0; i < tool_infos->size; i++) {
		struct tool_info* ti = (struct tool_info*) tool_infos->data[i];
		if (streq_nocase(ti->name, name)) return ti;
	}
	return NULL;
}

int isToolProficient(struct tool_info* ti, uint8_t harvest_level, block blk) {
	if (harvest_level < 0 || ti == NULL) return 0;
	if (harvest_level > ti->proficient_size) harvest_level = ti->proficient_size - 1;
	for (size_t x = 0; x <= harvest_level; x++) {
		for (size_t z = 0; z < ti->proficient_hl[x].proficient_size; z++) {
			struct tool_proficiency* tp = (struct tool_proficiency*) &ti->proficient_hl[x];
			if (tp == NULL) continue;
			for (size_t i = 0; i < tp->proficient_size; i++) {
				if ((tp->proficient[i] >> 4) == (blk >> 4)) return 1;
			}
		}
	}
	return 0;
}

void init_tools() {
	tool_infos = new_collection(8, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("tools.json", O_RDONLY);
	ssize_t r = 0;
	while ((r = read(fd, jsf + jsr, jsfs - jsr)) > 0) {
		jsr += r;
		if (jsfs - jsr < 512) {
			jsfs += 4096;
			jsf = xrealloc(jsf, jsfs + 1);
		}
	}
	jsf[jsr] = 0;
	if (r < 0) {
		printf("Error reading tool data: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
	parseJSON(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct tool_info* cr = xcalloc(sizeof(struct tool_info));
		cr->name = xstrdup(ur->name, 0);
		cr->proficient_hl = xmalloc(sizeof(struct tool_proficiency) * ur->child_count);
		cr->proficient_size = ur->child_count;
		for (size_t x = 0; x < ur->child_count; x++) {
			struct json_object* ir = ur->children[x];
			if (ir == NULL || ir->type != JSON_ARRAY) goto cerr;
			cr->proficient_hl[x].proficient = xmalloc(sizeof(block) * ir->child_count);
			cr->proficient_hl[x].proficient_size = ir->child_count;
			for (size_t z = 0; z < ir->child_count; z++) {
				struct json_object* er = ir->children[z];
				if (er == NULL || er->type != JSON_NUMBER) goto cerr;
				cr->proficient_hl[x].proficient[z] = (block) er->data.number;
			}
		}
		add_tool(cr);
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Tool \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
}

void add_tool(struct tool_info* tool) {
	add_collection(tool_infos, tool);
}
