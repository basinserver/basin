/*
 * xstring.c
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */
#include <string.h>
#include <ctype.h>

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
