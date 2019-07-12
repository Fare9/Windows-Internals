#pragma once

#include "pch.h"

#ifndef SCREENPRINTER_H
#define SCREENPRINTER_H

VOID PrintDoraemon();
VOID PrintHeader();
VOID PrintStructure(BYTE* buffer, ItemHeader* item);
VOID ClearScreen();



#endif // !SCREENPRINTER_H
