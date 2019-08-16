;-----------------------------------------------
;	Estructura FILETIME 
;-----------------------------------------------
FILETIME 				STRUCT

FT_dwLowDateTime				DD 			?
FT_dwHighDateTime 				DD 			?

FILETIME 				ENDS

;-----------------------------------------------
;	Estructuras de datos para ficheros
;	necesario para manejar FindFirst y FindNext
;-----------------------------------------------
WIN32_FIND_DATA			STRUCT

WFD_dwFileAttributes			DD			?
WFD_ftCreationTime				FILETIME	1 DUP (<>)
WFD_ftLastAccessTime			FILETIME	1 DUP (<>)
WFD_ftLastWriteTime				FILETIME 	1 DUP (<>)
WFD_nFileSizeHigh				DD			?
WFD_nFileSizeLow 				DD			?
WFD_dwReserved0					DD			?
WFD_dwReserved1					DD			?
WFD_szFileName					DB	260		DUP	(?)
WFD_szAlternateFileName			DB	13		DUP	(?)
DB	3	DUP	(?)	; PADDING 

WIN32_FIND_DATA			ENDS
