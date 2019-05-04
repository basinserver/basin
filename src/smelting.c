#include <basin/smelting.h>
#include <basin/item.h>
#include <basin/globals.h>
#include <avuna/hash.h>
#include <avuna/list.h>
#include <avuna/json.h>
#include <avuna/string.h>
#include <avuna/pmem.h>
#include <avuna/util.h>
#include <avuna/log.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

struct hashmap* smelting_fuels; // key = id << 16 | data

struct list* smelting_recipes;

struct mempool* smelting_pool;

int16_t smelting_burnTime(struct slot* slot) {
    if (slot == NULL || slot->item < 0) return 0;
    uint32_t key = (uint32_t) slot->item << 16u | (uint32_t) slot->damage;
    struct smelting_fuel* fuel = hashmap_getint(smelting_fuels, key);
    if (fuel == NULL) {
        key &= 0xFFFFlu << 16;
        key |= 0xFFFF;
        fuel = hashmap_getint(smelting_fuels, key);
    }
    if (fuel == NULL) {
        return 0;
    }
    return fuel->burn_time;
}

struct slot* smelting_output(struct slot* input) {
    // TODO: use a hashmap
    if (input == NULL || input->item < 0) return 0;
    pthread_rwlock_init(&smelting_fuels->rwlock);
    for (size_t i = 0; i < smelting_recipes->count; i++) {
        if (smelting_recipes->data[i] == NULL) continue;
        struct smelting_recipe* sf = (struct smelting_recipe*) smelting_recipes->data[i];
        if (slot_stackable_damage_ignore(input, &sf->input)) {
            pthread_rwlock_unlock(&smelting_fuels->rwlock);
            return &sf->output;
        }
    }
    pthread_rwlock_unlock(&smelting_fuels->rwlock);
    return 0;
}

void add_smelting_fuel(struct smelting_fuel* fuel) {
    uint32_t key = (uint32_t) fuel->id << 16u | (uint32_t) fuel->damage;
    hashmap_putint(smelting_fuels, key, fuel);
}

void add_smelting_recipe(struct smelting_recipe* recipe) {
    list_append(smelting_recipes, recipe);
}

void init_smelting() {
    smelting_pool = mempool_new();
    smelting_fuels = hashmap_thread_new(64, smelting_pool);
    smelting_recipes = list_thread_new(128, smelting_pool);
    char* json_file = (char*) read_file_fully(smelting_pool, "smelting.json", NULL);
    if (json_file == NULL) {
        errlog(delog, "Error reading smelting data: %s\n", strerror(errno));
        return;
    }
    struct json_object* json = NULL;
    json_parse(smelting_pool, &json, json_file);
    pprefree(smelting_pool, json_file);
    struct json_object* fuels = json_get(json, "fuels");
    if (fuels == NULL || fuels->type != JSON_OBJECT) goto format_error_json;
    struct json_object* recipes = json_get(json, "recipes");
    if (recipes == NULL || recipes->type != JSON_OBJECT) goto format_error_json;
    ITER_LLIST(fuels->children_list, value) {
        struct json_object* child_json = value;
        struct json_object* id_json = json_get(child_json, "id");
        if (id_json == NULL || id_json->type != JSON_NUMBER) {
            goto fuel_format_error;
        }
        struct smelting_fuel* fuel = pcalloc(smelting_pool, sizeof(struct smelting_fuel));
        fuel->id = (int16_t) id_json->data.number;
        if (fuel->id <= 0) {
            goto fuel_format_error;
        }
        struct json_object* damage_json = json_get(child_json, "damage");
        if (damage_json == NULL || damage_json->type != JSON_NUMBER) {
            goto fuel_format_error;
        }
        fuel->damage = (int16_t) damage_json->data.number;
        struct json_object* burn_time_json = json_get(child_json, "burnTime");
        if (burn_time_json == NULL || burn_time_json->type != JSON_NUMBER) {
            goto fuel_format_error;
        }
        fuel->burn_time = (int16_t) burn_time_json->data.number;
        add_smelting_fuel(fuel);
        continue;
        fuel_format_error:;
        printf("[WARNING] Error Loading Smelting Fuel \"%s\"! Skipped.\n", child_json->name);
        ITER_LLIST_END();
    }
    ITER_LLIST(recipes->children_list, value) {
        struct json_object* child_json = value;
        struct smelting_recipe* recipe = pcalloc(smelting_pool, sizeof(struct smelting_recipe));
        struct json_object* input_json = json_get(child_json, "input_item");
        if (input_json == NULL || input_json->type != JSON_NUMBER) {
            goto format_error_recipe;
        }
        recipe->input.item = (int16_t) input_json->data.number;
        if (recipe->input.item <= 0) goto format_error_recipe;
        struct json_object* input_damage_json = json_get(child_json, "input_damage");
        if (input_damage_json == NULL || input_damage_json->type != JSON_NUMBER) {
            goto format_error_recipe;
        }
        recipe->input.damage = (int16_t) input_damage_json->data.number;
        recipe->input.count = 1;
        recipe->input.nbt = NULL;
        struct json_object* output_json = json_get(child_json, "output_item");
        if (output_json == NULL || output_json->type != JSON_NUMBER) {
            goto format_error_recipe;
        }
        recipe->output.item = (int16_t) output_json->data.number;
        if (recipe->output.item <= 0) {
            goto format_error_recipe;
        }
        struct json_object* output_damage_json = json_get(child_json, "output_damage");
        if (output_damage_json == NULL || output_damage_json->type != JSON_NUMBER) {
            goto format_error_recipe;
        }
        recipe->output.damage = (int16_t) output_damage_json->data.number;
        struct json_object* output_count_json = json_get(child_json, "output_count");
        if (output_count_json == NULL || output_count_json->type != JSON_NUMBER) {
            goto format_error_recipe;
        }
        recipe->output.count = (uint8_t) output_count_json->data.number;
        recipe->output.nbt = NULL;
        add_smelting_recipe(recipe);
        continue;
        format_error_recipe:;
        printf("[WARNING] Error Loading Smelting Recipe \"%s\"! Skipped.\n", child_json->name);
        ITER_LLIST_END();
    }
    pfree(json->pool);
    return;
    format_error_json: ;
    pfree(json->pool);
    printf("[ERROR] Critical Failure Loading 'smelting.json', No Fuels/Recipes Object!\n");
}
