

#include "pch.h"

#include "../ProcessProtect/ProcessProtectCommon.h"

int Error(const char* msg);
int PrintUsage();
std::vector<DWORD> ParsePids(const wchar_t* buffer[], int count);


int wmain(int argc, const wchar_t *argv[])
{
	if (argc < 2)
		return PrintUsage();

	enum class Options 
	{
		Unknown,
		Add,
		Remove,
		Clear,
		Query
	};

	Options option;

	if (::_wcsicmp(argv[1], L"add") == 0)
		option = Options::Add;
	else if (::_wcsicmp(argv[1], L"remove") == 0)
		option = Options::Remove;
	else if (::_wcsicmp(argv[1], L"clear") == 0)
		option = Options::Clear;
	else if (::_wcsicmp(argv[1], L"query") == 0)
		option = Options::Query;
	else
	{
		printf("Unknown option.\n");
		return PrintUsage();
	}

	HANDLE hFile = ::CreateFile(L"\\\\.\\" PROCESS_PROTECT_NAME, GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	std::vector<DWORD> pids;

	BOOL success = FALSE;

	DWORD bytes;

	switch (option)
	{
	case Options::Add:
		pids = ParsePids(argv + 2, argc - 2);
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_PROTECT_BY_PID, pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD), nullptr, 0, &bytes, nullptr);
		break;
	case Options::Remove:
		pids = ParsePids(argv + 2, argc - 2);
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_UNPROTECT_BY_PID, pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD), nullptr, 0, &bytes, nullptr);
		break;
	case Options::Clear:
		success = ::DeviceIoControl(hFile, IOCTL_PROCESS_PROTECT_CLEAR, pids.data(), static_cast<DWORD>(pids.size()) * sizeof(DWORD), nullptr, 0, &bytes, nullptr);
		break;
	case Options::Query:
	{
		ULONG *pid;
		ULONG size = 0;

		success  = ::DeviceIoControl(hFile, IOCTL_PROCESS_QUERY_PIDS, &size, sizeof(ULONG), &size, sizeof(ULONG), &bytes, nullptr);

		printf("First DeviceIoControl error = %X & bytes = %X\n", GetLastError(), bytes);

		if (size != 0)
		{
			pid = (ULONG*)malloc(size);

			success = ::DeviceIoControl(hFile, IOCTL_PROCESS_QUERY_PIDS, nullptr, 0, pid, size, &bytes, nullptr);

			printf("Second DeviceIoControl error = %X & bytes = %X\n", GetLastError(), bytes);

			if (GetLastError() == ERROR_SUCCESS)
			{
				for (size_t i = 0; i < (bytes / sizeof(ULONG)); i++)
				{
					printf("%d ", pid[i]);
				}
				printf("\n");
			}
		}
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
	printf("Protect [add | remove | clear | query] [pid] ...\n");
	return 0;
}


std::vector<DWORD> ParsePids(const wchar_t* buffer[], int count) {
	std::vector<DWORD> pids;
	for (int i = 0; i < count; i++)
		pids.push_back(::_wtoi(buffer[i]));
	return pids;
}