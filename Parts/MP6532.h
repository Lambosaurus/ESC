#ifndef MP6532_H
#define MP6532_H

#include "STM32X.h"

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

/*
 * PUBLIC FUNCTIONS
 */

void MP6532_Init(void);
void MP6532_Deinit(void);
bool MP6532_IsFaulted(void);

void MP6532_SetDuty(uint8_t duty);
void MP6532_Step(void);

/*
 * EXTERN DECLARATIONS
 */

#endif //MP6532_H
