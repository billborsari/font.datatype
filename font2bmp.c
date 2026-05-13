#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/iffparse.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <libraries/diskfont.h>
#include <libraries/iffparse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* IFF ILBM Macros */
#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((uint32)(a)<<24 | (uint32)(b)<<16 | (uint32)(c)<<8 | (uint32)(d))
#endif
#define ID_ILBM MAKE_ID('I','L','B','M')
#define ID_BMHD MAKE_ID('B','M','H','D')
#define ID_CMAP MAKE_ID('C','M','A','P')
#define ID_BODY MAKE_ID('B','O','D','Y')

struct BitMapHeader {
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

int main(int argc, char **argv)
{
    struct Library *GfxBase = NULL;
    struct GraphicsIFace *IGraphics = NULL;
    struct Library *DOSBase = NULL;
    struct DOSIFace *IDOS = NULL;
    struct Library *DiskfontBase = NULL;
    struct DiskfontIFace *IDiskfont = NULL;
    struct Library *IFFParseBase = NULL;
    struct IFFParseIFace *IIFFParse = NULL;

    if (argc < 2) {
        printf("Usage: font2bmp <fontfile> [size]\n");
        return 10;
    }

    int font_size = 20;
    if (argc >= 3) font_size = atoi(argv[2]);

    DOSBase = IExec->OpenLibrary("dos.library", 53);
    IDOS = (struct DOSIFace *)IExec->GetInterface(DOSBase, "main", 1, NULL);
    GfxBase = IExec->OpenLibrary("graphics.library", 53);
    IGraphics = (struct GraphicsIFace *)IExec->GetInterface(GfxBase, "main", 1, NULL);
    DiskfontBase = IExec->OpenLibrary("diskfont.library", 53);
    IDiskfont = (struct DiskfontIFace *)IExec->GetInterface(DiskfontBase, "main", 1, NULL);
    IFFParseBase = IExec->OpenLibrary("iffparse.library", 53);
    IIFFParse = (struct IFFParseIFace *)IExec->GetInterface(IFFParseBase, "main", 1, NULL);

    if (!IGraphics || !IDOS || !IDiskfont || !IIFFParse) {
        printf("Failed to open libraries\n");
        return 20;
    }

    /* Use TTextAttr for modern font support (TrueType/OpenType) */
    struct TTextAttr tta;
    tta.tta_Name = argv[1];
    tta.tta_YSize = font_size;
    tta.tta_Style = 0;
    tta.tta_Flags = FSF_TAGGED; /* Signal that this is a TTextAttr */
    tta.tta_Tags = NULL;

    printf("Attempting to open font: %s (Size: %d)\n", argv[1], font_size);

    struct TextFont *font = IDiskfont->OpenDiskFont((struct TextAttr *)&tta);
    
    /* Fallback to standard TextAttr if TTextAttr fails */
    if (!font) {
        printf("TTextAttr failed, trying standard TextAttr...\n");
        struct TextAttr ta;
        ta.ta_Name = argv[1];
        ta.ta_YSize = font_size;
        ta.ta_Style = 0;
        ta.ta_Flags = 0;
        font = IDiskfont->OpenDiskFont(&ta);
    }

    if (font) {
        printf("Font opened successfully. Type: %s\n", (font->tf_Flags & FSF_TAGGED) ? "Scalable/TTextAttr" : "Bitmap");
        
        STRPTR test_str = "Amiga Universal Font Test: OK";
        struct RastPort rp;
        IGraphics->InitRastPort(&rp);
        IGraphics->SetFont(&rp, font);

        uint32 w = IGraphics->TextLength(&rp, test_str, strlen(test_str));
        if (w < 10) w = 300;
        uint32 h = font->tf_YSize + 6;
        
        uint32 w_rounded = (w + 15) & ~15;
        uint32 row_bytes = w_rounded / 8;

        printf("Rendering to %dx%d 1-bit bitmap...\n", (int)w_rounded, (int)h);

        struct BitMap *bm = IGraphics->AllocBitMap(w_rounded, h, 1, BMF_DISPLAYABLE|BMF_CLEAR, NULL);
        if (bm) {
            rp.BitMap = bm;
            IGraphics->SetAPen(&rp, 1);
            IGraphics->Move(&rp, 0, font->tf_Baseline + 2);
            IGraphics->Text(&rp, test_str, strlen(test_str));

            /* Export to IFF ILBM via iffparse.library */
            struct IFFHandle *iff = IIFFParse->AllocIFF();
            if (iff) {
                iff->iff_Stream = (uint32)IDOS->Open("out.iff", MODE_NEWFILE);
                if (iff->iff_Stream) {
                    IIFFParse->InitIFFasDOS(iff);
                    if (IIFFParse->OpenIFF(iff, IFFF_WRITE) == 0) {
                        IIFFParse->PushChunk(iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN);

                        /* BMHD Chunk */
                        struct BitMapHeader bmhd;
                        memset(&bmhd, 0, sizeof(bmhd));
                        bmhd.w = w_rounded;
                        bmhd.h = h;
                        bmhd.nPlanes = 1;
                        bmhd.xAspect = 10;
                        bmhd.yAspect = 11;
                        bmhd.pageWidth = w_rounded;
                        bmhd.pageHeight = h;
                        
                        IIFFParse->PushChunk(iff, 0, ID_BMHD, sizeof(bmhd));
                        IIFFParse->WriteChunkBytes(iff, &bmhd, sizeof(bmhd));
                        IIFFParse->PopChunk(iff);

                        /* CMAP Chunk */
                        UBYTE palette[6] = { 0, 0, 0, 255, 255, 255 };
                        IIFFParse->PushChunk(iff, 0, ID_CMAP, 6);
                        IIFFParse->WriteChunkBytes(iff, palette, 6);
                        IIFFParse->PopChunk(iff);

                        /* BODY Chunk */
                        IIFFParse->PushChunk(iff, 0, ID_BODY, IFFSIZE_UNKNOWN);
                        UBYTE *chunky_line = IExec->AllocVecTags(w_rounded, TAG_END);
                        UBYTE *planar_line = IExec->AllocVecTags(row_bytes, TAG_END);

                        for (uint32 y = 0; y < h; y++) {
                            IGraphics->ReadPixelLine8(&rp, 0, y, w_rounded, chunky_line, NULL);
                            memset(planar_line, 0, row_bytes);
                            for (uint32 x = 0; x < w_rounded; x++) {
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
                        printf("Success! out.iff created.\n");
                    }
                    IDOS->Close((BPTR)iff->iff_Stream);
                }
                IIFFParse->FreeIFF(iff);
            }
            IGraphics->FreeBitMap(bm);
        }
        IGraphics->CloseFont(font);
    } else {
        printf("ERROR: Failed to open font '%s'.\n", argv[1]);
    }

    IExec->DropInterface((struct Interface *)IIFFParse);
    IExec->CloseLibrary(IFFParseBase);
    IExec->DropInterface((struct Interface *)IDiskfont);
    IExec->CloseLibrary(DiskfontBase);
    IExec->DropInterface((struct Interface *)IDOS);
    IExec->CloseLibrary(DOSBase);
    IExec->DropInterface((struct Interface *)IGraphics);
    IExec->CloseLibrary(GfxBase);

    return 0;
}
