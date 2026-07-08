#pragma once

#include <stdio.h>
#include "types.h"
#include "qColor.h"

/*
=================================================================

PCX Loading

=================================================================
*/

/*
==============
WritePCXfile
==============
*/

uint8_p LoadPCX(FILE* f);

void WritePCXfile(
    cString filename, uint8_p data,
    int width, int height,
    int rowbytes, palette_p palette
);
