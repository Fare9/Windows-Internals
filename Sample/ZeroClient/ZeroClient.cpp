
#include "pch.h"
#include "../Zero/zero_common.h"

int Error(const char* msg)
{
	printf("%s: error=%d\n", msg, GetLastError());
	return 1;
}

int main()
{
	HANDLE hDevice = CreateFile(L"\\\\.\\Zero", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return Error("faileed to open device");
	}

	// test read
	BYTE buffer[64];

	// store some non-zero data
	for (int i = 0; i < sizeof(buffer); i++)
	{
		buffer[i] = i + 1;
	}

	DWORD bytes;
	BOOL ok = ReadFile(hDevice, buffer, sizeof(buffer), &bytes, nullptr);

	if (!ok)
	{
		CloseHandle(hDevice);
		return Error("failed to read");
	}

	if (bytes != sizeof(buffer))
		printf("Wrong number of bytes\n");

	// check if buffer data sum is zero
	long total = 0;
	for (auto n : buffer)
		total += n;
	if (total != 0)
		printf("Wrong data\n");

	// test write
	BYTE buffer2[1024];
	ok = WriteFile(hDevice, buffer2, sizeof(buffer2), &bytes, nullptr);
	if (!ok)
	{
		CloseHandle(hDevice);
		return Error("failed to write");
	}
	if (bytes != sizeof(buffer2))
		printf("Wrong byte count\n");

	NumberOfBytes nBytesStruct = { 0 };
	DWORD bytesWritten;

	ok = DeviceIoControl(
		hDevice,
		IOCTL_ZERO_COUNT_BYTES,
		&nBytesStruct,
		sizeof(NumberOfBytes),
		&nBytesStruct,
		sizeof(NumberOfBytes),
		&bytesWritten,
		nullptr
	);

	if (!ok)
	{
		CloseHandle(hDevice);
		return Error("failed DeviceIoControl");
	}

	printf("Number of bytes read by driver: %d\nNumber of bytes written by driver: %d\n", nBytesStruct.nBytesRead, nBytesStruct.nBytesWrite);


	CloseHandle(hDevice);
	return 0;
}

