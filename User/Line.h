#ifndef LINE_H
#define LINE_H

#include "STM32X.h"

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

typedef struct {
	char * buffer;
	uint32_t size;
	uint32_t index;
	void (*callback)(char * line);
} LineParser_t;

/*
 * PUBLIC FUNCTIONS
 */

void Line_Init(LineParser_t * line, void * buffer, uint32_t size, void (*callback)(char * line));
void Line_Parse(LineParser_t * line, char * read, uint32_t count);

/*
 * EXTERN DECLARATIONS
 */

#endif //LINE_H
