
#include "Line.h"

/*
 * PRIVATE DEFINITIONS
 */

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

/*
 * PRIVATE VARIABLES
 */

/*
 * PUBLIC FUNCTIONS
 */

void Line_Init(LineParser_t * line, void * buffer, uint32_t size, void (*callback)(char * line))
{
	line->buffer = (char *)buffer;
	line->size = size;
	line->index = 0;
	line->callback = callback;
}

void Line_Parse(LineParser_t * line, char * read, uint32_t count)
{
	while(count--)
	{
		char ch = *read++;
		switch (ch)
		{
		case '\n':
		case '\r':
		case 0:
			if (line->index)
			{
				line->buffer[line->index] = 0;
				line->callback(line->buffer);
				line->index = 0;
			}
			break;
		default:
			// Leave room for null terminator
			if (line->index >= line->size - 1)
			{
				// Discard the entire line
				line->index = 0;
			}
			else
			{
				line->buffer[line->index++] = ch;
			}
			break;
		}
	}
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * INTERRUPT ROUTINES
 */

