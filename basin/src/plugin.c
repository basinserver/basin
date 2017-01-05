/*
 * plugin.c
 *
 *  Created on: Jan 3, 2017
 *      Author: root
 */

#include "plugin.h"
#include <dirent.h>
#include "hashmap.h"
#include <stdint.h>
#include <dlfcn.h>
#include "util.h"
#include "xstring.h"

uint32_t next_plugin_id;

void init_plugins() {
	plugins = new_hashmap(1, 1);
	DIR* dir = opendir("plugins/");
	char lp[PATH_MAX];
	if (dir != NULL) {
		struct dirent* de = NULL;
		while ((de = readdir(dir)) != NULL) {
			if (!endsWith_nocase(de->d_name, ".so")) continue;
			snprintf(lp, PATH_MAX, "plugins/%s", de->d_name);
			struct plugin* pl = xcalloc(sizeof(struct plugin));
			pl->hnd = dlopen(lp, RTLD_GLOBAL | RTLD_NOW);
			pl->filename = xstrdup(de->d_name, 0);
			void (*init)(struct plugin* plugin) = dlsym(pl->hnd, "init_plugin");
			(*init)(pl);
			put_hashmap(plugins, next_plugin_id++, pl);
		}
		closedir(dir);
	}

}
