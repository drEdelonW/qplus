#pragma once

#include <stdio.h>
#include "types.h"

/*
=========================================================

TARGA LOADING

=========================================================
*/

uint8_p LoadTGA(FILE* fin);

void WriteTGAfile(
    cString filename, uint8_p data,
    int width, int height,
    int rowbytes, uint8_p palette
);
