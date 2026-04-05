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

#include "ctype.h"

bool islower(char chr)
{
    return chr >= 'a' && chr <= 'z';
}

bool isupper(char chr)
{
    return chr >= 'A' && chr <= 'Z';
}

char toupper(char chr)
{
    return islower(chr) ? (chr - 'a' + 'A') : chr;
}

bool isalpha(char chr)
{
    return (chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z');
}

bool isdigit(char chr)
{
    char num[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

    for(int i = 0; i < 10; i++)
    {
        if(num[i] == chr)
            return true;
    }

    return false;
}

int isspace(char ch) {
    return ch == ' ' ||
          ch == '\n' ||
          ch == '\r' ||
          ch == '\v' ||
          ch == '\f' ||
          ch == '\t';
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}