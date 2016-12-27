/*
 * json.h
 *
 *  Created on: May 20, 2016
 *      Author: root
 */

#ifndef JSON_H_
#define JSON_H_

#include <stdint.h>
#include <stdlib.h>

#define JSON_STRING 0
#define JSON_NUMBER 1
#define JSON_OBJECT 2
#define JSON_ARRAY 3
#define JSON_TRUE 4
#define JSON_FALSE 5
#define JSON_NULL 6
#define JSON_EOA 7

union json_data {
		char* string;
		double number;
		struct json_object* object;
};

struct json_object {
		char* name;
		uint8_t type;
		union json_data data;
		struct json_object** children;
		size_t child_count;
};

ssize_t parseJSON(struct json_object* root, char* json);

void freeJSON(struct json_object* root);

struct json_object* getJSONValue(struct json_object* parent, char* name);

#endif /* JSON_H_ */
