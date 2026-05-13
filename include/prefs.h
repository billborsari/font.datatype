/*
**	prefs.h - preferences handling interface for Font DataTypes class
**	Copyright © 1995-96 Michael Letowski
*/

#ifndef PREFS_H
#define PREFS_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DOS_RDARGS_H
#include <dos/rdargs.h>
#endif

#ifndef CLASSBASE_H
#include "classbase.h"
#endif

/*
**	Public constants
*/
#define TEMPLATE	"STRINGS/M,CENTER=CENTRE/S,INV=INVERSE/S,FN=FONTNAME/S,"\
					"DPI/K,FG=FOREGROUND/K,BG=BACKGROUND/K,TRUECOLOR/S"
#define TEMPLATEDPI	"XDPI/A/N,YDPI/A/N"
#define TEMPLATECOL	"R=RED/A/N,G=GREEN/A/N,B=BLUE/A/N"

/*
**	Public structures
*/
/* User preferences for FontDT */
/* struct Opts is now defined in classbase.h */

struct PrefsHandle {
	struct RDArgs *ph_Args;												/* Result of ReadArgs() call */
	struct RDArgs *ph_RDA1;												/* Preallocated RDArgs structures */
	struct RDArgs *ph_RDA2;												/* Preallocated RDArgs structures */
};	/* PrefsHandle */

/*
**	Public functions
*/
struct PrefsHandle *GetFontPrefs(struct ClassBase *cb, struct Opts *opts);
VOID FreeFontPrefs(struct ClassBase *cb, struct PrefsHandle *ph);

#endif	/* PREFS_H */
