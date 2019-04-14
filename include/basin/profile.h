/*
 * profile.h
 *
 *  Created on: Dec 30, 2016
 *      Author: root
 */

#ifndef PROFILE_H_
#define PROFILE_H_

void beginProfilerSection(char* name);

void endProfilerSection(char* name);

void printProfiler();

void clearProfiler();

#endif /* PROFILE_H_ */
