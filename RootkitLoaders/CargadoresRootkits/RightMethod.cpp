#include "stdafx.h"
#include "RightMethod.h"

BOOL util_load_sysfile(TCHAR *DriverName)
/*
*	Util_Load_Sysfile:
*		Correct method to load a driver in the system,
*		it creates a service to execute the driver,
*		this must be the method used by OSRDriver.
*		Info about OpenSCManager: https://msdn.microsoft.com/es-es/library/windows/desktop/ms684323(v=vs.85).aspx
*		
*	Parameters:
*		TCHAR *DriverName: driver's name, file.sys
*	Return:
*		BOOL: was everything correct mi comandante?
*/
{
	TCHAR aPath[1024];
	TCHAR aCurrentDirectory[515];

	// servicio
	SC_HANDLE sc = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!sc)
	{
		return FALSE;
	}

	GetCurrentDirectory(512, aCurrentDirectory);

	_sntprintf(aPath,
		1022,
		_T("%s\\%s.sys"),
		aCurrentDirectory,
		DriverName);

	_tprintf(_T("Loading %s\n"), aPath);

	SC_HANDLE rc = CreateService(sc,
		DriverName,
		DriverName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		aPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	if (!rc)
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS)
		{
			// the service already exists
			rc = OpenService(sc,
				DriverName,
				SERVICE_ALL_ACCESS);

			if (!rc)
			{
				CloseServiceHandle(sc);
				return FALSE;
			}
		}
		else
		{
			CloseServiceHandle(sc);
			return FALSE;
		}
	}
	// start the driver
	if (rc)
	{
		if (0 == StartService(rc, 0, NULL))
		{
			if (ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
			{
				// no problem.
			}
			else
			{
				CloseServiceHandle(sc);
				CloseServiceHandle(rc);
				return FALSE;
			}
		}
		CloseServiceHandle(sc);
		CloseServiceHandle(rc);
	}
	return TRUE;
}