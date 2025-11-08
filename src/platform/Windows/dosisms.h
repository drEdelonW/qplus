/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//
// dosisms.h: I'd call it dos.h, but the name's taken
//

#ifndef _DOSISMS_H_
#define _DOSISMS_H_

int dos_lockmem(TypeLess_ptr addr, int size);
int dos_unlockmem(TypeLess_ptr addr, int size);

typedef union {
	struct {
		unsigned long edi;
		unsigned long esi;
		unsigned long ebp;
		unsigned long res;
		unsigned long ebx;
		unsigned long edx;
		unsigned long ecx;
		unsigned long eax;
	} d;
	struct {
		uint16_t di, di_hi;
		uint16_t si, si_hi;
		uint16_t bp, bp_hi;
		uint16_t res, res_hi;
		uint16_t bx, bx_hi;
		uint16_t dx, dx_hi;
		uint16_t cx, cx_hi;
		uint16_t ax, ax_hi;
		uint16_t flags;
		uint16_t es;
		uint16_t ds;
		uint16_t fs;
		uint16_t gs;
		uint16_t ip;
		uint16_t cs;
		uint16_t sp;
		uint16_t ss;
	} x;
	struct {
		uint8_t edi[4];
		uint8_t esi[4];
		uint8_t ebp[4];
		uint8_t res[4];
		uint8_t bl, bh, ebx_b2, ebx_b3;
		uint8_t dl, dh, edx_b2, edx_b3;
		uint8_t cl, ch, ecx_b2, ecx_b3;
		uint8_t al, ah, eax_b2, eax_b3;
	} h;
} regs_t;

uint32_t ptr2real(TypeLess_ptr ptr);
TypeLess_ptr real2ptr(uint32_t real);
TypeLess_ptr far2ptr(uint32_t farptr);
uint32_t ptr2far(TypeLess_ptr ptr);

int	dos_inportb(int port);
int	dos_inportw(int port);
void dos_outportb(int port, int val);
void dos_outportw(int port, int val);

void dos_irqenable(void);
void dos_irqdisable(void);
void dos_registerintr(int intr, void (*handler)(void));
void dos_restoreintr(int intr);

int	dos_int86(int vec);

TypeLess_ptr dos_getmemory(int size);
void dos_freememory(TypeLess_ptr ptr);

void	dos_usleep(int usecs);

int dos_getheapsize(void);

extern regs_t regs;

#endif	// _DOSISMS_H_

