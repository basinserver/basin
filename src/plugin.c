#include <basin/plugin.h>
#include <avuna/hash.h>
#include <avuna/string.h>
#include <avuna/pmem.h>
#include <dirent.h>
#include <stdint.h>
#include <dlfcn.h>
#include <errno.h>
#include <jni.h>

uint32_t next_plugin_id;

struct mempool* plugin_pool;

void init_plugins() {
    plugin_pool = mempool_new();
    plugins = hashmap_new(16, plugin_pool);
    DIR* dir = opendir("plugins/");
    char lp[PATH_MAX];
    uint32_t lua_count = 0;
    char** lua_paths = NULL;
    if (dir != NULL) {
        struct dirent* de = NULL;
        while ((de = readdir(dir)) != NULL) {
            snprintf(lp, PATH_MAX, "plugins/%s", de->d_name);
            if (str_suffixes(de->d_name, ".so")) { // BASIN C
                struct plugin* pl = xcalloc(sizeof(struct plugin));
                pl->hnd = dlopen(lp, RTLD_GLOBAL | RTLD_NOW);
                if (pl->hnd == NULL) {
                    printf("Error loading plugin! %s\n", dlerror());
                    free(pl);
                    continue;
                }
                pl->filename = xstrdup(de->d_name, 0);
                void (*init)(struct plugin* plugin) = dlsym(pl->hnd, "init_plugin");
                (*init)(pl);
                pl->type = PLUGIN_BASIN;
                put_hashmap(plugins, next_plugin_id++, pl);
            } else if (str_suffixes(de->d_name, ".jar")) { // BUKKIT
                struct plugin* pl = xcalloc(sizeof(struct plugin));
                pl->type = PLUGIN_BUKKIT;
                char* cp = xmalloc(strlen(lp) + strlen("-Djava.class.path=bukkit.jar:basinBukkit.jar:") + 1);
                strcpy(cp, "-Djava.class.path=bukkit.jar:basinBukkit.jar:"); // len checks done above
                strcat(cp, lp);
                JavaVMOption jvmopt[1];
                jvmopt[0].optionString = cp;

                JavaVMInitArgs vmArgs;
                vmArgs.version = JNI_VERSION_1_2;
                vmArgs.nOptions = 1;
                vmArgs.options = jvmopt;
                vmArgs.ignoreUnrecognized = JNI_TRUE;
                long flag = JNI_CreateJavaVM(&pl->javaVM, (void**) &pl->jniEnv, &vmArgs);
                if (flag == JNI_ERR) {
                    printf("Error creating Java VM, Bukkit plugins will not be loaded.\n");
                    free(pl);
                    free(cp);
                    continue;
                }
                jclass loadHelper = (*pl->jniEnv)->FindClass(pl->jniEnv, "org.basin.bukkit.LoadHelper");
                if (loadHelper == NULL) {
                    (*pl->jniEnv)->ExceptionDescribe(pl->jniEnv);
                    (*pl->javaVM)->DestroyJavaVM(pl->javaVM);
                    free(pl);
                    free(cp);
                    continue;
                }
                jmethodID loadMainYaml = (*pl->jniEnv)->GetStaticMethodID(pl->jniEnv, loadHelper, "loadMainYaml", "()V");
                (*pl->jniEnv)->CallStaticVoidMethod(pl->jniEnv, loadHelper, loadMainYaml);
                if ((*pl->jniEnv)->ExceptionCheck(pl->jniEnv)) {
                    (*pl->jniEnv)->ExceptionDescribe(pl->jniEnv);
                    (*pl->jniEnv)->ExceptionClear(pl->jniEnv);
                }
                pl->filename = xstrdup(de->d_name, 0);
                put_hashmap(plugins, next_plugin_id++, pl);
            } else if (endsWith_nocase(de->d_name, ".zip")) { // LUA
                lua_paths = xrealloc(lua_paths, (lua_count + 1) * sizeof(char*));
                lua_paths[lua_count++] = xstrdup(lp, 0);
            }
        }
        closedir(dir);
    }
    if (lua_count > 0) {
        //TODO: create LUA VM
    }
}
