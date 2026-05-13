/*
**	prefs.c - preferences handling for Font DataTypes class
**	Copyright © 1995-96 Michael Letowski
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "classbase.h"
#include "prefs.h"

/*
**	Private functions
*/
STATIC LBOOL ReadPrefs(struct ClassBase *cb, struct PrefsHandle *ph,
												struct Opts *opts, STRPTR name);

/*
**	Local constants
*/
#define BUF_SIZE		512
#define PREFS_PATH1	"PROGDIR:font.datatype"
#define PREFS_PATH2	"ENV:font.datatype"


/*
**	Public functions
*/

struct PrefsHandle *GetFontPrefs(struct ClassBase *cb, struct Opts *opts)
{
	struct DOSIFace *IDOS = cb->cb_IDOS;
	struct ExecIFace *IExec = cb->cb_IExec;
	struct PrefsHandle *PH;
	LBOOL Res=FALSE;

	try(PH=new(PH, MEMF_SHARED),	NO_PREFS);
	try(PH->ph_RDA1=AllocDosObject(DOS_RDARGS,NULL),	NO_RDA1);
	try(PH->ph_RDA2=AllocDosObject(DOS_RDARGS,NULL),	NO_RDA2);

	if(GetProgramDir())											/* PROGDIR: exists? */
		Res=ReadPrefs(cb,PH,opts,PREFS_PATH1);			/* Try first path */
	unless(Res)																		/* Args not read so far */
		ReadPrefs(cb,PH,opts,PREFS_PATH2);					/* Try second path */
	return(PH);

	catch(NO_RDA2,	FreeDosObject(DOS_RDARGS,PH->ph_RDA2));
	catch(NO_RDA1,	FreeDosObject(DOS_RDARGS,PH->ph_RDA1));
	catch(NO_PREFS,	delete(PH));
	return(NULL);
}	/* GetFontPrefs */

VOID FreeFontPrefs(struct ClassBase *cb, struct PrefsHandle *ph)
{
	struct DOSIFace *IDOS = cb->cb_IDOS;
	struct ExecIFace *IExec = cb->cb_IExec;
	FreeArgs(ph->ph_Args);												/* Free args (may be NULL) */
	FreeDosObject(DOS_RDARGS,ph->ph_RDA2);				/* Free 2nd RDA structure */
	FreeDosObject(DOS_RDARGS,ph->ph_RDA1);				/* Free 1st RDA structure */
	delete(ph);
}	/* FreeFontPrefs */


/*
**	Private functions
*/
STATIC LBOOL ReadPrefs(struct ClassBase *cb, struct PrefsHandle *ph,
												struct Opts *opts, STRPTR name)
{
	struct DOSIFace *IDOS = cb->cb_IDOS;
	struct ExecIFace *IExec = cb->cb_IExec;
	struct UtilityIFace *IUtility = cb->cb_IUtility;

	struct RDArgs *RDA1=ph->ph_RDA1;
	struct RDArgs *RDA2=ph->ph_RDA2;
	STRPTR Buf,Buffer;
	LONG Len,Size;
	BPTR FH;

	try(FH=Open(name,MODE_OLDFILE),							NO_FILE);
	try(Buffer=AllocVecTags(BUF_SIZE, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_DONE),	NO_BUF);

	/* Consolidate multiple lines into a single buffer */
	Buf=Buffer;
	Size=BUF_SIZE-1;
	while(Size>0 && FGets(FH,Buf,Size))
	{
		Len=strlen(Buf);
		if(Buf[Len-1]=='\n')	Buf[Len-1]=' ';
		Buf+=Len;
		Size-=Len;
	}
	Close(FH);

	/* Read arguments from a string (buffer) */
	RDA1->RDA_Source.CS_Buffer=Buffer;
	RDA1->RDA_Source.CS_Length=strlen(Buffer);
	ph->ph_Args=ReadArgs(TEMPLATE,(LONG *)opts,RDA1);

	/* Parse string resolutions */
	if(ph->ph_Args && opts->opt_DPIStr)
	{
		RDA2->RDA_Source.CS_Buffer=opts->opt_DPIStr;
		RDA2->RDA_Source.CS_Length=strlen(opts->opt_DPIStr);
		if(ReadArgs(TEMPLATEDPI,(LONG *)&opts->opt_XDPI,RDA2))
			opts->opt_DPIFlag=TRUE;
		FreeArgs(RDA2);													/* Only free args, NOT the RDA struct! */
	}

	/* Parse foreground color string */
	if(ph->ph_Args && opts->opt_ForeStr)
	{
		RDA2->RDA_Source.CS_Buffer=opts->opt_ForeStr;
		RDA2->RDA_Source.CS_Length=strlen(opts->opt_ForeStr);
		if(ReadArgs(TEMPLATECOL,(LONG *)opts->opt_ForeCol,RDA2))
			opts->opt_ForeFlag=TRUE;
		FreeArgs(RDA2);
	}

	/* Parse background color string */
	if(ph->ph_Args && opts->opt_BackStr)
	{
		RDA2->RDA_Source.CS_Buffer=opts->opt_BackStr;
		RDA2->RDA_Source.CS_Length=strlen(opts->opt_BackStr);
		if(ReadArgs(TEMPLATECOL,(LONG *)opts->opt_BackCol,RDA2))
			opts->opt_BackFlag=TRUE;
		FreeArgs(RDA2);
	}

	FreeVec(Buffer);
	return(ph->ph_Args != NULL);

	catch(NO_BUF,		Close(FH));
	catch(NO_FILE,	);
	return(FALSE);
}
