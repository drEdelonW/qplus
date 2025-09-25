#pragma once
#include <stdint.h>
#include <stdbool.h>

//============================================================================

extern	bool		bigendien;

extern	int16_t(*BigShort) (int16_t l);
extern	int16_t(*LittleShort) (int16_t l);
extern	int32_t	(*BigLong) (int32_t l);
extern	int32_t	(*LittleLong) (int32_t l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

void COM_Endian_Init();