/*
**	otag.c - .otag files reading routines
**	Copyright © 1995-96 Michael Letowski
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <diskfont/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <graphics/text.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/utility.h>

#include "classbase.h"
#include "otag.h"

/*
**	Private constants and macros
*/
#define FTSUFFIX				".font"

#define SUFFIX_LEN			5
#define MAX_NAME_LEN		32

/* Get size of FontContentHeader */
#define SizeOfFCH(fch)	(sizeof(struct FontContentsHeader)+\
															(fch)->fch_NumEntries*sizeof(struct FontContents))
#define VarSizeOfFCH(x)	(sizeof(struct FontContentsHeader)+\
															(x)*sizeof(struct FontContents))


/*
**	Private functions
*/
STATIC struct FontContentsHeader *ReadOutlines(struct ClassBase *cb,
																								BPTR lock, STRPTR name);
STATIC LONG LocSeekFileSize(struct ClassBase *cb, BPTR fh);


/*
**	Extended DiskFont functions
*/
struct FontContentsHeader *NewFC(struct ClassBase *cb, BPTR lock, STRPTR name)
{
	struct FontContentsHeader *MyFCH=NULL;
	ULONG Size;
	
	/* Build 80: Heat-Seeking Lock-On Logic */
	BPTR file;
	BPTR oldDir = ((cb)->cb_IDOS->SetCurrentDir)(lock);
	
	DPRINTF("NewFC: Heat-Seeking Read\n");
	
	if (file = ((cb)->cb_IDOS->Open)(name, MODE_OLDFILE)) {
		unsigned char *scanBuf = (unsigned char *)((cb)->cb_IExec->AllocVec)(256, MEMF_PUBLIC | MEMF_CLEAR);
		if (scanBuf) {
			if (((cb)->cb_IDOS->Read)(file, scanBuf, 256) > 16) {
				int offset = -1;
				uint16 id = 0, entries = 0;
				LBOOL expanded = FALSE;
				
				/* Scan for 0x0F02 or 0x0F03 */
				for (int j = 0; j < 128; j++) {
					if (scanBuf[j] == 0x0F && (scanBuf[j+1] == 0x02 || scanBuf[j+1] == 0x03)) {
						offset = j;
						id = (scanBuf[j] << 8) | scanBuf[j+1];
						/* Check if it's expanded (look for zeros before ID or after) */
						if (j > 0 && scanBuf[j-1] == 0) expanded = TRUE;
						
						/* Entry count follows the ID */
						if (expanded) {
							/* Skip 00 Count 00 Count */
							entries = (scanBuf[j+3] << 8) | scanBuf[j+5];
						} else {
							entries = (scanBuf[j+2] << 8) | scanBuf[j+3];
						}
						break;
					}
				}
				
				if (offset != -1) {
					DPRINTF("NewFC: Locked On!\n");
					Size = sizeof(struct FontContentsHeader) + (entries * sizeof(struct TFontContents));
					if(MyFCH=(struct FontContentsHeader *)((cb)->cb_IExec->AllocVec)(Size, MEMF_PUBLIC | MEMF_CLEAR)) {
						MyFCH->fch_FileID = id;
						MyFCH->fch_NumEntries = entries;
						
						/* Seek to the start of the entries in the file */
						int entryFileOffset = offset + (expanded ? 8 : 4);
						((cb)->cb_IDOS->ChangeFilePosition)(file, entryFileOffset, OFFSET_BEGINNING);
						
						struct TFontContents *tfc = (struct TFontContents *)(MyFCH + 1);
						int chunkLen = expanded ? 520 : 260;
						unsigned char *ebuf = (unsigned char *)((cb)->cb_IExec->AllocVec)(chunkLen, MEMF_PUBLIC | MEMF_CLEAR);
						
						if (ebuf) {
							for (int i = 0; i < entries; i++) {
								if (((cb)->cb_IDOS->Read)(file, ebuf, chunkLen) == chunkLen) {
									unsigned char *dst = (unsigned char *)&tfc[i];
									if (expanded) {
										for (int k = 0; k < 260; k++) dst[k] = ebuf[(k * 2) + 1];
									} else {
										((cb)->cb_IExec->CopyMem)(ebuf, dst, 260);
									}
								}
							}
							((cb)->cb_IExec->FreeVec)(ebuf);
							DPRINTF("NewFC: Squeeze SUCCESS\n");
						}
					}
				} else {
					DPRINTF("NewFC: FAILED to find signature in 256-byte scan\n");
				}
			}
			((cb)->cb_IExec->FreeVec)(scanBuf);
		}
		((cb)->cb_IDOS->Close)(file);
	}
 else {
		DPRINTF("NewFC: Direct Open failed\n");
	}
	
	((cb)->cb_IDOS->SetCurrentDir)(oldDir);
	return(MyFCH);
}	/* NewFC */

VOID DisposeFC(struct ClassBase *cb, struct FontContentsHeader *fch)
{
	struct ExecIFace *IExec = cb->cb_IExec;
	FreeVec(fch);
}	/* DisposeFC */


/*
**	Private functions
*/
STATIC struct FontContentsHeader *ReadOutlines(struct ClassBase *cb,
																								BPTR lock, STRPTR name)
{
	char NewName[MAX_NAME_LEN];

	struct FontContentsHeader *FCH=NULL;
	struct TFontContents *TFC;
	struct TagItem *Tags;
	BPTR FH,OldLock;

	UWORD *Sizes;
	STRPTR Suffix;
	ULONG Offset,I;
	LONG FileLen;

	OldLock=SetCurrentDir(lock);											/* Change to FONTS: dir */

	/* Chage extension */
	clear(&NewName);															/* Set to NULLs */
	strncpy(NewName,name,MAX_NAME_LEN-1);					/* Copy name to buffer */
	unless(Suffix=strstr(NewName,FTSUFFIX))
		throw2(SetIoErr(ERROR_OBJECT_WRONG_TYPE),	EXIT);
	strncpy(Suffix,OTSUFFIX,SUFFIX_LEN);					/* Replace ".font" with ".otag" */

	/* Open .otag file and get its size */
	try(FH=Open(NewName,MODE_OLDFILE),						NO_FILE);
	{
		struct ExamineData *dat = ExamineObjectTags(EX_FileHandle, FH, TAG_DONE);
		if (dat) {
			FileLen = dat->FileSize;
			FreeDosObject(DOS_EXAMINEDATA, dat);
		} else {
			FileLen = LocSeekFileSize(cb, FH);
		}
	}



	try(FileLen>=0,	ERROR1);											/* Make sure size is positive */
	try(Tags=AllocVecTags(FileLen, AVT_Type, MEMF_PUBLIC | MEMF_CLEAR, TAG_DONE),					NO_TAGS);	/* Allocate space for tags */
	try(Read(FH,Tags,FileLen)==FileLen,						ERROR2);	/* Read tags */
	try(GetTagData(OT_FileIdent,0,Tags)==FileLen,	ERROR2);	/* Verify size */

	/* Get size of new FCH and set up data */
	try(Offset=GetTagData(OT_AvailSizes,0,Tags),	ERROR2);
	Sizes=(UWORD *)((UBYTE *)Tags+Offset);					/* Calculate position of data */

	if(FCH=AllocVecTags(VarSizeOfFCH(*Sizes), AVT_Type, MEMF_PUBLIC | MEMF_CLEAR, TAG_DONE))
	{
		FCH->fch_NumEntries=*Sizes;
		for(I=0; I<*Sizes; I++)
		{
			TFC=&TFontContents(FCH)[I];
((cb)->cb_IUtility->SNPrintf)(TFC->tfc_FileName,MAXFONTPATH,"%s/%ld",name,Sizes[I+1]);
			TFC->tfc_TagCount=1;											/* TAG_DONE */
			TFC->tfc_YSize=Sizes[I+1];
			TFC->tfc_Style=FS_NORMAL;									/* Really ??? */
			TFC->tfc_Flags=FPF_DISKFONT;							/* Really ??? */
		}
	}

	catch(ERROR2,		);
	catch(NO_TAGS,	FreeVec(Tags));
	catch(ERROR1,		);
	catch(NO_FILE,	Close(FH));										/* Close opened .otag file */
	catch(EXIT,			);

	SetCurrentDir(OldLock);													/* Get back to old dir */
	return(FCH);
}	/* ReadOutlines */

STATIC LONG LocSeekFileSize(struct ClassBase *cb, BPTR fh)
{
	int64 Pos, Size;

	Pos = GetFilePosition(fh);
	Size = ChangeFilePosition(fh, 0, OFFSET_END);
	ChangeFilePosition(fh, Pos, OFFSET_BEGINNING);

	return (LONG)Size;
}
	/* LocSeekFileSize */
