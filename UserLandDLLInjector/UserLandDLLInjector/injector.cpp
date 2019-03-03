#include "injector.h"

BOOL Injector::create_process_and_inject(std::string name, std::string dll_path)
{
	if (!check_if_dll_exists(dll_path))
	{
		fprintf(stderr, "ERROR DLL MUST EXISTS FOR INJECTION");
		return FALSE;
	}

	// Structs
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// CreateProcess
	if (is_process_created = CreateProcessA(
		name.c_str(), NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, (LPSTARTUPINFOA)&si, &pi))
	{
		fprintf(stdout, "PROCESS CREATED\n");
	}
	else
	{
		fprintf(stderr, "ERROR CREATING PROCESS, ERROR NUMBER: %d\n", GetLastError());
		return FALSE;
	}

	fprintf(stdout, "Process handle: 0x%x; Process ID: 0x%x\n", pi.hProcess, pi.dwProcessId);
	
	if (!injection_check(dll_path)) return FALSE;

	// Inject DLL
	if (!inject_dll(dll_path)) return FALSE;

	return TRUE;
}

BOOL Injector::inject_to_process(std::string name, std::string dll_path)
{
	DWORD pid;

	ZeroMemory(&pi, sizeof(pi));

	if (!check_if_dll_exists(dll_path))
	{
		fprintf(stderr, "ERROR DLL MUST EXISTS FOR INJECTION");
		return FALSE;
	}

	pid = get_pid_of_process(name);

	if (pid == -1)
	{
		fprintf(stderr, "ERROR MAYBE PROCESS DOES NOT EXISTS\n");
		return FALSE;
	}

	pi.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (pi.hProcess == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "ERROR NOT POSSIBLE TO OPEN THE PROCESS");
		return FALSE;
	}

	if (!injection_check(dll_path)) return FALSE;

	return inject_dll(dll_path);
}

BOOL Injector::inject_dll(std::string path_to_dll)
{
	LPVOID remote_path, loadlibrarya;

	// No Process?
	if (!pi.hProcess) return FALSE;

	loadlibrarya = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

	remote_path = VirtualAllocEx(pi.hProcess, NULL, path_to_dll.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
	if (remote_path == NULL)
		return FALSE;

	// write path
	if (!WriteProcessMemory(pi.hProcess, (LPVOID)remote_path, path_to_dll.c_str(), path_to_dll.size(), NULL))
		return FALSE;

	if (!CreateRemoteThread(pi.hProcess,
		NULL, NULL,
		(LPTHREAD_START_ROUTINE)loadlibrarya,
		(LPVOID)remote_path,
		NULL, NULL))
	{
		fprintf(stderr, "ERROR CREATING REMOTE THREAD, ERROR NUMBER: %d\n", GetLastError());
		return FALSE;
	}

	// once dll is injected resume the thread
	ResumeThread(pi.hThread);

	return TRUE;
}

BOOL Injector::injection_check(std::string path_to_dll)
{
	process = std::make_unique<PE>(pi.hProcess);
	file = std::make_unique<PE>(path_to_dll.c_str());

	if (!process->analyze_file() || !file->analyze_file())
	{
		fprintf(stderr, "ERROR ANALYZING PROCESSES");
		return FALSE;
	}

	if (file->get_type() != PE::file_type_enum::dll_file_k)
	{
		fprintf(stderr, "ERROR FILE TO INJECT MUST BE A DLL");
		return FALSE;
	}

	if (process->get_architecture() != file->get_architecture())
	{
		fprintf(stderr, "ERROR DIFFERENT ARCHITECTURES");
		return FALSE;
	}

	return TRUE;
}

BOOL Injector::check_if_dll_exists(std::string dll_path)
{
	return PathFileExistsA(dll_path.c_str());
}

BOOL Injector::list_processes()
{
	HANDLE hProcessSnap, openedProcess;
	PROCESSENTRY32 pe32;
	std::unique_ptr<PE> pe;
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	std::string arch = "";

	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	else
	{
		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hProcessSnap, &pe32))
		{

			openedProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);

			if (openedProcess != NULL)
			{
				pe = std::make_unique<PE>(openedProcess);

				pe->analyze_file();

				if (pe->get_architecture() == PE::architecture_enum::x86_k)
					arch = "x86";
				else
					arch = "x64";

				pe->close();
			}
			else
			{
				arch = "UNK";
			}

			fprintf(stdout, "ARCH\tPID\tPPID\t\tProcess Name\n");
			fprintf(stdout, "%s\t0x%x\t0x%x\t\t%s\n", arch.c_str(), pe32.th32ProcessID, pe32.th32ParentProcessID, pe32.szExeFile);



			while (Process32Next(hProcessSnap, &pe32))
			{
				openedProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);

				if (openedProcess != NULL)
				{
					pe = std::make_unique<PE>(openedProcess);

					pe->analyze_file();

					if (pe->get_architecture() == PE::architecture_enum::x86_k)
						arch = "x86";
					else
						arch = "x64";

					pe->close();
				}
				else
				{
					arch = "UNK";
				}

				fprintf(stdout, "%s\t0x%x\t0x%x\t\t%s\n", arch.c_str(), pe32.th32ProcessID, pe32.th32ParentProcessID, pe32.szExeFile);
			}
		}
	}
	return TRUE;
}

DWORD Injector::get_pid_of_process(std::string name)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	else
	{
		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hProcessSnap, &pe32))
		{
			if (strcmp(name.c_str(), pe32.szExeFile) == 0)
				return pe32.th32ProcessID;

			while (Process32Next(hProcessSnap, &pe32))
			{
				if (strcmp(name.c_str(), pe32.szExeFile) == 0)
					return pe32.th32ProcessID;
			}
		}
	}
	return -1;
}