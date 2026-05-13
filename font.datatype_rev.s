VERSION = 39
REVISION = 6

.macro DATE
.ascii "30.1.2008"
.endm

.macro VERS
.ascii "font.datatype 39.6"
.endm

.macro VSTRING
.ascii "font.datatype 39.6 (30.1.2008)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: font.datatype 39.6 (30.1.2008)"
.byte 0
.endm
