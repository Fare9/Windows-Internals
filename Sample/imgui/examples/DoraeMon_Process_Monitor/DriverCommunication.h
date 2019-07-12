#pragma once

#include "headers.h"

#ifndef DRIVERCOMMUNICATION_H
#define DRIVERCOMMUNICATION_H

BOOL OpenCommunicationWithDriver(const WCHAR* driverPath);
BOOL ReadDataFromDriver();
BOOL Monitorize();
BOOL CloseCommunicationWithDriver();
VOID SaveStructure(BYTE* buffer, ItemHeader* item);
VOID PrintProcesses();

#endif // !DRIVERCOMMUNICATION_H
