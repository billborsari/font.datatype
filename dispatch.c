#include "classbase.h"
#include <string.h>
#include <libraries/iffparse.h>
#include <graphics/modeid.h>

/* IFF ILBM Macros */
#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((uint32)(a)<<24 | (uint32)(b)<<16 | (uint32)(c)<<8 | (uint32)(d))
#endif
#define ID_ILBM MAKE_ID('I','L','B','M')
#define ID_BMHD MAKE_ID('B','M','H','D')
#define ID_CMAP MAKE_ID('C','M','A','P')
#define ID_BODY MAKE_ID('B','O','D','Y')


/* ObtainEngine hook */
Class *_DTClass_ObtainEngine(struct DTClassIFace *Self)
{
    struct ClassBase *cb = (struct ClassBase *)Self->Data.LibBase;
    return (Class *)cb->cb_Class;
}




/* Helper to open the best possible font for a given size and path */
static struct TextFont *OpenBestFont(STRPTR filename, uint16 size, const char **engine_name)
{
    struct TextFont *tf = NULL;
    STRPTR name = (STRPTR)IDOS->FilePart(filename);

    LogDebug("OpenBestFont: Attempting to load '%s' at size %d", filename, size);

    /* Try 1: Standard OpenDiskFont (.font or installed) */
    struct TTextAttr tta1 = { name, size, FSF_TAGGED, FPF_DISKFONT, NULL };
    tf = IDiskfont->OpenDiskFont((struct TextAttr *)&tta1);
    if (tf) { 
        LogDebug("OpenBestFont: SUCCESS via Standard OpenDiskFont");
        if (engine_name) *engine_name = "diskfont.library (Standard)"; 
        return tf; 
    }

    /* Try 2: Full path (uninstalled .font) */
    struct TTextAttr tta2 = { filename, size, FSF_TAGGED, FPF_DISKFONT, NULL };
    tf = IDiskfont->OpenDiskFont((struct TextAttr *)&tta2);
    if (tf) { 
        LogDebug("OpenBestFont: SUCCESS via Path OpenDiskFont");
        if (engine_name) *engine_name = "diskfont.library (Standard/Path)"; 
        return tf; 
    }

    /* Try 3: Direct FreeType forcing */
    char font_name[256];
    IUtility->SNPrintf(font_name, 256, "%s.font", name);

    struct TagItem ft_tags[] = { 
        { OT_DeviceDPI, (72 << 16) | 72 },
        { OT_Engine,    (uintptr_t)"freetype" }, 
        { OT_FontFile,  (uintptr_t)filename },
        { OT_FontFormat, (uintptr_t)"truetype" },
        { OT_PointHeight, (uint32)(size << 16) }, /* Fixed point 16.16 */
        { TAG_DONE,     0 } 
    };
    struct TTextAttr tta3 = { font_name, size, FSF_TAGGED, FPF_DISKFONT, ft_tags };
    tf = IDiskfont->OpenDiskFont((struct TextAttr *)&tta3);
    if (tf) { 
        LogDebug("OpenBestFont: SUCCESS via FreeType forcing (name=%s)", font_name);
        if (engine_name) *engine_name = "diskfont.library (FreeType Engine)"; 
        return tf; 
    }

    /* Try 4: Direct Bullet forcing */
    struct TagItem bullet_tags[] = { 
        { OT_DeviceDPI, (72 << 16) | 72 },
        { OT_Engine,    (uintptr_t)"bullet" }, 
        { OT_FontFile,  (uintptr_t)filename },
        { TAG_DONE,     0 } 
    };
    struct TTextAttr tta4 = { name, size, FSF_TAGGED, FPF_DISKFONT, bullet_tags };
    tf = IDiskfont->OpenDiskFont((struct TextAttr *)&tta4);
    if (tf) { 
        LogDebug("OpenBestFont: SUCCESS via Bullet forcing");
        if (engine_name) *engine_name = "diskfont.library (Bullet Engine)"; 
        return tf; 
    }

    /* Try 5: Legacy non-tagged */
    struct TextAttr ta5 = { filename, size, 0, 0 };
    tf = IDiskfont->OpenDiskFont(&ta5);
    if (tf) { 
        LogDebug("OpenBestFont: SUCCESS via Legacy OpenDiskFont");
        if (engine_name) *engine_name = "diskfont.library (Legacy)"; 
        return tf; 
    }

    /* Final fallback: Topaz */
    LogDebug("OpenBestFont: ALL modern paths failed, falling back to Topaz");
    struct TextAttr ta6 = { "topaz.font", 8, 0, 0 };
    tf = IGraphics->OpenFont(&ta6);
    if (tf) { if (engine_name) *engine_name = "graphics.library (Fallback Topaz)"; }
    
    return tf;
}

/* Method: OM_NEW */
static uint32 Method_New(Class *cl, Object *o, struct opSet *msg)
{
    Object *newobj;
    struct InstanceData *id;
    STRPTR filename = (STRPTR)IUtility->GetTagData(DTA_Name, 0, msg->ops_AttrList);

    DPRINTF("OM_NEW: %s\n", filename ? filename : "NULL");
    
    /*
     * Pass DTA_GroupID=GID_PICTURE so picture.datatype superclass knows the type.
     * Do NOT set DTA_SourceType here — let it default from the file handle that
     * datatypes.library already set up. We will NOT pass DTST_MEMORY because that
     * mode has a completely different contract (DTA_SourceAddress + DTA_SourceSize).
     * Instead we rely on DTST_FILE (which the caller already established via the
     * file handle) and supply our own ClassBitMap so picture.datatype skips its
     * own file read and just remaps our bitmap.
     */
    struct TagItem extra[] = {
        {DTA_GroupID,    GID_PICTURE},
        {TAG_MORE,       (uintptr_t)msg->ops_AttrList}
    };
    struct opSet new_msg = {OM_NEW, extra};

    newobj = (Object *)IIntuition->IDoSuperMethodA(cl, o, (Msg)&new_msg);

    if (!newobj) {
        DPRINTF("OM_NEW: Superclass fail\n");
        return 0;
    }

    id = (struct InstanceData *)INST_DATA(cl, newobj);
    IExec->InitSemaphore(&id->id_Lock);
    id->id_Font    = NULL;
    id->id_BitMap  = NULL;
    id->id_ColorMap = NULL;

    if (filename) {
        IUtility->Strlcpy(id->id_Name, filename, 256);
    }

    /* Preview configuration with localization support */
    const char *preview_text = "The quick brown fox jumps over the lazy dog 0123456789 !@#$%^&*()_+";
    struct Locale *loc = NULL;
    
    if (ILocale) {
        loc = ILocale->OpenLocale(NULL);
        if (loc) {
            if (IUtility->Stricmp(loc->loc_LanguageName, "french") == 0) {
                preview_text = "Portez ce vieux morphion juge en fureur qui tue 0123456789 !@#$%^&*()_+";
            } else if (IUtility->Stricmp(loc->loc_LanguageName, "german") == 0) {
                preview_text = "Franz jagt im komplett verwahrlosten Taxi quer durch Bayern 0123456789 !@#$%^&*()_+";
            } else if (IUtility->Stricmp(loc->loc_LanguageName, "spanish") == 0) {
                preview_text = "El veloz murciélago hindú comía feliz cardillo y escabeche 0123456789 !@#$%^&*()_+";
            } else if (IUtility->Stricmp(loc->loc_LanguageName, "polish") == 0) {
                preview_text = "Pchnąć w tę łódź jeża lub ośm skrzyń fig 0123456789 !@#$%^&*()_+";
            }
            ILocale->CloseLocale(loc);
        }
    }

    static const uint16 preview_sizes[] = {11, 15, 20, 36, 50};
    const int num_sizes = sizeof(preview_sizes) / sizeof(preview_sizes[0]);
    
    /* Calculate total height and max width */
    id->id_Width  = 640;
    id->id_Height = 0;
    id->id_Depth  = 1;

    /* Add space for the engine note at the top (size 12 + margin) */
    id->id_Height += 12 + 12;

    const char *final_engine_name = "Unknown";
    BOOL first_open_success = FALSE;

    /* Loop 1: Measure height */
    for (int i = 0; i < num_sizes; i++) {
        struct TextFont *temp_font = OpenBestFont(filename, preview_sizes[i], &final_engine_name);
        if (temp_font) {
            if (!first_open_success) {
                DPRINTF("OM_NEW: Engine %s\n", final_engine_name);
                first_open_success = TRUE;
            }
            id->id_Height += temp_font->tf_YSize + 8;
            IGraphics->CloseFont(temp_font);
        }
    }
    id->id_Height += 8; /* Bottom margin */

    /* Generate the preview BitMap */
    id->id_BitMap = IGraphics->AllocBitMap(id->id_Width, id->id_Height, id->id_Depth,
                                            BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (id->id_BitMap) {
        struct RastPort rp;
        IGraphics->InitRastPort(&rp);
        rp.BitMap = id->id_BitMap;

        /* Fill background (White) */
        IGraphics->SetAPen(&rp, 0);
        IGraphics->RectFill(&rp, 0, 0, id->id_Width - 1, id->id_Height - 1);
        IGraphics->SetAPen(&rp, 1); /* Black text */

        /* Render engine note at the top */
        struct TextAttr note_ta = { "topaz.font", 8, 0, 0 };
        struct TextFont *note_font = IGraphics->OpenFont(&note_ta);
        if (note_font) {
            IGraphics->SetFont(&rp, note_font);
            char note_buf[128];
            IUtility->SNPrintf(note_buf, 128, "Engine used: %s", final_engine_name);
            IGraphics->Move(&rp, 8, 4 + note_font->tf_Baseline);
            IGraphics->Text(&rp, note_buf, strlen(note_buf));
            IGraphics->CloseFont(note_font);
        }

        int current_y = 24;
        for (int i = 0; i < num_sizes; i++) {
            struct TextFont *temp_font = OpenBestFont(filename, preview_sizes[i], NULL);
            if (temp_font) {
                IGraphics->SetFont(&rp, temp_font);
                IGraphics->Move(&rp, 8, current_y + temp_font->tf_Baseline);
                IGraphics->Text(&rp, preview_text, strlen(preview_text));
                
                current_y += temp_font->tf_YSize + 8;
                IGraphics->CloseFont(temp_font);
            }
        }
    } 

    if (!id->id_BitMap) {
        DPRINTF("OM_NEW: BitMap alloc failed\n");
        IIntuition->IDoMethod(newobj, OM_DISPOSE);
        return 0;
    }

    /* Initialize persistent color table */
    id->id_CRegs[0] = 0; id->id_CRegs[1] = 0; id->id_CRegs[2] = 0; /* Black */
    id->id_CRegs[3] = 0xFFFFFFFF; id->id_CRegs[4] = 0xFFFFFFFF; id->id_CRegs[5] = 0xFFFFFFFF; /* White */

    /* Initialize persistent BMHD */
    memset(&id->id_BMHD, 0, sizeof(struct IFF_BitMapHeader));
    id->id_BMHD.w = id->id_Width;
    id->id_BMHD.h = id->id_Height;
    id->id_BMHD.nPlanes = id->id_Depth;
    id->id_BMHD.xAspect = 10;
    id->id_BMHD.yAspect = 11;
    id->id_BMHD.pageWidth = id->id_Width;
    id->id_BMHD.pageHeight = id->id_Height;

    /*
     * Now tell picture.datatype about our image via SetAttrs.
     */
    IDataTypes->SetDTAttrs(newobj, NULL, NULL,
        DTA_ObjName,         (uintptr_t)id->id_Name,
        DTA_NominalHoriz,    (uint32)id->id_Width,
        DTA_NominalVert,     (uint32)id->id_Height,
        PDTA_NumColors,      (uint32)2,
        PDTA_ClassBitMap,    (uintptr_t)id->id_BitMap,
        PDTA_BitMap,         (uintptr_t)id->id_BitMap,
        PDTA_SourceMode,     (uint32)PMODE_V42,
        TAG_END);

    /* Now retrieve the internally allocated structures and populate them */
    struct BitMapHeader *bmh = NULL;
    struct ColorRegister *creg = NULL;
    uint32 *creg32 = NULL;

    IDataTypes->GetDTAttrs(newobj,
        PDTA_BitMapHeader,   (uintptr_t)&bmh,
        PDTA_ColorRegisters, (uintptr_t)&creg,
        PDTA_CRegs,          (uintptr_t)&creg32,
        TAG_END);

    if (bmh) {
        bmh->bmh_Width  = id->id_BMHD.w;
        bmh->bmh_Height = id->id_BMHD.h;
        bmh->bmh_Depth  = id->id_BMHD.nPlanes;
        bmh->bmh_XAspect = id->id_BMHD.xAspect;
        bmh->bmh_YAspect = id->id_BMHD.yAspect;
        bmh->bmh_PageWidth = id->id_BMHD.pageWidth;
        bmh->bmh_PageHeight = id->id_BMHD.pageHeight;
    }

    if (creg) {
        creg[0].red = 0x00; creg[0].green = 0x00; creg[0].blue = 0x00;
        creg[1].red = 0xFF; creg[1].green = 0xFF; creg[1].blue = 0xFF;
    }

    if (creg32) {
        creg32[0] = id->id_CRegs[0]; creg32[1] = id->id_CRegs[1]; creg32[2] = id->id_CRegs[2];
        creg32[3] = id->id_CRegs[3]; creg32[4] = id->id_CRegs[4]; creg32[5] = id->id_CRegs[5];
    }

    DPRINTF("OM_NEW: success (%dx%d)\n", id->id_Width, id->id_Height);
    return (uint32)newobj;
}

/* Method: OM_GET */
static uint32 Method_Get(Class *cl, Object *o, struct opGet *msg)
{
    struct InstanceData *id = (struct InstanceData *)INST_DATA(cl, o);
    uint32 attr = msg->opg_AttrID;


    if (!msg->opg_Storage) {
        return IIntuition->IDoSuperMethodA(cl, o, (Msg)msg);
    }

    uint32 res = 0;

    switch (attr) {
        case DTA_ObjName:
        case DTA_NodeName:
            *msg->opg_Storage = (uintptr_t)id->id_Name;
            res = 1;
            break;

        case DTA_BaseName:
            *msg->opg_Storage = (uintptr_t)DATATYPENAME;
            res = 1;
            break;

        default:
            res = IIntuition->IDoSuperMethodA(cl, o, (Msg)msg);
            break;
    }

    return res;
}

/* Method: OM_DISPOSE */
static uint32 Method_Dispose(Class *cl, Object *o, Msg msg)
{
    struct InstanceData *id = (struct InstanceData *)INST_DATA(cl, o);
    DPRINTF("OM_DISPOSE\n");

    /* Note: PDTA_ClassBitMap is ours to free; PDTA_BitMap is freed by picture.datatype.
     * Since we set both to the same pointer, we must NOT double-free.
     * We will let picture.datatype free it via PDTA_BitMap and we only own PDTA_ClassBitMap
     * which picture.datatype explicitly does not free.
     * HOWEVER since we set both, we must be careful. Let super dispose first, which will
     * free PDTA_BitMap, then we clear our pointer so we don't double-free. */
    id->id_BitMap = NULL;   /* Cleared before super dispose to avoid double-free */

    if (id->id_Font)     { IGraphics->CloseFont(id->id_Font);     id->id_Font     = NULL; }
    if (id->id_ColorMap) { IGraphics->FreeColorMap(id->id_ColorMap); id->id_ColorMap = NULL; }

    return IIntuition->IDoSuperMethodA(cl, o, msg);
}

/* Method: DTM_PROCLAYOUT and DTM_ASYNCLAYOUT
 *
 * Key insight from SDK docs:
 *   - picture.datatype's DTM_PROCLAYOUT remaps the source bitmap (PDTA_ClassBitMap)
 *     to the screen's current colour depth/palette into PDTA_DestBitMap.
 *   - It then sends GM_RENDER to redraw the window.
 *   - DTM_ASYNCLAYOUT should NOT return 0 — that just signals "async failed"
 *     and Multiview will never call DTM_PROCLAYOUT.
 *   - Instead, we intercept BOTH DTM_ASYNCLAYOUT and DTM_PROCLAYOUT and pass
 *     both to the superclass as DTM_PROCLAYOUT (synchronous). This forces a
 *     synchronous layout which is safe since our bitmap is ready.
 */
static uint32 Method_ProcLayout(Class *cl, Object *o, struct gpLayout *msg)
{

    /*
     * Build a DTM_PROCLAYOUT message (even if we got DTM_ASYNCLAYOUT).
     * The struct gpLayout is the same for both methods; we just change the MethodID.
     * gpl_Initial=1 on first layout, 0 on resize/refresh.
     */
    struct gpLayout layout_msg;
    if (msg) {
        layout_msg = *msg;
    } else {
        memset(&layout_msg, 0, sizeof(layout_msg));
        layout_msg.gpl_Initial = 1;
    }
    layout_msg.MethodID = DTM_PROCLAYOUT;

    uint32 res = IIntuition->IDoSuperMethodA(cl, o, (Msg)&layout_msg);
    DPRINTF("DTM_PROCLAYOUT done\n");
    IDataTypes->SetDTAttrs(o, NULL, NULL,
        DTA_Sync, TRUE,
        TAG_END);

    return res ? res : 1;
}

/* Class Dispatcher */
uint32 APICALL Dispatch(Class *cl, Object *o, Msg msg)
{
    uint32 ret;

    switch (msg->MethodID) {
        case OM_NEW:
            ret = Method_New(cl, o, (struct opSet *)msg);
            break;

        case OM_GET:
            ret = Method_Get(cl, o, (struct opGet *)msg);
            break;

        case OM_DISPOSE:
            ret = Method_Dispose(cl, o, msg);
            break;

        case DTM_PROCLAYOUT:
            ret = Method_ProcLayout(cl, o, (struct gpLayout *)msg);
            break;

        case DTM_ASYNCLAYOUT:
            ret = Method_ProcLayout(cl, o, (struct gpLayout *)msg);
            break;

        default:
            ret = IIntuition->IDoSuperMethodA(cl, o, msg);
            break;
    }
    return ret;
}

/* Class Initialization */
Class *InitClass(struct ClassBase *cb)
{
    Class *cl;
    cl = IIntuition->MakeClass(DATATYPENAME, PICTUREDTCLASS, NULL, sizeof(struct InstanceData), 0);
    if (cl) {
        cl->cl_Dispatcher.h_Entry = (HOOKFUNC)Dispatch;
        cl->cl_Dispatcher.h_Data  = (APTR)cb;
        cl->cl_UserData           = (uint32)cb;
        IIntuition->AddClass(cl);
    }
    return cl;
}

/* Minimal Triage Logging Utility */
void LogDebug(const char *fmt, ...)
{
    if (!IDOS || !IUtility) return;

    va_list args;
    char buffer[512];
    char final_msg[640];

    va_start(args, fmt);
    IUtility->VSNPrintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    IUtility->SNPrintf(final_msg, sizeof(final_msg), "[font.datatype] %s", buffer);

    /* Route to T: for triage - Use MODE_READWRITE + ChangeFilePosition because MODE_APPEND is not always defined */
    BPTR log = IDOS->Open("T:font.datatype.log", MODE_READWRITE);
    if (!log) {
        log = IDOS->Open("T:font.datatype.log", MODE_NEWFILE);
    } else {
        IDOS->ChangeFilePosition(log, 0, OFFSET_END);
    }

    if (log) {
        IDOS->Write(log, final_msg, strlen(final_msg));
        IDOS->Close(log);
    }
}
