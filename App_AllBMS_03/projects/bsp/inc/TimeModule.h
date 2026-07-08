#ifndef __TIMEMODULE_H__
#define __TIMEMODULE_H__

#include "UserDef.h"

extern const uint8_t c_szMonthDay[12];
extern uint8_t IsLeap(uint16_t wYear);
extern uint32_t DATE_DateToUtc(tTimeParam tTime);
extern uint8_t DATE_UtcToDate(uint32_t dwSencondCount, tTimeParam *tTime);

#endif
