VERSION		EQU	39
REVISION	EQU	6

DATE	MACRO
		dc.b '30.1.2008'
		ENDM

VERS	MACRO
		dc.b 'font.datatype 39.6'
		ENDM

VSTRING	MACRO
		dc.b 'font.datatype 39.6 (30.1.2008)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: font.datatype 39.6 (30.1.2008)',0
		ENDM
