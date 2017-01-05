/*
 * json.c
 *
 *  Created on: May 20, 2016
 *      Author: root
 */

#include "xstring.h"
#include "json.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "util.h"

char* __readJSONString(char* json, size_t* i) {
	int cs = 0;
	size_t sl = strlen(json);
	char* ret = NULL;
	size_t rs = 0;
	for (; *i < sl; (*i)++) {
		if (cs == 0) {
			if (json[*i] == '\"') cs = 1;
		} else if (cs == 1) {
			if (json[*i] == '\"') {
				errno = 0;
				ret[rs] = 0;
				return ret;
			} else if (json[*i] == '\\') {
				(*i)++;
				if (*i >= sl) continue;
				if (ret == NULL) {
					ret = xmalloc(2);
					rs = 0;
				} else ret = xrealloc(ret, rs + 2);
				if (json[*i] == '\"') ret[rs++] = '\"';
				else if (json[*i] == '\\') ret[rs++] = '\\';
				else if (json[*i] == '/') ret[rs++] = '/';
				else if (json[*i] == '\b') ret[rs++] = '\b';
				else if (json[*i] == '\f') ret[rs++] = '\f';
				else if (json[*i] == '\n') ret[rs++] = '\n';
				else if (json[*i] == '\r') ret[rs++] = '\r';
				else if (json[*i] == '\t') ret[rs++] = '\t';
				else if (json[*i] == 'u') {
					(*i)--;
					goto uce;
				}
				ret[rs] = 0;
				continue;
				uce: ;
			}
			if (ret == NULL) {
				ret = xmalloc(2);
				rs = 0;
			} else ret = xrealloc(ret, rs + 2);
			ret[rs++] = json[*i];
			ret[rs] = 0;
		}
	}
	if (ret == NULL) errno = EINVAL;
	else ret[rs] = 0;
	return ret;
}

int __recurJSON(struct json_object* cur, size_t* i, char* json);

int __readJSONValue(char* json, size_t* i, struct json_object* into) {
	size_t sl = strlen(json);
	into->children = NULL;
	into->child_count = 0;
	into->data.string = NULL;
	for (; *i < sl; (*i)++) {
		if (json[*i] == '\"') {
			into->type = JSON_STRING;
			into->data.string = __readJSONString(json, i);
		} else if (json[*i] == '-' || (json[*i] >= '0' && json[*i] <= '9')) {
			into->type = JSON_NUMBER;
			char* tp = NULL;
			into->data.number = strtod(json + *i, &tp);
			*i = tp - json - 1;
		} else if (json[*i] == '{') {
			into->type = JSON_OBJECT;
			if (__recurJSON(into, i, json)) return -1;
		} else if (json[*i] == '[') {
			into->type = JSON_ARRAY;
			int cs = 0;
			size_t sl = strlen(json);
			for (; *i < sl; (*i)++) {
				if (cs == 0) {
					if (json[*i] == '[') {
						cs = 1;
					}
				} else if (cs == 1) {
					struct json_object* sj = xmalloc(sizeof(struct json_object));
					sj->name = NULL;
					if (__readJSONValue(json, i, sj)) {
						xfree(sj);
						return -1;
					}
					if (sj->type == JSON_EOA) {
						xfree(sj);
						break;
					} else {
						if (into->children == NULL) {
							into->child_count = 0;
							into->children = xmalloc(sizeof(struct json_object*));
						} else {
							into->children = xrealloc(into->children, sizeof(struct json_object*) * (into->child_count + 1));
						}
						into->children[into->child_count++] = sj;
						cs = 2;
					}
				} else if (cs == 2) {
					if (json[*i] == ',') cs = 1;
					else if (json[*i] == ']') break;
				}
			}
		} else if (startsWith_nocase(json + *i, "true")) {
			(*i) += 3;
			into->type = JSON_TRUE;
		} else if (startsWith_nocase(json + *i, "false")) {
			(*i) += 4;
			into->type = JSON_FALSE;
		} else if (startsWith_nocase(json + *i, "null")) {
			(*i) += 3;
			into->type = JSON_NULL;
		} else if (json[*i] == ']') {
			into->type = JSON_EOA;
		} else continue;
		break;
	}
	return 0;
}

int __recurJSON(struct json_object* cur, size_t* i, char* json) {
	size_t sl = strlen(json);
	int cs = 0;
	struct json_object* nchild = NULL;
	char ck = cur->type == JSON_OBJECT ? '}' : ']';
	for (; *i < sl; (*i)++) {
		if (cs == 0) {
			if (json[*i] == '{') cs = 1;
		} else if (cs == 1) {
			if (json[*i] == ck) break;
			else if (json[*i] == '\"') {
				nchild = xmalloc(sizeof(struct json_object));
				memset(nchild, 0, sizeof(struct json_object));
				nchild->name = __readJSONString(json, i);
				if (nchild->name == NULL) {
					xfree(nchild);
					errno = EINVAL;
					return -1;
				}
				cs = 2;
			}
		} else if (cs == 2) {
			if (json[*i] == ':') cs = 3;
		} else if (cs == 3) {
			if (__readJSONValue(json, i, nchild)) {
				if (nchild->name != NULL) xfree(nchild->name);
				xfree(nchild);
				return -1;
			}
			if (cur->children == NULL) {
				cur->child_count = 0;
				cur->children = xmalloc(sizeof(struct json_object*));
			} else {
				cur->children = xrealloc(cur->children, sizeof(struct json_object*) * (cur->child_count + 1));
			}
			cur->children[cur->child_count++] = nchild;
			nchild = NULL;
			cs = 4;
		} else if (cs == 4) {
			if (json[*i] == ',') cs = 1;
			else if (json[*i] == ck) break;
		}
	}
	if (nchild != NULL) {
		if (nchild->name != NULL) xfree(nchild->name);
		xfree(nchild);
	}
	return 0;
}

ssize_t parseJSON(struct json_object* root, char* json) {
	if (strlen(json) < 2 || (json[0] != '{' && json[0] != '[')) {
		memset(root, 0, sizeof(struct json_object));
		errno = EINVAL;
		return -1;
	}
	root->name = NULL;
	root->type = json[0] == '{' ? JSON_OBJECT : JSON_ARRAY;
	root->children = NULL;
	root->child_count = 0;
	root->data.string = NULL;
	size_t it = 0;
	__recurJSON(root, &it, json);
	return it;
}

void freeJSON(struct json_object* root) {
	if (root->name != NULL) {
		xfree(root->name);
		root->name = NULL;
	}
	if (root->type == JSON_STRING && root->data.string != NULL) {
		xfree(root->data.string);
		root->data.string = NULL;
	} else if (root->type == JSON_OBJECT && root->children != NULL) {
		for (size_t i = 0; i < root->child_count; i++) {
			if (root->children[i] != NULL) {
				freeJSON(root->children[i]);
				xfree(root->children[i]);
				root->children[i] = NULL;
			}
		}
		if (root->children != NULL) {
			xfree(root->children);
			root->children = NULL;
		}
	}
}

struct json_object* getJSONValue(struct json_object* parent, char* name) {
	if (parent->children == NULL) return NULL;
	for (size_t i = 0; i < parent->child_count; i++) {
		if (parent->children[i] != NULL && streq_nocase(parent->children[i]->name, name)) {
			return parent->children[i];
		}
	}
	return NULL;
}
