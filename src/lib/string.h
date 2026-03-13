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



#pragma once

const char* strchr(const char* str, char chr);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, int num);
unsigned strlen(const char* str);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, int num);
char *strtok(char *s, const char *delim);
long strtol(char* start, char** end, int base);
