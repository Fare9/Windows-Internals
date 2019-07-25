

#include "pch.h"

#include "../YouShallNotPass/YouShallNotPassCommon.h"

int Error(const char* msg);
int PrintUsage();

int wmain(int argc, const wchar_t *argv[])
{
	if (argc < 2)
		return PrintUsage();

	enum class Options
	{
		Unknown,
		AddPath,
		RemovePath
	};

	Options option;

	if (::_wcsicmp(argv[1], L"AddPath") == 0)
		option = Options::AddPath;
	else if (::_wcsicmp(argv[1], L"RemovePath") == 0)
		option = Options::RemovePath;
	else
	{
		printf("Unknown option.\n");
		return PrintUsage();
	}

	HANDLE hFile = ::CreateFile(L"\\\\.\\" DEVICE_NAME, GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	BOOL success = FALSE;
	
	DWORD bytes;

	switch (option)
	{
	case Options::AddPath:
	{
		auto size = wcslen(argv[2]);
		size *= sizeof(wchar_t);
		size += sizeof(wchar_t); // + 00 00

		printf("Adding Path %ws with a size in bytes (with last 0) %d\n", argv[2], (int)size);

		success = ::DeviceIoControl(hFile, IOCTL_YOU_SHALL_NOT_PASS, (LPVOID)argv[2], (DWORD)size, nullptr, 0, &bytes, nullptr);
		break;
	}
	case Options::RemovePath:
	{
		auto size = wcslen(argv[2]);
		size *= sizeof(wchar_t);
		size += sizeof(wchar_t); // + 00 00

		printf("Removing Path %ws with a size in bytes (with last 0) %d\n", argv[2], (int)size);

		success = ::DeviceIoControl(hFile, IOCTL_YOU_CAN_PASS_AGAIN, (LPVOID)argv[2], (DWORD)size, nullptr, 0, &bytes, nullptr);

		break;
	}
	default:
		break;
	}

	if (!success)
		return Error("Failed in DeviceIoControl");

	printf("Operation succeeded.\n");

	::CloseHandle(hFile);

	return 0;
}


int Error(const char* msg) {
	printf("%s (Error: %d)\n", msg, ::GetLastError());
	return 1;
}


int PrintUsage() {
	printf("YouShallNotPassCLI.exe [AddPath | RemovePath] [path]\n");
	return 0;
}