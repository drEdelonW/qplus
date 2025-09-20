#pragma once
#include "qboolean.h"
#include <stdint.h>

//============================================================================

extern	qboolean		bigendien;

extern	int16_t	(*BigShort) (int16_t l);
extern	int16_t	(*LittleShort) (int16_t l);
extern	int	(*BigLong) (int l);
extern	int	(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

void COM_Endian_Init();