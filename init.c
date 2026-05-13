#include "classbase.h"

/* Global Pointers */
struct Library *SysBase = NULL;
struct ExecIFace *IExec = NULL;
struct Library *DOSBase = NULL;
struct DOSIFace *IDOS = NULL;
struct Library *IntuitionBase = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct Library *GraphicsBase = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct Library *DataTypesBase = NULL;
struct DataTypesIFace *IDataTypes = NULL;
struct Library *DiskfontBase = NULL;
struct DiskfontIFace *IDiskfont = NULL;
struct Library *LocaleBase = NULL;
struct LocaleIFace *ILocale = NULL;
struct Library *FT2Base = NULL;
struct Interface *IFT2 = NULL;
struct Library *UtilityBase = NULL;
struct UtilityIFace *IUtility = NULL;
struct Library *NewlibBase = NULL;
struct Interface *INewlib = NULL;
struct Library *IFFParseBase = NULL;
struct IFFParseIFace *IIFFParse = NULL;

/* Prototypes */
extern Class *_DTClass_ObtainEngine(struct DTClassIFace *Self);
extern Class *InitClass(struct ClassBase *cb);

#include "font.datatype_rev.h"
#define DATATYPENAME "font.datatype"
#define FULLVERSION VERS " (" DATE ")"
#define FULLID VSTRING

/* _start stub for ELF loader */
int _start(void)
{
    return 0;
}

/* Library Manager Interface Implementation */
uint32 _manager_Obtain(struct LibraryManagerInterface *Self)
{
    Self->Data.RefCount++;
    return Self->Data.RefCount;
}

uint32 _manager_Release(struct LibraryManagerInterface *Self)
{
    Self->Data.RefCount--;
    return Self->Data.RefCount;
}

struct Library * _manager_Open(struct LibraryManagerInterface *Self, uint32 version)
{ 
    struct ClassBase *cb = (struct ClassBase *)Self->Data.LibBase;
    if (IExec) IExec->DebugPrintF("[font.datatype] _manager_Open version %u\n", version);
    cb->cb_Lib.lib_OpenCnt++;
    return (struct Library *)cb; 
}

APTR _manager_Close(struct LibraryManagerInterface *Self)
{ 
    struct ClassBase *cb = (struct ClassBase *)Self->Data.LibBase;
    if (IExec) IExec->DebugPrintF("[font.datatype] _manager_Close\n");
    cb->cb_Lib.lib_OpenCnt--;
    return NULL; 
}

APTR _manager_LibExpunge(struct LibraryManagerInterface *Self)
{
    struct ClassBase *cb = (struct ClassBase *)Self->Data.LibBase;
    BPTR seglist = cb->cb_SegList;

    if (IExec) IExec->DebugPrintF("[font.datatype] _manager_LibExpunge entry\n");

    if (cb->cb_Lib.lib_OpenCnt == 0) {
        if (cb->cb_Class && IIntuition) {
            IIntuition->FreeClass(cb->cb_Class);
        }

        if (INewlib) { IExec->DropInterface(INewlib); INewlib = NULL; }
        if (NewlibBase) { IExec->CloseLibrary(NewlibBase); NewlibBase = NULL; }
        if (IIFFParse) { IExec->DropInterface((struct Interface *)IIFFParse); IIFFParse = NULL; }
        if (IFFParseBase) { IExec->CloseLibrary(IFFParseBase); IFFParseBase = NULL; }
        if (IUtility) { IExec->DropInterface((struct Interface *)IUtility); IUtility = NULL; }
        if (UtilityBase) { IExec->CloseLibrary(UtilityBase); UtilityBase = NULL; }
        if (IDataTypes) { IExec->DropInterface((struct Interface *)IDataTypes); IDataTypes = NULL; }
        if (DataTypesBase) { IExec->CloseLibrary(DataTypesBase); DataTypesBase = NULL; }
        if (IDiskfont) { IExec->DropInterface((struct Interface *)IDiskfont); IDiskfont = NULL; }
        if (DiskfontBase) { IExec->CloseLibrary(DiskfontBase); DiskfontBase = NULL; }
        if (ILocale) { IExec->DropInterface((struct Interface *)ILocale); ILocale = NULL; }
        if (LocaleBase) { IExec->CloseLibrary(LocaleBase); LocaleBase = NULL; }
        if (IFT2) { IExec->DropInterface(IFT2); IFT2 = NULL; }
        if (FT2Base) { IExec->CloseLibrary(FT2Base); FT2Base = NULL; }
        if (IGraphics) { IExec->DropInterface((struct Interface *)IGraphics); IGraphics = NULL; }
        if (GraphicsBase) { IExec->CloseLibrary(GraphicsBase); GraphicsBase = NULL; }
        if (IIntuition) { IExec->DropInterface((struct Interface *)IIntuition); IIntuition = NULL; }
        if (IntuitionBase) { IExec->CloseLibrary(IntuitionBase); IntuitionBase = NULL; }
        if (IDOS) { IExec->DropInterface((struct Interface *)IDOS); IDOS = NULL; }
        if (DOSBase) { IExec->CloseLibrary(DOSBase); DOSBase = NULL; }

        IExec->Remove(&cb->cb_Lib.lib_Node);
        IExec->DeleteLibrary(&cb->cb_Lib);
        return (APTR)seglist;
    }
    cb->cb_Lib.lib_Flags |= LIBF_DELEXP;
    return 0;
}

/* 
 * Library Manager Vectors 
 * Order: Obtain, Release, Expunge, Clone, Open, Close, LibExpunge, GetInterface
 */
APTR lib_manager_vectors[] = {
    (APTR)_manager_Obtain,     /* 0 */
    (APTR)_manager_Release,    /* 1 */
    (APTR)NULL,                /* 2: Expunge */
    (APTR)NULL,                /* 3: Clone */
    (APTR)_manager_Open,       /* 4: Open */
    (APTR)_manager_Close,      /* 5: Close */
    (APTR)_manager_LibExpunge, /* 6: LibExpunge */
    (APTR)NULL,                /* 7: GetInterface */
    (APTR)-1                   /* terminator */
};

struct TagItem lib_manager_tags[] = {
    { MIT_Name, (Tag)"__library" }, { MIT_VectorTable, (Tag)lib_manager_vectors }, { MIT_Version, 1 }, { TAG_DONE, 0 }
};

/* 
 * Main Interface (DTClassIFace)
 * Order: Obtain, Release, Expunge, Clone, ObtainEngine
 * IDENTIFY IS NOT PART OF THIS INTERFACE.
 */
uint32 _main_Obtain(struct Interface *Self) { return 0; }
uint32 _main_Release(struct Interface *Self) { return 0; }

APTR main_vectors[] = {
    (APTR)_main_Obtain,   /* 0 */
    (APTR)_main_Release,  /* 1 */
    (APTR)NULL,           /* 2: Expunge */
    (APTR)NULL,           /* 3: Clone */
    (APTR)_DTClass_ObtainEngine, /* 4: ObtainEngine - CRITICAL FIX */
    (APTR)-1              /* terminator */
};

struct TagItem main_tags[] = {
    { MIT_Name, (Tag)"main" }, { MIT_VectorTable, (Tag)main_vectors }, { MIT_Version, 1 }, { TAG_DONE, 0 }
};

uint32 libInterfaces[] = { (uint32)lib_manager_tags, (uint32)main_tags, (uint32)NULL };

struct Library * libInit(struct Library *LibraryBase, BPTR seglist, struct Interface *exec)
{
    struct ClassBase *cb = (struct ClassBase *)LibraryBase;
    IExec = (struct ExecIFace *)exec;
    SysBase = (struct Library *)((uint32)IExec->Data.LibBase);

    IExec->DebugPrintF("[font.datatype] libInit " FULLVERSION " entry\n");

    cb->cb_Lib.lib_Node.ln_Type = NT_LIBRARY;
    cb->cb_Lib.lib_Node.ln_Name = (STRPTR)DATATYPENAME;
    cb->cb_Lib.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    cb->cb_Lib.lib_Version      = VERSION;
    cb->cb_Lib.lib_Revision     = REVISION;
    cb->cb_Lib.lib_IdString     = (APTR)FULLID;
    cb->cb_SegList = seglist;

    /* Trace Dependencies */
    NewlibBase = IExec->OpenLibrary("newlib.library", 52);
    if (NewlibBase) INewlib = IExec->GetInterface(NewlibBase, "main", 1, NULL);
    if (!INewlib) { IExec->DebugPrintF("[font.datatype] ERROR: newlib failed\n"); return NULL; }

    DOSBase = IExec->OpenLibrary("dos.library", 52);
    if (DOSBase) IDOS = (struct DOSIFace *)IExec->GetInterface(DOSBase, "main", 1, NULL);
    if (!IDOS) { IExec->DebugPrintF("[font.datatype] ERROR: dos failed\n"); return NULL; }

    IntuitionBase = IExec->OpenLibrary("intuition.library", 52);
    if (IntuitionBase) IIntuition = (struct IntuitionIFace *)IExec->GetInterface(IntuitionBase, "main", 1, NULL);
    if (!IIntuition) { IExec->DebugPrintF("[font.datatype] ERROR: intuition failed\n"); return NULL; }

    GraphicsBase = IExec->OpenLibrary("graphics.library", 52);
    if (GraphicsBase) IGraphics = (struct GraphicsIFace *)IExec->GetInterface(GraphicsBase, "main", 1, NULL);
    if (!IGraphics) { IExec->DebugPrintF("[font.datatype] ERROR: graphics failed\n"); return NULL; }

    DataTypesBase = IExec->OpenLibrary("datatypes.library", 52);
    if (DataTypesBase) IDataTypes = (struct DataTypesIFace *)IExec->GetInterface(DataTypesBase, "main", 1, NULL);
    if (!IDataTypes) { IExec->DebugPrintF("[font.datatype] ERROR: datatypes failed\n"); return NULL; }

    DiskfontBase = IExec->OpenLibrary("diskfont.library", 52);
    if (DiskfontBase) IDiskfont = (struct DiskfontIFace *)IExec->GetInterface(DiskfontBase, "main", 1, NULL);
    if (!IDiskfont) { IExec->DebugPrintF("[font.datatype] ERROR: diskfont failed\n"); return NULL; }

    LocaleBase = IExec->OpenLibrary("locale.library", 52);
    if (LocaleBase) ILocale = (struct LocaleIFace *)IExec->GetInterface(LocaleBase, "main", 1, NULL);
    cb->cb_LocaleBase = LocaleBase;
    cb->cb_ILocale = ILocale;

    FT2Base = IExec->OpenLibrary("ft2.library", 52);
    if (FT2Base) IFT2 = IExec->GetInterface(FT2Base, "main", 1, NULL);
    /* Note: ft2.library is optional for now, we won't fail if it's missing but we will log it */
    if (!IFT2) { IExec->DebugPrintF("[font.datatype] WARNING: ft2.library failed to open main interface\n"); }

    UtilityBase = IExec->OpenLibrary("utility.library", 52);
    if (UtilityBase) IUtility = (struct UtilityIFace *)IExec->GetInterface(UtilityBase, "main", 1, NULL);
    if (!IUtility) { IExec->DebugPrintF("[font.datatype] ERROR: utility failed\n"); return NULL; }

    IFFParseBase = IExec->OpenLibrary("iffparse.library", 52);
    if (IFFParseBase) IIFFParse = (struct IFFParseIFace *)IExec->GetInterface(IFFParseBase, "main", 1, NULL);
    if (!IIFFParse) { IExec->DebugPrintF("[font.datatype] ERROR: iffparse failed\n"); return NULL; }

    cb->cb_Class = InitClass(cb);
    if (!cb->cb_Class) { IExec->DebugPrintF("[font.datatype] ERROR: class init failed\n"); return NULL; }

    IExec->DebugPrintF("[font.datatype] libInit SUCCESSFUL (FT2Base=%p)\n", FT2Base);
    return (struct Library *)cb;
}

struct TagItem libCreateTags[] = {
    { CLT_DataSize, (uint32)sizeof(struct ClassBase) },
    { CLT_InitFunc, (uint32)libInit },
    { CLT_Interfaces, (uint32)libInterfaces },
    { TAG_DONE, 0 }
};

struct Resident lib_res __attribute__((used)) = {
    RTC_MATCHWORD,
    &lib_res,
    &lib_res + 1,
    RTF_NATIVE | RTF_AUTOINIT,
    DATATYPEVERSION,
    NT_LIBRARY,
    0,
    (STRPTR)DATATYPENAME,
    (STRPTR)FULLID,
    libCreateTags
};
