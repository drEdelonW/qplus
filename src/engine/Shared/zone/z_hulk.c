// #include "zone.h"
#include "z_hunk.h"
#include "zone_prv.h"

#include <string.h>
#include "host.h"
#include "q_tools.h"


#define	HUNK_SENTINAL	(0x1DF001ED)


typedef struct {
    uint32_t    sentinal;
    size_t	    size;       // including sizeof(hunk_t), -1 = not allocated
    char 	    name[8];
} hunk_t;
typedef hunk_t* hunk_p;

uint8_p hunk_base;
size_t  hunk_size;

size_t  hunk_low_used;
size_t  hunk_high_used;

static bool _hunk_tempactive;
static size_t   _hunk_tempmark;


void Hulk_Init(TypeLess_ptr buf, size_t size) {
    hunk_base = buf;
    hunk_size = size;
    hunk_low_used = 0;
    hunk_high_used = 0;
}


/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check() {
    for (hunk_p h = (hunk_p)hunk_base; (uint8_p)h != (hunk_base + hunk_low_used); ) {
        if ((h->sentinal) != HUNK_SENTINAL)     Host_Error("Hunk_Check: trahsed sentinal");
        if ((h->size < 16) ||
            ((h->size + (uint8_p)h - hunk_base) > hunk_size)
            )                                   Host_Error("Hunk_Check: bad size");

        h = (hunk_p)((uint8_p)h + h->size);
    }
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void Hunk_Print(bool all) {
    char    name[9] = { 0 };

    hunk_p h = (hunk_p)hunk_base;
    hunk_p endlow = (hunk_p)(hunk_base + hunk_low_used);
    hunk_p starthigh = (hunk_p)(hunk_base + hunk_size - hunk_high_used);
    hunk_p endhigh = (hunk_p)(hunk_base + hunk_size);

    Host_Printf(
        "          :%8i total hunk size\n"
        "-------------------------\n",
        hunk_size
    );

    // size_t count = 0;
    size_t sum = 0;
    int totalblocks = 0;
    while (1) {
        //
        // skip to the high hunk if done with low hunk
        //
        if (h == endlow) {
            Host_Printf(
                "-------------------------\n"
                "          :%8i REMAINING\n"
                "-------------------------\n",
                hunk_size - hunk_low_used - hunk_high_used
            );
            h = starthigh;
        }

        //
        // if totally done, break
        //
        if (h == endhigh)
            break;

        //
        // run consistancy checks
        //
        if (h->sentinal != HUNK_SENTINAL)       Host_Error("Hunk_Check: trahsed sentinal");

        if ((h->size < 16) ||
            ((h->size + (uint8_p)h - hunk_base) > hunk_size)
            )                                   Host_Error("Hunk_Check: bad size");

        hunk_p next = (hunk_p)((uint8_p)h + h->size);
        // count++;
        totalblocks++;
        sum += h->size;

        //
        // print the single block
        //
        memcpy(name, h->name, 8);
        if (all)
            Host_Printf("%8p :%8i %8s\n", h, h->size, name);

        //
        // print the total
        //
        if (
            (next == endlow) ||
            (next == endhigh) ||
            (strncmp(h->name, next->name, 8))
            ) {
            if (!all)
                Host_Printf("          :%8i %8s (TOTAL)\n", sum, name);
            // count = 0;
            sum = 0;
        }

        h = next;
    }

    Host_Printf(
        "-------------------------\n"
        "%8i total blocks\n",
        totalblocks
    );

}

/*
===================
Hunk_AllocName
===================
*/
TypeLess_ptr Hunk_AllocName(size_t size, cStringRO name) {
#ifdef PARANOID
    Hunk_Check();
#endif

    size = sizeof(hunk_t) + ALIGN_UP(size, 16);

    if ((hunk_size - hunk_low_used - hunk_high_used) < size)
        Host_Error("Hunk_Alloc: failed on %i bytes", size);

    hunk_p h = (hunk_p)(hunk_base + hunk_low_used);
    hunk_low_used += size;

    Cache_FreeLow(hunk_low_used);

    memset(h, 0, size);
    h->size = size;
    h->sentinal = HUNK_SENTINAL;
    Q_strncpy(h->name, name, 8);

    return (TypeLess_ptr)(h + 1);
}

/*
===================
Hunk_Alloc
===================
*/
TypeLess_ptr Hunk_Alloc(size_t size) {
    return Hunk_AllocName(size, "unknown");
}

size_t	Hunk_LowMark() {
    return hunk_low_used;
}

void Hunk_FreeToLowMark(size_t mark) {
    if ((mark > hunk_low_used))             Host_Error("Hunk_FreeToLowMark: bad mark %i", mark);

    memset((hunk_base + mark), 0, (hunk_low_used - mark));
    hunk_low_used = mark;
}

size_t	Hunk_HighMark() {
    if (_hunk_tempactive) {
        _hunk_tempactive = false;
        Hunk_FreeToHighMark(_hunk_tempmark);
    }

    return hunk_high_used;
}

void Hunk_FreeToHighMark(size_t mark) {
    if (_hunk_tempactive) {
        _hunk_tempactive = false;
        Hunk_FreeToHighMark(_hunk_tempmark);
    }
    if (mark > hunk_high_used)          Host_Error("Hunk_FreeToHighMark: bad mark %i", mark);

    memset((hunk_base + hunk_size - hunk_high_used), 0, (hunk_high_used - mark));
    hunk_high_used = mark;
}


/*
===================
Hunk_HighAllocName
===================
*/
TypeLess_ptr Hunk_HighAllocName(size_t size, cStringRO name) {
    if (_hunk_tempactive) {
        Hunk_FreeToHighMark(_hunk_tempmark);
        _hunk_tempactive = false;
    }

#ifdef PARANOID
    Hunk_Check();
#endif

    size = sizeof(hunk_t) + ALIGN_UP(size, 16);

    if ((hunk_size - hunk_low_used - hunk_high_used) < size) {
        Host_Printf("Hunk_HighAlloc: failed on %i bytes\n", size);
        return NULL;
    }

    hunk_high_used += size;
    Cache_FreeHigh(hunk_high_used);

    hunk_p h = (hunk_p)(hunk_base + hunk_size - hunk_high_used);

    memset(h, 0, size);
    h->size = size;
    h->sentinal = HUNK_SENTINAL;
    Q_strncpy(h->name, name, 8);

    return (TypeLess_ptr)(h + 1);
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
TypeLess_ptr Hunk_TempAlloc(size_t size) {
    size = ALIGN_UP(size, 16);

    if (_hunk_tempactive) {
        Hunk_FreeToHighMark(_hunk_tempmark);
        _hunk_tempactive = false;
    }

    _hunk_tempmark = Hunk_HighMark();

    TypeLess_ptr buf = Hunk_HighAllocName(size, "temp");

    _hunk_tempactive = true;

    return buf;
}
