/*
 * tools.h
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#include "collection.h"
#include "network.h"
#include "item.h"
#include "world.h"

struct tool_proficiency {
		uint16_t* proficient;
		size_t proficient_size;
};

struct tool_info {
		char* name;
		struct tool_proficiency* proficient_hl;
		size_t proficient_size;
};

void init_tools();

struct tool_info* getToolInfo(char* name);

int isToolProficient(struct tool_info* ti, uint8_t harvest_level, uint16_t blk);

void add_tool(struct tool_info* tool);

#endif /* TOOLS_H_ */
