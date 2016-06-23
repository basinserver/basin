/*
 * streams.h
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#ifndef STREAMS_H_
#define STREAMS_H_

size_t readLine(int fd, char* line, size_t len);

size_t writeLine(int fd, char* line, size_t len);

#endif /* STREAMS_H_ */
