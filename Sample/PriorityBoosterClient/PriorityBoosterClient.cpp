
#include <windows.h>
#include <stdio.h>
#include "..\PriorityBooster\PriorityBoosterCommon.h"

int Error(const char* message);

int main(int argc, const char *argv[])
{
	HANDLE hDevice;
	ThreadData data;
	DWORD returned;
	BOOL success;

	if (argc < 3)
	{
		printf("USAGE: %s <threadid> <priority>\n");
		return 0;
	}

	hDevice = CreateFile(L"\\\\.\\PriorityBooster", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");
    
	
	data.ThreadId = atoi(argv[1]);
	data.Priority = atoi(argv[2]);

	success = DeviceIoControl(hDevice,
		IOCTL_PRIORITY_BOOSTER_SET_PRIORITY,
		&data, sizeof(data),
		nullptr, 0,
		&returned, nullptr);

	if (success)
		printf("Priority change succeeded!\n");
	else
		Error("Priority change failed!");

	CloseHandle(hDevice);
}

int Error(const char* message)
{
	printf("%s (error=%d)\n", message, GetLastError());
	return 1;
}

