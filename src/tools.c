/*
 * craft.c
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#include <basin/globals.h>
#include <basin/network.h>
#include <basin/tools.h>
#include <basin/block.h>
#include <basin/item.h>
#include <basin/player.h>
#include <avuna/json.h>
#include <avuna/string.h>
#include <avuna/util.h>
#include <avuna/llist.h>
#include <avuna/log.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

struct mempool* tool_pool;
struct hashmap* tool_infos;

struct tool_info* tools_get(char* name) {
    size_t name_len = strlen(name) + 1;
      char name_lowered[name_len];
      memcpy(name_lowered, name, name_len);
      str_tolower(name_lowered);
      return hashmap_get(tool_infos, name_lowered);
}

int tools_proficient(struct tool_info* info, uint8_t harvest_level, block b) {
    if (harvest_level < 0 || info == NULL) return 0;
    if (harvest_level > info->proficient_size) harvest_level = (uint8_t) (info->proficient_size - 1);
    for (size_t x = 0; x <= harvest_level; x++) {
        for (size_t z = 0; z < info->proficiencies[x].proficient_size; z++) {
            struct tool_proficiency* proficiency = &info->proficiencies[x];
              if (proficiency == NULL) continue;
          for (size_t i = 0; i < proficiency->proficient_size; i++) {
            if ((proficiency->proficient[i] >> 4) == (b >> 4)) return 1;
        }
    }
}
  return 0;
}

void tools_init() {
    tool_pool = mempool_new();
    char* json_file = (char*) read_file_fully(tool_pool, "tools.json", NULL);
    if (json_file == NULL) {
        errlog(delog, "Error reading tool data: %s\n", strerror(errno));
        return;
    }
    struct json_object* json = NULL;
        json_parse(tool_pool, &json, json_file);
    pprefree(tool_pool, json_file);
    if (json == NULL) {
        errlog(delog, "Syntax error in 'tools.json'");
        return;
    }
    if (json->type != JSON_OBJECT) {
        errlog(delog, "Format error in 'tools.json'");
        return;
    }
    ITER_LLIST(json->children_list, value) {
        struct json_object* child = value;
        if (child->type != JSON_OBJECT) {
            goto format_error;
        }
        struct tool_info* info = pcalloc(tool_pool, sizeof(struct tool_info));
        info->name = str_tolower(pxfer(json->pool, tool_pool, child->name));
        info->proficiencies = pmalloc(tool_pool, sizeof(struct tool_proficiency) * child->children_list->size);
        info->proficient_size = child->children_list->size;
        size_t proficiency_index = 0;
        ITER_LLIST(child->children_list, sub_value) {
            struct json_object* sub_child = sub_value;
            if (sub_child == NULL || sub_child->type != JSON_ARRAY) goto format_error;
            info->proficiencies[proficiency_index].proficient = pmalloc(tool_pool, sizeof(block) * sub_child->children_list->size);
            info->proficiencies[proficiency_index].proficient_size = sub_child->children_list->size;
            size_t entry_index = 0;
            ITER_LLIST(sub_child->children_list, sub_value2) {
                struct json_object* sub_child2 = sub_value2;
                   if (sub_child2 == NULL || sub_child2->type != JSON_NUMBER) goto format_error;
                info->proficiencies[proficiency_index].proficient[entry_index] = (block) sub_child2->data.number;
                ++entry_index;
                ITER_LLIST_END();
              }
            ++proficiency_index;
            ITER_LLIST_END();
        }
        tools_add(info);
        continue;
        format_error: ;
        errlog(delog, "[WARNING] Error Loading Tool \"%s\"! Skipped.\n", child->name);
        ITER_LLIST_END();
    }
    pfree(json->pool);
}

void tools_add(struct tool_info* tool) {
    hashmap_put(tool_infos, tool->name, tool);
}
