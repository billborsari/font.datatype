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

struct IFF_BitMapHeader {
    UWORD w, h;
    WORD  x, y;
    UBYTE nPlanes;
    UBYTE masking;
    UBYTE compression;
    UBYTE pad1;
    UWORD transparentColor;
    UBYTE xAspect, yAspect;
    WORD  pageWidth, pageHeight;
};

/* Forensic Tool: Save generated BitMap to IFF for inspection */
static void SaveBitMapToIFF(struct BitMap *bm, struct TextFont *font)
{
    struct IFFHandle *iff;
    uint32 w = IGraphics->GetBitMapAttr(bm, BMA_WIDTH);
    uint32 h = IGraphics->GetBitMapAttr(bm, BMA_HEIGHT);
    uint32 row_bytes = (w + 15) / 16 * 2; /* 16-bit aligned row bytes */

    IExec->DebugPrintF("[font.datatype] Forensic: Saving %dx%d bitmap to SYS:font_preview.iff\n", w, h);

    if (!(iff = IIFFParse->AllocIFF())) return;

    iff->iff_Stream = (uint32)IDOS->Open("SYS:font_preview.iff", MODE_NEWFILE);
    if (iff->iff_Stream) {
        IIFFParse->InitIFFasDOS(iff);
        if (IIFFParse->OpenIFF(iff, IFFF_WRITE) == 0) {
            IIFFParse->PushChunk(iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN);

            /* BMHD */
            struct IFF_BitMapHeader bmhd;
            memset(&bmhd, 0, sizeof(bmhd));
            bmhd.w = w;
            bmhd.h = h;
            bmhd.nPlanes = 1;
            bmhd.xAspect = 10;
            bmhd.yAspect = 11;
            bmhd.pageWidth = w;
            bmhd.pageHeight = h;
            IIFFParse->PushChunk(iff, 0, ID_BMHD, sizeof(bmhd));
            IIFFParse->WriteChunkBytes(iff, &bmhd, sizeof(bmhd));
            IIFFParse->PopChunk(iff);

            /* CMAP */
            UBYTE palette[6] = { 255, 255, 255, 0, 0, 0 }; /* White BG, Black FG */
            IIFFParse->PushChunk(iff, 0, ID_CMAP, 6);
            IIFFParse->WriteChunkBytes(iff, palette, 6);
            IIFFParse->PopChunk(iff);

            /* BODY */
            IIFFParse->PushChunk(iff, 0, ID_BODY, IFFSIZE_UNKNOWN);
            UBYTE *chunky_line = IExec->AllocVecTags(w + 16, TAG_END);
            UBYTE *planar_line = IExec->AllocVecTags(row_bytes, TAG_END);
            
            struct RastPort rp;
            IGraphics->InitRastPort(&rp);
            rp.BitMap = bm;

            for (uint32 y = 0; y < h; y++) {
                IGraphics->ReadPixelLine8(&rp, 0, y, w, chunky_line, NULL);
                memset(planar_line, 0, row_bytes);
                for (uint32 x = 0; x < w; x++) {
                    if (chunky_line[x]) {
                        planar_line[x >> 3] |= (1 << (7 - (x & 7)));
                    }
                }
                IIFFParse->WriteChunkBytes(iff, planar_line, row_bytes);
            }
            IExec->FreeVec(chunky_line);
            IExec->FreeVec(planar_line);
            
            IIFFParse->PopChunk(iff); /* Pop BODY */
            IIFFParse->PopChunk(iff); /* Pop FORM */
            IIFFParse->CloseIFF(iff);
        }
        IDOS->Close((BPTR)iff->iff_Stream);
    }
    IIFFParse->FreeIFF(iff);
}

/* ObtainEngine hook */
Class *_DTClass_ObtainEngine(struct DTClassIFace *Self)
{
    struct ClassBase *cb = (struct ClassBase *)Self->Data.LibBase;
    IExec->DebugPrintF("[font.datatype] ObtainEngine: returning class %p\n", cb->cb_Class);
    return (Class *)cb->cb_Class;
}




/* Method: OM_NEW */
static uint32 Method_New(Class *cl, Object *o, struct opSet *msg)
{
    Object *newobj;
    struct InstanceData *id;
    STRPTR filename = (STRPTR)IUtility->GetTagData(DTA_Name, 0, msg->ops_AttrList);

    IExec->DebugPrintF("[font.datatype] OM_NEW entry: filename=%s\n", filename ? filename : (STRPTR)"NULL");
    
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
        IExec->DebugPrintF("[font.datatype] OM_NEW: Superclass returned NULL\n");
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

    const char *final_engine_name = "diskfont.library (Standard)";
    BOOL first_open_success = FALSE;

    for (int i = 0; i < num_sizes; i++) {
        struct TextFont *temp_font = NULL;
        if (filename) {
            STRPTR name = (STRPTR)IDOS->FilePart(filename);
            
            /* Try standard OpenDiskFont with just the name (best for .font in FONTS: or relative) */
            struct TTextAttr tta = { name, preview_sizes[i], FSF_TAGGED, FPF_DISKFONT, NULL };
            temp_font = IDiskfont->OpenDiskFont((struct TextAttr *)&tta);

            if (!temp_font) {
                /* Try with full path (some systems/engines prefer this) */
                struct TTextAttr tta_full = { (STRPTR)filename, preview_sizes[i], FSF_TAGGED, FPF_DISKFONT, NULL };
                temp_font = IDiskfont->OpenDiskFont((struct TextAttr *)&tta_full);
            }

            if (!temp_font) {
                /* Alternate path: Try forcing FreeType engine with explicit file path */
                struct TagItem ft_tags[] = { 
                    { OT_Engine,     (uintptr_t)"freetype" }, 
                    { OT_FontFile,   (uintptr_t)filename },
                    { TAG_DONE,      0 } 
                };
                struct TTextAttr tta_ft = { name, preview_sizes[i], FSF_TAGGED, FPF_DISKFONT, ft_tags };
                temp_font = IDiskfont->OpenDiskFont((struct TextAttr *)&tta_ft);
                if (temp_font) final_engine_name = "diskfont.library (FreeType Engine)";
            }
        }
        if (!temp_font && filename) {
            /* Final fallback: non-tagged OpenDiskFont */
            struct TextAttr ta = { (STRPTR)filename, preview_sizes[i], 0, 0 };
            temp_font = IDiskfont->OpenDiskFont(&ta);
        }
        if (!temp_font) {
            struct TextAttr ta = { "topaz.font", 8, 0, 0 };
            temp_font = IGraphics->OpenFont(&ta);
            final_engine_name = "graphics.library (Fallback Topaz)";
        }

        if (temp_font) {
            if (!first_open_success) {
                IExec->DebugPrintF("[font.datatype] OM_NEW: Request: %s (Size %d) -> Engine: %s\n", 
                                   filename ? filename : "Unknown", preview_sizes[i], final_engine_name);
                first_open_success = TRUE;
            }
            id->id_Height += temp_font->tf_YSize + 8;
            IGraphics->CloseFont(temp_font);
        }
    }
    id->id_Height += 8; /* Bottom margin */

    /* Generate the preview BitMap */
    IExec->DebugPrintF("[font.datatype] OM_NEW: Allocating %dx%d bitmap\n", id->id_Width, id->id_Height);
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
        struct TextAttr note_ta = { "topaz.font", 8, 0, 0 }; /* Fallback note font */
        struct TextFont *note_font = IGraphics->OpenFont(&note_ta);
        if (note_font) {
            IGraphics->SetFont(&rp, note_font);
            char note_buf[128];
            IUtility->SNPrintf(note_buf, 128, "Engine used to render the font: %s", final_engine_name);
            IGraphics->Move(&rp, 8, 4 + note_font->tf_Baseline);
            IGraphics->Text(&rp, note_buf, strlen(note_buf));
            IGraphics->CloseFont(note_font);
        }

        int current_y = 24;
        for (int i = 0; i < num_sizes; i++) {
            struct TextFont *temp_font = NULL;
            if (filename) {
                STRPTR name = (STRPTR)IDOS->FilePart(filename);
                /* Try exactly the same engine that succeeded in the measurement loop */
                struct TagItem ft_tags[] = { 
                    { OT_Engine,   (uintptr_t)"freetype" }, 
                    { OT_FontFile, (uintptr_t)filename },
                    { TAG_DONE,    0 } 
                };
                struct TTextAttr tta = { name, preview_sizes[i], FSF_TAGGED, FPF_DISKFONT, (final_engine_name[21] == 'F') ? ft_tags : NULL };
                temp_font = IDiskfont->OpenDiskFont((struct TextAttr *)&tta);

                if (!temp_font && (final_engine_name[21] != 'F')) {
                    /* If standard failed with FilePart, try full path */
                    struct TTextAttr tta_full = { (STRPTR)filename, preview_sizes[i], FSF_TAGGED, FPF_DISKFONT, NULL };
                    temp_font = IDiskfont->OpenDiskFont((struct TextAttr *)&tta_full);
                }
            }
            if (!temp_font && filename) {
                struct TextAttr ta = { (STRPTR)filename, preview_sizes[i], 0, 0 };
                temp_font = IDiskfont->OpenDiskFont(&ta);
            }
            if (!temp_font) {
                struct TextAttr ta = { "topaz.font", 8, 0, 0 };
                temp_font = IGraphics->OpenFont(&ta);
            }

            if (temp_font) {
                IGraphics->SetFont(&rp, temp_font);
                IGraphics->Move(&rp, 8, current_y + temp_font->tf_Baseline);
                IGraphics->Text(&rp, preview_text, strlen(preview_text));
                
                current_y += temp_font->tf_YSize + 8;
                IGraphics->CloseFont(temp_font);
            }
        }

        /* Forensic save of the final composite bitmap */
        SaveBitMapToIFF(id->id_BitMap, NULL);
    } 

    if (!id->id_BitMap) {
        IExec->DebugPrintF("[font.datatype] OM_NEW ERROR: No BitMap created, failing.\n");
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
        *bmh = id->id_BMHD;
    }

    if (creg) {
        creg[0].red = 0x00; creg[0].green = 0x00; creg[0].blue = 0x00;
        creg[1].red = 0xFF; creg[1].green = 0xFF; creg[1].blue = 0xFF;
    }

    if (creg32) {
        creg32[0] = id->id_CRegs[0]; creg32[1] = id->id_CRegs[1]; creg32[2] = id->id_CRegs[2];
        creg32[3] = id->id_CRegs[3]; creg32[4] = id->id_CRegs[4]; creg32[5] = id->id_CRegs[5];
    }

    IExec->DebugPrintF("[font.datatype] OM_NEW exit: success (bm=%p, w=%u, h=%u)\n",
                       id->id_BitMap, id->id_Width, id->id_Height);
    return (uint32)newobj;
}

/* Method: OM_GET */
static uint32 Method_Get(Class *cl, Object *o, struct opGet *msg)
{
    struct InstanceData *id = (struct InstanceData *)INST_DATA(cl, o);
    uint32 attr = msg->opg_AttrID;

    IExec->DebugPrintF("[font.datatype] OM_GET: attr=%08x\n", attr);

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
        {
            res = IIntuition->IDoSuperMethodA(cl, o, (Msg)msg);
            IExec->DebugPrintF("[font.datatype] OM_GET: super(attr %08x) -> res %u, val %08x\n",
                               attr, res, *msg->opg_Storage);
            return res;
        }
    }

    IExec->DebugPrintF("[font.datatype] OM_GET: handled(attr %08x) -> res %u, val %08x\n",
                       attr, res, *msg->opg_Storage);
    return res;
}

/* Method: OM_DISPOSE */
static uint32 Method_Dispose(Class *cl, Object *o, Msg msg)
{
    struct InstanceData *id = (struct InstanceData *)INST_DATA(cl, o);
    IExec->DebugPrintF("[font.datatype] OM_DISPOSE\n");

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
    IExec->DebugPrintF("[font.datatype] DTM_PROCLAYOUT/ASYNCLAYOUT entry (initial=%d)\n",
                       msg ? msg->gpl_Initial : -1);

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
    IExec->DebugPrintF("[font.datatype] DTM_PROCLAYOUT super returned %u\n", res);

    /* 
     * Since we intercepted DTM_ASYNCLAYOUT and did the layout synchronously,
     * the usual async thread that sends DTA_Sync was never spawned.
     * We MUST explicitly notify Multiview that we are done, otherwise it waits forever.
     */
    IExec->DebugPrintF("[font.datatype] DTM_PROCLAYOUT complete. Sending DTA_Sync=TRUE\n");
    IDataTypes->SetDTAttrs(o, NULL, NULL,
        DTA_Sync, TRUE,
        TAG_END);

    return res ? res : 1;
}

/* Class Dispatcher */
uint32 APICALL Dispatch(Class *cl, Object *o, Msg msg)
{
    uint32 ret;

    IExec->DebugPrintF("[font.datatype] Dispatch: Method %08x\n", msg->MethodID);

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
            /* Synchronous layout — called by Multiview on the app's process */
            ret = Method_ProcLayout(cl, o, (struct gpLayout *)msg);
            break;

        case DTM_ASYNCLAYOUT:
            /*
             * CRITICAL: Do NOT return 0 here. Returning 0 signals "async layout failed"
             * and Multiview will show "Loading..." forever without ever calling
             * DTM_PROCLAYOUT. Instead, we treat it as DTM_PROCLAYOUT and do it
             * synchronously (which is safe since our bitmap was pre-generated in OM_NEW).
             */
            IExec->DebugPrintF("[font.datatype] DTM_ASYNCLAYOUT -> redirecting to DTM_PROCLAYOUT\n");
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
