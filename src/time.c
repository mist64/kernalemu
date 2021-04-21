// KERNAL Emulator
// Copyright (c) 2009-2021 Michael Steil
// All rights reserved. License: 2-clause BSD

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "glue.h"
#include "time.h"

// SETTIM - Set real time clock
void
SETTIM()
{
	unsigned long jiffies = y*65536 + x*256 + a;
	unsigned long seconds = jiffies/60;
#ifdef _WIN32
	SYSTEMTIME st;

	GetLocalTime(&st);
	st.wHour = (WORD)(seconds/3600);
	st.wMinute = (WORD)(seconds/60);
	st.wSecond = (WORD)(seconds%60);
	st.wMilliseconds = (WORD)((jiffies % 60) * 1000 / 60);
	SetLocalTime(&st);
#else
	time_t now = time(0);
	struct tm bd;
	struct timeval tv;

	localtime_r(&now, &bd);

	bd.tm_sec = seconds % 60;
	bd.tm_min = seconds / 60;
	bd.tm_hour = seconds / 3600;

	tv.tv_sec = mktime(&bd);
	tv.tv_usec = (jiffies % 60) * (1000000 / 60);

	settimeofday(&tv, 0);
#endif
}

// RDTIM - Read real time clock
void
RDTIM()
{
	unsigned long jiffies;
#ifdef _WIN32
	SYSTEMTIME st;

	GetLocalTime(&st);
	jiffies = ((st.wHour*60 + st.wMinute)*60 + st.wSecond)*60 + st.wMilliseconds * 60 / 1000;
#else
	time_t now = time(0);
	struct tm bd;
	struct timeval tv;

	localtime_r(&now, &bd);
	gettimeofday(&tv, 0);

	jiffies = ((bd.tm_hour * 60 + bd.tm_min) * 60 + bd.tm_sec) * 60 + tv.tv_usec / (1000000 / 60);
#endif
	y = (uint8_t)(jiffies / 65536);
	x = (uint8_t)((jiffies % 65536) / 256);
	a = (uint8_t)(jiffies % 256);
}

// UDTIM - Increment real time clock
void
UDTIM()
{
	// do nothing
}
