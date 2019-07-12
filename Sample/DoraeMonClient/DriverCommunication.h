#pragma once

#include "pch.h"


#ifndef DRIVERCOMMUNICATION_H
#define DRIVERCOMMUNICATION_H

BOOL OpenCommunicationWithDriver(const WCHAR* driverPath);
BOOL ReadDataFromDriver();
BOOL Monitorize();
BOOL CloseCommunicationWithDriver();


#endif // !DRIVERCOMMUNICATION_H
