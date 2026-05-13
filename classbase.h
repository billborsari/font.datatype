#ifndef CLASSBASE_H
#define CLASSBASE_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/interfaces.h>
#include <exec/resident.h>
#include <stdint.h>
#include <dos/dos.h>
#include <intuition/classes.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <diskfont/diskfonttag.h>



#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>
#include <proto/utility.h>
#include <interfaces/iffparse.h>
#include <interfaces/datatypes.h>
#include <interfaces/diskfont.h>

struct DTClassIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)(struct DTClassIFace *Self);
	ULONG APICALL (*Release)(struct DTClassIFace *Self);
	void APICALL (*Expunge)(struct DTClassIFace *Self);
	struct Interface * APICALL (*Clone)(struct DTClassIFace *Self);
	Class * APICALL (*ObtainEngine)(struct DTClassIFace *Self);
};

/* Global Pointers required for newlib and internal use */
extern struct Library *SysBase;
extern struct ExecIFace *IExec;
extern struct Library *DOSBase;
extern struct DOSIFace *IDOS;
extern struct Library *IntuitionBase;
extern struct IntuitionIFace *IIntuition;
extern struct Library *GraphicsBase;
extern struct GraphicsIFace *IGraphics;
extern struct Library *DataTypesBase;
extern struct DataTypesIFace *IDataTypes;
extern struct Library *DiskfontBase;
extern struct DiskfontIFace *IDiskfont;
extern struct Library *LocaleBase;
extern struct LocaleIFace *ILocale;
extern struct Library *FT2Base;
extern struct Interface *IFT2;
extern struct Library *UtilityBase;
extern struct UtilityIFace *IUtility;
extern struct Library *NewlibBase;
extern struct Interface *INewlib;
extern struct Library *IFFParseBase;
extern struct IFFParseIFace *IIFFParse;

struct ClassBase {
    struct Library cb_Lib;
    BPTR cb_SegList;
    Class *cb_Class;
    struct SignalSemaphore cb_Lock;

    struct Library *cb_LocaleBase;
    struct LocaleIFace *cb_ILocale;
};

struct InstanceData {
    struct SignalSemaphore id_Lock;
    struct TextFont *id_Font;
    struct BitMap *id_BitMap;
    uint32 id_Width;
    uint32 id_Height;
    uint32 id_Depth;
    char id_Name[256];
    struct ColorMap *id_ColorMap;      /* Optional: graphics ColorMap for this palette */
    struct IFF_BitMapHeader id_BMHD;   /* Persistent copy of BMHD for superclass */
    ULONG id_CRegs[6];                 /* Persistent color table (Index 0: Black, Index 1: White) */
};

#define DATATYPENAME "font.datatype"
#define DATATYPEVERSION 54
#define DATATYPEREVISION 1

#define DPRINTF(fmt, ...) IExec->DebugPrintF("[%s] " fmt, DATATYPENAME, ##__VA_ARGS__)

#endif
