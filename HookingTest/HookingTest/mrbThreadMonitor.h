/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef MRBTHREADMONITOR
#define MRBTHREADMONITOR

#include <windows.h>
#include <tlhelp32.h>
#include "linkedList.h"
#include "mrbDetoursStyleHook.h"

#define STATUS_SUCCESS 0

typedef struct _CLIENT_ID
{
     PVOID UniqueProcess;
     PVOID UniqueThread;
} CLIENT_ID, *PCLIENT_ID;


ThreadList Threads;

void InitThreadMonitor();
void DestroyThreadMonitor();

#endif