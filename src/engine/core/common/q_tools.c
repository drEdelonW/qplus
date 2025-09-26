#include "q_tools.h"
#include <stdint.h>

/*
============================================================================

                    LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void Q_memset(typeless_ptr dest, int32_t fill, int32_t count) {
    if ((((int32_t)dest | count) & 3) == 0) {
        count >>= 2;
        fill = fill | (fill << 8) | (fill << 16) | (fill << 24);
        for (int32_t i = 0; i < count; i++) {
            ((int32_t*)dest)[i] = fill;
        }
    }
    else {
        for (int32_t i = 0; i < count; i++) {
            ((uint8_p)dest)[i] = fill;
        }
    }
}

void Q_memcpy(typeless_ptr dest, typeless_ptr src, int32_t count) {
    if ((((int32_t)dest | (int32_t)src | count) & 3) == 0) {
        count >>= 2;
        for (int32_t i = 0; i < count; i++) {
            ((int32_t*)dest)[i] = ((int32_t*)src)[i];
        }
    }
    else {
        for (int32_t i = 0; i < count; i++) {
            ((uint8_p)dest)[i] = ((uint8_p)src)[i];
        }
    }
}

int Q_memcmp(typeless_ptr m1, typeless_ptr m2, int32_t count) {
    while (count) {
        count--;
        if (((uint8_p)m1)[count] != ((uint8_p)m2)[count]) {
            return -1;
        }
    }
    return 0;
}

void Q_strcpy(cstring dest, cstring src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest++ = 0;
}

void Q_strncpy(cstring dest, cstring src, int32_t count) {
    while (*src && count--) {
        *dest++ = *src++;
    }
    if (count) {
        *dest++ = 0;
    }
}

int32_t Q_strlen(cstring str) {
    int32_t count = 0;
    while (str[count])
        count++;

    return count;
}

cstring Q_strrchr(cstring s, char c) {
    int32_t len = Q_strlen(s);
    s += len;
    while (len--) {
        if (*--s == c) {
            return s;
        }
    }
    return 0;
}

void Q_strcat(cstring dest, cstring src) {
    dest += Q_strlen(dest);
    Q_strcpy(dest, src);
}

int32_t Q_strcmp(cstring s1, cstring s2) {
    while (1) {
        if (*s1 != *s2)
            return -1;              // strings not equal
        if (!*s1)
            return 0;               // strings are equal
        s1++;
        s2++;
    }

    return -1;
}

int Q_strncmp(cstring s1, cstring s2, int32_t count) {
    while (1) {
        if (!count--)
            return 0;
        if (*s1 != *s2)
            return -1;              // strings not equal
        if (!*s1)
            return 0;               // strings are equal
        s1++;
        s2++;
    }

    return -1;
}

int Q_strncasecmp(cstring s1, cstring s2, int32_t n) {
    while (1) {
        int c1 = *s1++;
        int c2 = *s2++;

        if (!n--)
            return 0;               // strings are equal until end point

        if (c1 != c2) {
            if ((c1 >= 'a') && (c1 <= 'z'))
                c1 -= ('a' - 'A');
            if ((c2 >= 'a') && (c2 <= 'z'))
                c2 -= ('a' - 'A');
            if (c1 != c2)
                return -1;              // strings not equal
        }
        if (!c1)
            return 0;               // strings are equal
        //              s1++;
        //              s2++;
    }

    return -1;
}

int Q_strcasecmp(cstring s1, cstring s2) {
    return Q_strncasecmp(s1, s2, 99999);
}

int Q_atoi(cstring str) {
    int sign;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else {
        sign = 1;
    }

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
            if ((c >= '0') && (c <= '9'))
                val = (val << 4) + c - '0';
            else if ((c >= 'a') && (c <= 'f'))
                val = (val << 4) + c - 'a' + 10;
            else if ((c >= 'A') && (c <= 'F'))
                val = (val << 4) + c - 'A' + 10;
            else
                return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'') {
        return sign * str[1];
    }

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


float Q_atof(cstring str) {
    int     sign;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else {
        sign = 1;
    }

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
            if ((c >= '0') && (c <= '9'))
                val = (val * 16) + c - '0';
            else if ((c >= 'a') && (c <= 'f'))
                val = (val * 16) + c - 'a' + 10;
            else if ((c >= 'A') && (c <= 'F'))
                val = (val * 16) + c - 'A' + 10;
            else
                return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'') {
        return sign * str[1];
    }

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
