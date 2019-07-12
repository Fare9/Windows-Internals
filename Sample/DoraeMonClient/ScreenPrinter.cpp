#include "pch.h"
#include "ScreenPrinter.h"

HANDLE hOut;

std::map<ULONG, std::wstring> processes;

float version = 0.2;

char DORAEMON[] = "                                                            \n \
                               /@&&%#((((##%&@&,                               \n \
                        &%((&(    *&(((((((@    .@(((&(                        \n \
                   .&(((((.          @(((           &(((((@                    \n \
                ##((((((@             @(             .(((((((&                 \n \
              &(((((((((               &              &(((((((((@              \n \
           .(((((((((((&         %.@   (  .%.@        ,(((((((((((&            \n \
          #(((((((((((((       @    .  @ @    (       @(((((((((((((@          \n \
        @((((((((((((@( @             @               &@%((((((((((((#         \n \
       %(((((((((@       %          @%%%%@          @       .@((((((((((       \n \
      ((((((((@             &&*.*&@%%   #%%@@#,,#@,             @(((((((&      \n \
     #((((((@ #                   %%%. /%%%%&                     @(((((((     \n \
    &(((((&        &&             %%%%%%%%%%*             /@*       %(((((     \n \
   *(((((@               @/        %%%%%%%%/        %@               @((((@    \n \
   %((((&                     @.     .&&/     %&                      &((((    \n \
   ((((#                                                               ((((@   \n \
  (((((&                                                               @(((&   \n \
  %((((                                                                *(((%   \n \
  %(((#                                                                 (((%   \n \
  .(((%      @                                                   /      (((&   \n \
   (((#       (     ,%@@%,                           (@@#.      &       (((@   \n \
   @(((   *     @                                             @  /&@   &(((    \n \
    (((@          #                                         @          %((@    \n \
    @(((            @                                    .#           ,(((     \n \
     %((@             ,#                               @              %((/     \n \
      (((,                @                        ##                @((@      \n \
       (((                    #@.             /@.                   %((@       \n \
        #((                                                        /((%        \n \
         &((                                                      %((          \n \
          /(((                                                   @(@           \n \
            @(@                                                 %(.            \n \
              &(.                                             &(%              \n \
              %%%%%%%%%&&&&&&@@@@@@@@@@@@@@@@@@@@@@&&&&&%%%%%%%%&              ";


VOID PrintDoraemon()
{
	printf("%s\n", DORAEMON);
	printf("\t\tDoraeMON Process Monitor by %s Version %.1f\n\n", "Fare9", version);
}


VOID PrintHeader()
{
	SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	ClearScreen();
	PrintDoraemon();
}

VOID PrintStructure(BYTE* buffer, ItemHeader* item)
{

	SetConsoleTextAttribute(hOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	SYSTEMTIME systemtime;

	FileTimeToSystemTime((FILETIME*)&item->Time, &systemtime);
	auto hour = systemtime.wHour;
	auto minute = systemtime.wMinute;
	auto second = systemtime.wSecond;
	
	printf("================================>");
	switch (item->Type)
	{
	case ItemType::ProcessCreate:
	{
		printf(" PROCESS CREATE\n");
		auto processInfo = (ProcessCreateInfo*)item;

		printf("\tProcess ID: %X(%d)\n", processInfo->ProcessId, processInfo->ProcessId);
		printf("\tParent PID: %X(%d)\n", processInfo->ParentProcessId, processInfo->ParentProcessId);
		printf("\tSize: %X(%d)\n", processInfo->size, processInfo->size);
		
		
		printf("\tTime: %02d:%02d:%02d\n", hour, minute, second);

		std::wstring CommandLine((WCHAR*)(buffer + processInfo->CommandLineOffset), processInfo->CommandLineLength);

		std::wstring ImageFileName((WCHAR*)(buffer + processInfo->ImageFileNameOffset), processInfo->ImageFileNameLength);

		wprintf(L"\tIMAGEFILENAME: %-256ws\n\tCOMMANDLINE: %-256ws\n", ImageFileName.c_str(), CommandLine.c_str());

		processes.insert(std::pair<ULONG, std::wstring>(processInfo->ProcessId, ImageFileName));
		break;
	}
	case  ItemType::ProcessExit:
	{
		printf(" PROCESS EXIT\n");

		auto processInfo = (ProcessExitInfo*)item;


		printf("\tProcess ID: %X(%d)\n", processInfo->ProcessId, processInfo->ProcessId);
		printf("\tParent PID: %s(%s)\n", "None", "None");
		printf("\tSize: %X(%d)\n", processInfo->size, processInfo->size);
		printf("\tTime: %02d:%02d:%02d\n", hour, minute, second);

		if (processes.find(processInfo->ProcessId) != processes.end())
		{
			printf("\tIMAGEFILENAME: %-256ws\n", processes[processInfo->ProcessId].c_str());
			processes.erase(processInfo->ProcessId);
		}
		
		break;
	}
	case ItemType::ThreadCreate:
	{
		printf(" THREAD CREATE\n");

		auto threadinfo = (ThreadCreateExitInfo*)buffer;

		printf("\tThread ID: %X(%d)\n", threadinfo->ThreadId, threadinfo->ThreadId);
		printf("\tProcess ID: %X(%d)\n", threadinfo->ProcessId, threadinfo->ProcessId);
		printf("\tParent PID: %s(%s)\n", "None", "None");
		printf("\tSize: %X(%d)\n", threadinfo->size, threadinfo->size);
		printf("\tTime: %02d:%02d:%02d\n", hour, minute, second);

		if (processes.find(threadinfo->ProcessId) != processes.end())
		{
			printf("\tIMAGEFILENAME: %-256ws\n", processes[threadinfo->ProcessId].c_str());
		}
		break;
	}
	case ItemType::ThreadExit:
	{
		printf(" THREAD EXIT\n");

		auto threadinfo = (ThreadCreateExitInfo*)buffer;

		printf("\tThread ID: %X(%d)\n", threadinfo->ThreadId, threadinfo->ThreadId);
		printf("\tProcess ID: %X(%d)\n", threadinfo->ProcessId, threadinfo->ProcessId);
		printf("\tParent PID: %s(%s)\n", "None", "None");
		printf("\tSize: %X(%d)\n", threadinfo->size, threadinfo->size);
		printf("\tTime: %02d:%02d:%02d\n", hour, minute, second);

		if (processes.find(threadinfo->ProcessId) != processes.end())
		{
			printf("\tIMAGEFILENAME: %-256ws\n", processes[threadinfo->ProcessId].c_str());
		}
		break;
	}
	default:
		break;
	}
}

VOID ClearScreen()
{
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	// Get the number of character cells in the current buffer. 

	if (!GetConsoleScreenBufferInfo(hOut, &csbi))
	{
		return;
	}

	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.

	if (!FillConsoleOutputCharacter(hOut,        // Handle to console screen buffer 
		(TCHAR) ' ',     // Character to write to the buffer
		dwConSize,       // Number of cells to write 
		coordScreen,     // Coordinates of first cell 
		&cCharsWritten))// Receive number of characters written
	{
		return;
	}

	// Get the current text attribute.

	if (!GetConsoleScreenBufferInfo(hOut, &csbi))
	{
		return;
	}

	// Set the buffer's attributes accordingly.

	if (!FillConsoleOutputAttribute(hOut,         // Handle to console screen buffer 
		csbi.wAttributes, // Character attributes to use
		dwConSize,        // Number of cells to set attribute 
		coordScreen,      // Coordinates of first cell 
		&cCharsWritten)) // Receive number of characters written
	{
		return;
	}

	// Put the cursor at its home coordinates.

	SetConsoleCursorPosition(hOut, coordScreen);
}