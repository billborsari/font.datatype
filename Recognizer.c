#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

/*
 * Recognizer for Font DataType
 * Usage: Recognizer <filename>
 */

int main(int argc, char *argv[])
{
    struct Library *SysBase = (*((struct Library **) 4));
    struct ExecIFace *IExec = (struct ExecIFace *)((struct ExecBase *)SysBase)->MainInterface;
    struct Library *DOSBase;
    struct DOSIFace *IDOS;

    if (argc < 2) return RETURN_FAIL;

    DOSBase = IExec->OpenLibrary("dos.library", 52);
    if (!DOSBase) return RETURN_FAIL;
    IDOS = (struct DOSIFace *)IExec->GetInterface(DOSBase, "main", 1, NULL);
    if (!IDOS) {
        IExec->CloseLibrary(DOSBase);
        return RETURN_FAIL;
    }

    BPTR lock = IDOS->Lock(argv[1], SHARED_LOCK);
    if (lock) {
        struct ExamineData *dat = IDOS->ExamineObjectTags(EX_LockInput, lock, TAG_END);
        if (dat) {
            if (EXD_IS_DIRECTORY(dat)) {
                IDOS->FreeDosObject(DOS_EXAMINEDATA, dat);
                IDOS->UnLock(lock);
                IExec->DropInterface((struct Interface *)IDOS);
                IExec->CloseLibrary(DOSBase);
                return RETURN_FAIL;
            }
            IDOS->FreeDosObject(DOS_EXAMINEDATA, dat);
        }

        BPTR fh = IDOS->Open(argv[1], MODE_OLDFILE);
        if (fh) {
            uint8 buffer[32];
            int32 read = IDOS->Read(fh, buffer, 32);
            IDOS->Close(fh);

            if (read >= 14) {
                /* Legacy Amiga Font (0x0F00/0x0F02/0x0F03 at offset 12) */
                uint16 magic = (buffer[12] << 8) | buffer[13];
                if ((magic & 0xFFFC) == 0x0F00) {
                    IDOS->UnLock(lock);
                    IExec->DropInterface((struct Interface *)IDOS);
                    IExec->CloseLibrary(DOSBase);
                    return RETURN_OK; /* DOSTRUE */
                }

                /* TrueType / OpenType Check */
                uint32 head = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                if (head == 0x00010000 || head == 0x4F54544F) {
                    IDOS->UnLock(lock);
                    IExec->DropInterface((struct Interface *)IDOS);
                    IExec->CloseLibrary(DOSBase);
                    return RETURN_OK;
                }

                /* PostScript Type 1 (.pfb) Check: Starts with 0x80 0x01 (ASCII) or 0x80 0x02 (Binary) */
                if (buffer[0] == 0x80 && (buffer[1] == 0x01 || buffer[1] == 0x02)) {
                    IDOS->UnLock(lock);
                    IExec->DropInterface((struct Interface *)IDOS);
                    IExec->CloseLibrary(DOSBase);
                    return RETURN_OK;
                }

                /* .otag (Outline Tag) Check: Starts with OT_FileIdent (0x80001001) */
                uint32 otag_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                if (otag_id == 0x80001001) {
                    IDOS->UnLock(lock);
                    IExec->DropInterface((struct Interface *)IDOS);
                    IExec->CloseLibrary(DOSBase);
                    return RETURN_OK;
                }
            }
        }
        IDOS->UnLock(lock);
    }

    IExec->DropInterface((struct Interface *)IDOS);
    IExec->CloseLibrary(DOSBase);
    return RETURN_FAIL; /* DOSFALSE */
}
