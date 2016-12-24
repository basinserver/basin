/*
 * xstring.c
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "util.h"

char* trim(char* str) {
	if (str == NULL) return NULL;
	size_t len = strlen(str);
	for (int i = len - 1; i >= 0; i--) {
		if (isspace(str[i])) {
			str[i] = 0;
		} else break;
	}
	for (int i = 0; i < len; i++) {
		if (!isspace(str[i])) return str + i;
	}
	return str + len;
}

int streq(const char* str1, const char* str2) {
	if (str1 == NULL || str2 == NULL) return 0;
	if (str1 == str2) return 1;
	size_t l1 = strlen(str1);
	size_t l2 = strlen(str2);
	if (l1 != l2) return 0;
	for (int i = 0; i < l1; i++) {
		if (str1[i] != str2[i]) {
			return 0;
		}
	}
	return 1;
}

int streq_nocase(const char* str1, const char* str2) {
	if (str1 == NULL || str2 == NULL) return 0;
	if (str1 == str2) return 1;
	size_t l1 = strlen(str1);
	size_t l2 = strlen(str2);
	if (l1 != l2) return 0;
	for (int i = 0; i < l1; i++) {
		char s1 = str1[i];
		if (s1 >= 'A' && s1 <= 'Z') s1 += ' ';
		char s2 = str2[i];
		if (s2 >= 'A' && s2 <= 'Z') s2 += ' ';
		if (s1 != s2) {
			return 0;
		}
	}
	return 1;
}

int startsWith(const char* str, const char* with) {
	if (str == NULL || with == NULL) return 0;
	if (str == with) return 1;
	size_t l1 = strlen(str);
	size_t l2 = strlen(with);
	if (l1 < l2) return 0;
	for (int i = 0; i < l2; i++) {
		if (str[i] != with[i]) {
			return 0;
		}
	}
	return 1;
}

int startsWith_nocase(const char* str, const char* with) {
	if (str == NULL || with == NULL) return 0;
	if (str == with) return 1;
	size_t l1 = strlen(str);
	size_t l2 = strlen(with);
	if (l1 < l2) return 0;
	for (int i = 0; i < l2; i++) {
		char s1 = str[i];
		if (s1 >= 'A' && s1 <= 'Z') s1 += ' ';
		char s2 = with[i];
		if (s2 >= 'A' && s2 <= 'Z') s2 += ' ';
		if (s1 != s2) {
			return 0;
		}
	}
	return 1;
}

int endsWith(const char* str, const char* with) {
	if (str == NULL || with == NULL) return 0;
	if (str == with) return 1;
	size_t l1 = strlen(str);
	size_t l2 = strlen(with);
	if (l1 < l2) return 0;
	for (int i = 0; i < l2; i++) {
		if (str[l1 - 1 - (l2 - 1 - i)] != with[i]) {
			return 0;
		}
	}
	return 1;
}

int endsWith_nocase(const char* str, const char* with) {
	if (str == NULL || with == NULL) return 0;
	if (str == with) return 1;
	size_t l1 = strlen(str);
	size_t l2 = strlen(with);
	if (l1 < l2) return 0;
	for (int i = 0; i < l2; i++) {
		char s1 = str[l1 - 1 - (l2 - 1 - i)];
		if (s1 >= 'A' && s1 <= 'Z') s1 += ' ';
		char s2 = with[i];
		if (s2 >= 'A' && s2 <= 'Z') s2 += ' ';
		if (s1 != s2) {
			return 0;
		}
	}
	return 1;
}

int contains(const char* str, const char* with) {
	if (str == NULL || with == NULL) return 0;
	if (str == with) return 1;
	size_t l1 = strlen(str);
	size_t l2 = strlen(with);
	if (l1 < l2) return 0;
	int ml = 0;
	for (int i = 0; i < l1; i++) {
		if (str[i] == with[ml]) {
			if (++ml == l2) {
				return 1;
			}
		} else ml = 0;
	}
	return 0;
}

int contains_nocase(const char* str, const char* with) {
	if (str == NULL || with == NULL) return 0;
	if (str == with) return 1;
	size_t l1 = strlen(str);
	size_t l2 = strlen(with);
	if (l1 < l2) return 0;
	int ml = 0;
	for (int i = 0; i < l1; i++) {
		char s1 = str[i];
		if (s1 >= 'A' && s1 <= 'Z') s1 += ' ';
		char s2 = with[ml];
		if (s2 >= 'A' && s2 <= 'Z') s2 += ' ';
		if (s1 == s2) {
			if (++ml == l2) {
				return 1;
			}
		} else ml = 0;
	}
	return 0;
}

char* toLowerCase(char* str) {
	if (str == NULL) return NULL;
	size_t l = strlen(str);
	for (int i = 0; i < l; i++) {
		if (str[i] >= 'A' && str[i] <= 'Z') str[i] += ' ';
	}
	return str;
}

char* toUpperCase(char* str) {
	if (str == NULL) return NULL;
	size_t l = strlen(str);
	for (int i = 0; i < l; i++) {
		if (str[i] >= 'a' && str[i] <= 'z') str[i] -= ' ';
	}
	return str;
}

char* urlencode(char* str) {
	size_t sl = strlen(str);
	ssize_t off = 0;
	for (size_t i = 0; i < sl; i++) {
		char c = str[i];
		if (c == '\"' || c == '#' || c == '$' || c == '%' || c == '&' || c == '+' || c == '-' || c == ',' || c == '/' || c == ':' || c == ';' || c == '=' || c == '?' || c == '@' || c == ' ' || c == '\t' || c == '>' || c == '<' || c == '{' || c == '}' || c == '|' || c == '\\' || c == '^' || c == '~' || c == '[' || c == ']' || c == '`') {
			sl += 3;
			str = xrealloc(str, sl + 1);
			str[sl] = 0;
			memmove(str + i + 3, str + i + 1, sl - i);
			char sc[4];
			snprintf(sc + 1, 3, "%02X", (uint8_t) c);
			sc[0] = '%';
			memcpy(str + i - 1 + 1, sc, 3);
			off += (3 - 1);
			i += (3 - 1);
		}
	}
	return str;
}

char* replace(char* str, char* from, char* to) {
	size_t sl = strlen(str);
	size_t fl = strlen(from);
	size_t tl = strlen(to);
	size_t ml = 0;
	for (size_t i = 0; i < sl - fl; i++) {
		char c = str[i];
		if (c == from[ml]) {
			if (++ml == fl) {
				if (tl == fl) {
					memcpy(str + i - fl + 1, to, tl);
				} else if (tl < fl) {
					memcpy(str + i - fl + 1, to, tl);
					memmove(str + i + tl - fl + 1, str + i + fl - fl + 1, sl - i - fl + 1 + 1);
				} else {
					sl += (tl - fl);
					str = xrealloc(str, sl);
					memmove(str + i + tl - fl - 1, str + i - 1, sl - i + 1 + 1);
					memcpy(str + i - fl + 1, to, tl);
				}
				i += (tl - fl);
			}
		} else ml = 0;
	}
	return str;
}

char* replace_nocase(char* str, char* from, char* to) {
	size_t sl = strlen(str);
	size_t fl = strlen(from);
	size_t tl = strlen(to);
	size_t ml = 0;
	for (size_t i = 0; i < sl - fl; i++) {
		char c = str[i];
		if (c >= 'A' && c <= 'Z') c += ' ';
		char c2 = from[ml];
		if (c2 >= 'A' && c2 <= 'Z') c2 += ' ';
		if (c == c2) {
			if (++ml == fl) {
				if (tl == fl) {
					memcpy(str + i - fl + 1, to, tl);
				} else if (tl < fl) {
					memcpy(str + i - fl + 1, to, tl);
					memmove(str + i + tl - fl + 1, str + i + fl - fl + 1, sl - i - fl + 1 + 1);
				} else {
					sl += (tl - fl);
					str = xrealloc(str, sl);
					memmove(str + i + tl - fl - 1, str + i - 1, sl - i + 1 + 1);
					memcpy(str + i - fl + 1, to, tl);
				}
				i += (tl - fl);
			}
		} else ml = 0;
	}
	return str;
}

int strisunum(const char* str) {
	if (str == NULL) return 0;
	size_t len = strlen(str);
	if (len < 1) return 0;
	for (int i = 0; i < len; i++) {
		if (str[i] < '0' || str[i] > '9') {
			return 0;
		}
	}
	return 1;
}

