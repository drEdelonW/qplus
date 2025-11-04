#include "q_tools.h"

/*
============================================================================

                    LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

#if 0
void Q_memset(TypeLess_ptr dest, int32_t fill, int32_t count) {
    if ((((int32_t)dest | count) & 3) == 0) {
        count >>= 2;
        fill = fill | (fill << 8) | (fill << 16) | (fill << 24);
        for (int32_t i = 0; i < count; i++) {
            ((int32_p)dest)[i] = fill;
        }
    }
    else {
        for (int32_t i = 0; i < count; i++) {
            ((uint8_p)dest)[i] = fill;
        }
    }
}
#else
void Q_memset(TypeLess_ptr dest, int32_t fill, int32_t count) {
    if (count <= 0) return;
    /* 64-bit safe alignment check */
    if ((((uintptr_t)dest | (uintptr_t)count) & 3u) == 0u) {
        int32_t n = count >> 2;
        uint32_t f = (uint8_t)fill;
        f |= (f << 8);
        f |= (f << 16);
        uint32_p d32 = (uint32_p)dest;
        for (int32_t i = 0; i < n; ++i)
            d32[i] = f;
    }
    else {
        uint8_p d8 = (uint8_p)dest;
        uint8_t f8 = (uint8_t)fill;
        for (int32_t i = 0; i < count; ++i)
            d8[i] = f8;
    }
}
#endif

#if 0
void Q_memcpy(TypeLess_ptr dest, TypeLess_ptr src, int32_t count) {
    if ((((int32_t)dest | (int32_t)src | count) & 3) == 0) {
        count >>= 2;
        for (int32_t i = 0; i < count; i++) {
            ((int32_p)dest)[i] = ((int32_p)src)[i];
        }
    }
    else {
        for (int32_t i = 0; i < count; i++) {
            ((uint8_p)dest)[i] = ((uint8_p)src)[i];
        }
    }
}
#else
void Q_memcpy(TypeLess_ptr dest, TypeLess_ptr src, int32_t count) {
    if (count <= 0) return;
    if ((((uintptr_t)dest | (uintptr_t)src | (uintptr_t)count) & 3u) == 0u) {
        int32_t n = count >> 2;
        uint32_p d32 = (uint32_p)dest;
        const uint32_p s32 = (const uint32_p)src;
        for (int32_t i = 0; i < n; ++i) d32[i] = s32[i];
    }
    else {
        uint8_p d8 = (uint8_p)dest;
        const uint8_p s8 = (const uint8_p)src;
        for (int32_t i = 0; i < count; ++i) d8[i] = s8[i];
    }
}
#endif

int Q_memcmp(TypeLess_ptr m1, TypeLess_ptr m2, int32_t count) {
    while (count) {
        count--;
        if (((uint8_p)m1)[count] != ((uint8_p)m2)[count]) {
            return -1;
        }
    }
    return 0;
}

void Q_strcpy(cString dest, cStringRO src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest++ = 0;
}

#if 0
void Q_strncpy(cString dest, cStringRO src, int32_t count) {
    while (*src && count--) {
        *dest++ = *src++;
    }
    if (count) {
        *dest++ = 0x00;
    }
}
#else
void Q_strncpy(cString dest, cStringRO src, int32_t count) {
    // if (count <= 0) return;
    int32_t n = count;
    while ((--n > 0) && *src) {
        *dest++ = *src++;
    }
    *dest = 0x00; /* always NUL-terminate */
}
#endif

int Q_strlen(cStringRO str) {
    int32_t count = 0;
    while (str[count])
        count++;

    return count;
}

cString Q_strrchr(cString s, char c) {
    int32_t len = Q_strlen(s);
    s += len;
    while (len--) {
        if (*--s == c) {
            return s;
        }
    }
    return NULL;
}

void Q_strcat(cString dest, cStringRO src) {
    dest += Q_strlen(dest);
    Q_strcpy(dest, src);
}

int Q_strcmp(cStringRO s1, cStringRO s2) {
    while (1) {
        if (*s1 != *s2)     return -1;              // strings not equal
        if (!*s1)           return 0;               // strings are equal
        s1++;
        s2++;
    }

    return -1;
}

int Q_strncmp(cStringRO s1, cStringRO s2, int32_t count) {
    while (1) {
        if (!count--)       return 0;
        if (*s1 != *s2)     return -1;              // strings not equal
        if (!*s1)           return 0;               // strings are equal
        s1++;
        s2++;
    }

    return -1;
}

#if 0
int Q_strncasecmp(cStringRO s1, cStringRO s2, int32_t n) {
    while (1) {
        int c1 = *s1++;
        int c2 = *s2++;

        if (!n--)   return 0;               // strings are equal until end point

        if (c1 != c2) {
            if ((c1 >= 'a') && (c1 <= 'z')) c1 -= ('a' - 'A');
            if ((c2 >= 'a') && (c2 <= 'z')) c2 -= ('a' - 'A');
            if (c1 != c2)   return -1;              // strings not equal
        }
        if (!c1)            return 0;               // strings are equal
    }

    return -1;
}
#else
int Q_strncasecmp(cStringRO s1, cStringRO s2, int32_t n) {
    // if (n <= 0) return 0;
    do {
        int c1 = *s1++;
        int c2 = *s2++;
        if ((c1 >= 'a') && (c1 <= 'z'))     c1 -= ('a' - 'A');
        if ((c2 >= 'a') && (c2 <= 'z'))     c2 -= ('a' - 'A');
        if (c1 != c2)   return -1;     /* not equal */
        if (c1 == 0)    return 0;      /* equal (both '\0') */
    } while (--n);
    return 0;
}
#endif

int Q_strcasecmp(cStringRO s1, cStringRO s2) {
    return Q_strncasecmp(s1, s2, 99999);
}

int Q_atoi(cStringRO str) {
    int sign;

    if (*str == '-') { sign = -1;   str++; }
    else                sign = 1;


    int val = 0;

    //
    // check for hex
    //
    if (
        (str[0] == '0') &&
        ((str[1] == 'x') ||
            (str[1] == 'X'))
        ) {
        str += 2;
        while (1) {
            int c = *str++;
            if ((c >= '0') && (c <= '9'))           val = (val << 4) + c - '0';
            else if ((c >= 'a') && (c <= 'f'))      val = (val << 4) + c - 'a' + 10;
            else if ((c >= 'A') && (c <= 'F'))      val = (val << 4) + c - 'A' + 10;
            else                                    return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'')
        return sign * str[1];


    //
    // assume decimal
    //
    while (1) {
        int c = *str++;
        if ((c < '0') || (c > '9'))
            return val * sign;

        val = val * 10 + c - '0';
    }

    return 0;
}


float Q_atof(cStringRO str) {
    int     sign;

    if (*str == '-') { sign = -1;   str++; }
    else                sign = 1;

    double val = 0;

    //
    // check for hex
    //
    if (
        (str[0] == '0') &&
        ((str[1] == 'x') ||
            (str[1] == 'X'))
        ) {
        str += 2;
        while (1) {
            int c = *str++;
            if ((c >= '0') && (c <= '9'))           val = (val * 16) + c - '0';
            else if ((c >= 'a') && (c <= 'f'))      val = (val * 16) + c - 'a' + 10;
            else if ((c >= 'A') && (c <= 'F'))      val = (val * 16) + c - 'A' + 10;
            else                                    return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'')
        return sign * str[1];


    //
    // assume decimal
    //
    int decimal = -1;
    int total = 0;
    while (1) {
        int c = *str++;
        if (c == '.') {
            decimal = total;
            continue;
        }
        if ((c < '0') || (c > '9'))
            break;

        val = val * 10 + c - '0';
        total++;
    }

    if (decimal == -1)
        return val * sign;

    while (total > decimal) {
        val /= 10;
        total--;
    }

    return val * sign;
}
