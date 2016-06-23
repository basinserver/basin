/*
 * xstring.h
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#ifndef XSTRING_H_
#define XSTRING_H_

#include <string.h>

char* trim(char* str);

int streq(const char* str1, const char* str2);

int streq_nocase(const char* str1, const char* str2);

int startsWith(const char* str, const char* with);

int startsWith_nocase(const char* str, const char* with);

int endsWith(const char* str, const char* with);

int endsWith_nocase(const char* str, const char* with);

int contains(const char* str, const char* with);

int contains_nocase(const char* str, const char* with);

char* toLowerCase(char* str);

char* toUpperCase(char* str);

int strisunum(const char* str);

#endif /* XSTRING_H_ */
