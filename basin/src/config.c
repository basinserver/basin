/*
 * config.c
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "util.h"
#include "config.h"
#include "streams.h"
#include "xstring.h"

struct config* loadConfig(const char* file) {
	if (file == NULL) {
		errno = EBADF;
		return NULL;
	}
	if (access(file, F_OK)) {
		errno = EINVAL;
		return NULL;
	}
	if (access(file, R_OK)) {
		errno = EPERM;
		return NULL;
	}
	int fd = open(file, O_RDONLY);
	if (fd < 0) return NULL;
	struct config* ret = xmalloc(sizeof(struct config));
	ret->node_count = 0;
	ret->nodes = NULL;
	char line[1024];
	int l = 0;
	struct cnode* cat = NULL;
	while (1) {
		l = readLine(fd, line, 1024);
		if (l < 0) break;
		char* wl = trim(line);
		if (wl[0] == 0) continue;
		char* comment = strchr(line, '#');
		if (comment != NULL) {
			comment[0] = 0;
			wl = trim(line);
			if (wl[0] == 0) continue;
		}
		l = strlen(wl);
		if (l > 5 && wl[0] == '[' && wl[l - 1] == ']') {
			wl[--l] = 0;
			wl++;
			char* id = strchr(wl, ' ');
			if (id != NULL) {
				id[0] = 0;
				id++;
				id = trim(id);
			}
			wl = trim(wl);
			cat = xmalloc(sizeof(struct cnode));
			if (ret->node_count == 0) {
				ret->nodes = xmalloc(sizeof(struct cnode*));
				ret->node_count = 1;
			} else {
				ret->node_count++;
				ret->nodes = xrealloc(ret->nodes, sizeof(struct cnode*) * ret->node_count);
			}
			ret->nodes[ret->node_count - 1] = cat;
			cat->keys = NULL;
			cat->values = NULL;
			cat->entries = 0;
			if (streq_nocase(wl, "server")) {
				cat->cat = CAT_SERVER;
			} else if (streq_nocase(wl, "daemon")) {
				cat->cat = CAT_DAEMON;
			} else {
				cat->cat = CAT_UNKNOWN;
			}
			if (id == NULL) {
				cat->id = NULL;
			} else {
				int idl = strlen(id) + 1;
				cat->id = xmalloc(idl);
				memcpy(cat->id, id, idl);
			}
		} else {
			char* value = strchr(wl, '=');
			if (value == NULL) continue;
			value[0] = 0;
			value++;
			value = trim(value);
			wl = trim(wl);
			int wll = strlen(wl);
			int vl = strlen(value);
			if (cat->entries == 0) {
				cat->keys = xmalloc(sizeof(char*));
				cat->values = xmalloc(sizeof(char*));
				cat->entries = 1;
			} else {
				cat->keys = xrealloc(cat->keys, ++cat->entries * sizeof(char*));
				cat->values = xrealloc(cat->values, cat->entries * sizeof(char*));
			}
			cat->keys[cat->entries - 1] = xmalloc(wll + 1);
			cat->values[cat->entries - 1] = xmalloc(vl + 1);
			memcpy(cat->keys[cat->entries - 1], wl, wll + 1);
			memcpy(cat->values[cat->entries - 1], value, vl + 1);
		}
	}
	close(fd);
	return ret;
}

const char* getConfigValue(const struct cnode* cat, const char* name) {
	if (cat == NULL || name == NULL || cat->entries == 0) return NULL;
	for (int i = 0; i < cat->entries; i++) {
		if (streq_nocase(cat->keys[i], name)) {
			return cat->values[i];
		}
	}
	return NULL;
}

int hasConfigKey(const struct cnode* cat, const char* name) {
	if (cat == NULL || name == NULL || cat->entries == 0) return NULL;
	for (int i = 0; i < cat->entries; i++) {
		if (streq_nocase(cat->keys[i], name)) {
			return 1;
		}
	}
	return 0;
}

struct cnode* getCatByID(const struct config* cfg, const char* id) {
	if (cfg == NULL || id == NULL || cfg->node_count == 0) return NULL;
	for (int i = 0; i < cfg->node_count; i++) {
		if (streq_nocase(cfg->nodes[i]->id, id)) {
			return cfg->nodes[i];
		}
	}
	return NULL;
}

struct cnode** getCatsByCat(const struct config* cfg, int cat, int* count) {
	if (cfg == NULL || cat < CAT_UNKNOWN || cfg->node_count == 0) return NULL;
	if (cat == CAT_UNKNOWN) return cfg->nodes;
	int rs = 0;
	struct cnode** ret = NULL;
	for (int i = 0; i < cfg->node_count; i++) {
		if (cfg->nodes[i]->cat == cat) {
			if (rs == 0) {
				rs = 1;
				ret = xmalloc(sizeof(struct cnode**));
			} else {
				rs++;
				ret = xrealloc(ret, sizeof(struct cnode**) * rs);
			}
			ret[rs - 1] = cfg->nodes[i];
		}
	}
	*count = rs;
	return ret;
}

struct cnode* getUniqueByCat(const struct config* cfg, int cat) {
	if (cfg == NULL || cat <= CAT_UNKNOWN || cfg->node_count == 0) return NULL;
	for (int i = 0; i < cfg->node_count; i++) {
		if (cfg->nodes[i]->cat == cat && cfg->nodes[i]->id == NULL) {
			return cfg->nodes[i];
		}
	}
	return NULL;
}
