#pragma once

#include "Process.h"
#include "Thread.h"

void HelpMarker(const char* desc);


void ShowProcessOnList(Process process);
void ShowThreadOnList(Thread thread);


void RemoveProcessFromList(Process process);
void RemoveThreadFromList(Thread thread);


void ShowBar(const char* prefix, int pid, bool show_less);
