/*
 * profile.h
 *
 *  Created on: Dec 30, 2016
 *      Author: root
 */

#ifndef BASIN_PROFILE_H_
#define BASIN_PROFILE_H_

void beginProfilerSection(char* name);

void endProfilerSection(char* name);

void printProfiler();

void clearProfiler();

#endif /* BASIN_PROFILE_H_ */
