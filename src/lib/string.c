/*
 * Copyright (C) 2025,  Novice
 *
 * This file is part of the Novix software.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/



#include "string.h"
#include "ctype.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

const char* strchr(const char* str, char chr)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char* strcpy(char* dst, const char* src)
{
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src)
    {
        *dst = *src;
        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

char* strncpy(char* dst, const char* src, int num)
{
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    // while (*src)
    for(int i = 0; i < num && *src; i++)
    {
        *dst = *src;
        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

unsigned strlen(const char* str)
{
    unsigned len = 0;
    while (*str)
    {
        ++len;
        ++str;
    }

    return len;
}

int strcmp(const char *str1, const char *str2)
{
    while (*str1 == *str2)
    {
        if (*str1 == '\0')
            return 0;

        str1++;
        str2++;
    }

    return (*str1 < *str2) ? -1 : 1;
}

int strncmp(const char *str1, const char *str2, int num)
{
    for(int i = 0; i < num; i++)
    {
        if ((*str1 != *str2))
            return (*str1 < *str2) ? -1 : 1;

        if(*str1 == '\0')
            return 0;   // and of string

        str1++;
        str2++;
    }

    return 0;
}

char *strcat(char *dst, const char *src) {
    char *r = dst; while (*dst) dst++; while ((*dst++ = *src++)); return r;
}
char *strncat(char *dst, const char *src, size_t n) {
    char *r = dst; while (*dst) dst++;
    for (size_t i = 0; i < n && src[i]; i++) *dst++ = src[i];
    *dst = '\0'; return r;
}

static char *_tok;
char *strtok(char *s, const char *delim) {
    if (s) _tok = s;
    if (!_tok) return NULL;
    while (*_tok && strchr(delim, *_tok)) _tok++;
    if (!*_tok) { _tok = NULL; return NULL; }
    char *tok = _tok;
    while (*_tok && !strchr(delim, *_tok)) _tok++;
    if (*_tok) *_tok++ = '\0'; else _tok = NULL;
    return tok;
}

long strtol(char* start, char** end, int base)
{
    int c, cutlim, outRangeStatus;
    unsigned long cutoff, accumulator;
    bool is_negative = false;
    char *s = start;


    /*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */

    do
    {
        c = *s++;
    }while(c == ' ' || c == '\t');

    if (c == '-')
    {
		is_negative = true;
		c = *s++;
	}
    else if (c == '+')
		c = *s++;
    
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X'))
    {
		c = s[1];
		s += 2;
		base = 16;
	}
    else if ((base == 0 || base == 2) && c == '0' && (*s == 'b' || *s == 'B'))
    {
		c = s[1];
		s += 2;
		base = 2;
	}

	if (base == 0)
		base = c == '0' ? 8 : 10;

    /*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==false) or 8 (neg==true), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */

    cutoff = is_negative ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;

    for (accumulator = 0, outRangeStatus = 0;; c = *s++)
    {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;

		if (c >= base)
			break;
        
		if (outRangeStatus < 0 || accumulator > cutoff || accumulator == cutoff && c > cutlim)
			outRangeStatus = -1;
		else
        {
			outRangeStatus = 1;
			accumulator *= base;
			accumulator += c;
		}
	}

    if (outRangeStatus < 0)
		accumulator = is_negative ? LONG_MIN : LONG_MAX;
    else if (is_negative)
		accumulator = -accumulator;
    
	if (end != NULL)
		*end = (char *)(outRangeStatus ? s - 1 : start);
    
	return (accumulator);
}