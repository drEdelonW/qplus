#include "progLump.h"

static TypeLess_ptr _base = NULL;
void SetLumpBase(TypeLess_ptr base){ _base = base; }

#include "host.h"
TypeLess_ptr GetPtrFromLump(progLump_t pl) {
    if (_base)  return (uint8_p)_base + pl.ofs;
    else        Host_Error("Lump _base not seted\n");
}