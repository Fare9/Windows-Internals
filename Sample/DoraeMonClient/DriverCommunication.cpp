#include "pch.h"
#include "DriverCommunication.h"
#include "ScreenPrinter.h"

#define BUFFER_SIZE 16 // 64KB

HANDLE hFile;

BYTE buffer[1 << BUFFER_SIZE] = { 0 };

DWORD bytes;

BOOL Error(const char* msg)
{
	printf("%s: error=%d\n", msg, GetLastError());
	return FALSE;
}

BOOL OpenCommunicationWithDriver(const WCHAR* driverPath)
{
	hFile = CreateFile(driverPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return Error("Failed to open driver");
	}

	return TRUE;
}

BOOL ReadDataFromDriver()
{
	if (!ReadFile(hFile, buffer, sizeof(buffer), &bytes, nullptr))
	{
		return Error("Failed to read");
	}

	return TRUE;
}

BOOL Monitorize()
{

	if (!ReadDataFromDriver())
	{
		return FALSE;
	}

	if (bytes)
	{
		PrintHeader();
		auto count = bytes;
		auto local_buffer = buffer;

		while (count > 0)
		{
			auto header = (ItemHeader*)local_buffer;
			PrintStructure(local_buffer, header);
			local_buffer += header->size; 
			count -= header->size;
		}
	}

	return TRUE;
}

BOOL CloseCommunicationWithDriver()
{
	if (!CloseHandle(hFile))
	{
		return Error("Error closing driver");
	}

	return TRUE;
}