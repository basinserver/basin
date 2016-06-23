/*
 * config.h
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define CAT_UNKNOWN -1
#define CAT_SERVER 0
#define CAT_DAEMON 1

struct cnode {
		int cat;
		char* id;
		int entries;
		char** keys;
		char** values;
};

struct config {
		int node_count;
		struct cnode** nodes;
};

struct config* loadConfig(const char* file);

const char* getConfigValue(const struct cnode* cat, const char* name);

int hasConfigKey(const struct cnode* cat, const char* name);

struct cnode* getCatByID(const struct config* cfg, const char* name);

struct cnode** getCatsByCat(const struct config* cfg, int cat, int* count);

struct cnode* getUniqueByCat(const struct config* cfg, int cat);

#endif /* CONFIG_H_ */
