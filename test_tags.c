#include <stdio.h>
#include <proto/exec.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    struct Library *DiskfontBase = IExec->OpenLibrary("diskfont.library", 53);
    struct DiskfontIFace *IDiskfont = (struct DiskfontIFace *)IExec->GetInterface(DiskfontBase, "main", 1, NULL);
    
    char dummy_font[256];
    sprintf(dummy_font, "T:dummy.font");
    FILE *f = fopen(dummy_font, "w");
    if (f) {
        unsigned short fch[2] = {0x0F03, 0};
        fwrite(fch, 1, 4, f);
        fclose(f);
    }

    struct TagItem ft_tags[] = {
        { OT_Engine, (uint32)"freetype" },
        { OT_FontFile, (uint32)argv[1] },
        { TAG_DONE, 0 }
    };
    
    // Try FSF_TAGGED in Style
    struct TTextAttr tta_style = { dummy_font, 20, FSF_TAGGED, FPF_DISKFONT, ft_tags };
    struct TextFont *tf1 = IDiskfont->OpenDiskFont((struct TextAttr *)&tta_style);
    printf("Style FSF_TAGGED: %s\n", tf1 ? "SUCCESS" : "FAIL");
    if (tf1) IGraphics->CloseFont(tf1); // wait I don't have IGraphics

    // Try FSF_TAGGED in Flags
    struct TTextAttr tta_flags = { dummy_font, 20, 0, FPF_DISKFONT | FSF_TAGGED, ft_tags };
    struct TextFont *tf2 = IDiskfont->OpenDiskFont((struct TextAttr *)&tta_flags);
    printf("Flags FSF_TAGGED: %s\n", tf2 ? "SUCCESS" : "FAIL");

    return 0;
}
