/*
**	classbase.h - include file for Font DataType class
**	Copyright © 1995-96 Michael Letowski
*/

#ifndef CLASSBASE_H
#define CLASSBASE_H

#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/diskfont.h>


#include <string.h>





typedef unsigned long LBOOL;
#define min(x,y) (((x) > (y))?(y):(x))
#define max(x,y) (((x) > (y))?(x):(y))
#define clamp(x,l,h) min(max(x,l),h)

#define ftst(x,y) ((x)&(y))
#define fset(x,y) (x) |= (y)

#define tswap(x,y,t) t = x, x = y, y = t
#define Color32(x) ((x)*0x01010101)

#define unless(x) if(!(x))
#define elif(x) else if(x)

#define try(x,y) unless(x) goto y
#define catch(x,y) x: y
#define throw2(x,y) {x; goto y; }

#define clear(x) memset((x),0,sizeof(*(x)))
#define new(x,y) ((x) = AllocVecTags(sizeof(*(x)), AVT_Type, (y) | MEMF_PUBLIC | MEMF_CLEAR, TAG_DONE))
#define delete(x) FreeVec((x))

#define ThisProcessS ((struct Process *)FindTask(NULL))

#define MEMF_PUBCLR MEMF_SHARED|MEMF_CLEAR

/*
**	Global library base
*/
struct ClassBase
{
	struct Library          cb_Lib;								/* Library data */
	BPTR                    cb_SegList;
	struct ExecIFace       *cb_IExec;
	struct DOSIFace        *cb_IDOS;
	struct UtilityIFace    *cb_IUtility;
	struct IntuitionIFace  *cb_IIntuition;
	struct GraphicsIFace   *cb_IGraphics;
	struct DataTypesIFace  *cb_IDataTypes;
	struct DiskfontIFace   *cb_IDiskfont;
	struct P96IFace        *cb_IP96;
	struct Interface       *cb_INewlib;
	struct Library         *cb_DOSBase;
	struct Library         *cb_IntuitionBase;
	struct Library         *cb_GfxBase;
	struct Library         *cb_UtilityBase;
	struct Library         *cb_DataTypesBase;
	struct Library         *cb_DiskfontBase;
	struct Library         *cb_P96Base;
	struct Library         *cb_SuperClassBase;
	Class                  *cb_Class;
};	/* ClassBase */

/* User preferences for FontDT */
struct Opts {
	/* These fields are filled when parsing with TEMPLATE */
	STRPTR *opt_Strings;													/* Strings to display */
	LONG    opt_Center;														/* Center lines */
	LONG    opt_Inverse;													/* Inverted colors */
	LONG    opt_FontName;													/* Use font's name as text */
	STRPTR  opt_DPIStr;														/* DPI resolutions string */
	STRPTR  opt_ForeStr;													/* Foreground color string */
	STRPTR  opt_BackStr;													/* Background color string */
	LBOOL   opt_TrueColor;
	/* The following fields are filled by custom code: */
	LONG    opt_XDPI;															/* XDPI */
	LONG    opt_YDPI;															/* YDPI */
	LONG    opt_ForeCol[3];												/* Foreground RGB */
	LONG    opt_BackCol[3];												/* Background RGB */
	LBOOL   opt_DPIFlag;													/* DPI/K parsed */
	LBOOL   opt_ForeFlag;													/* FOREGROUND/K parsed */
	LBOOL   opt_BackFlag;													/* BACKGROUND/K parsed */
};

#ifdef DEBUG_VERSION
#define DPRINTF(fmt, ...) do { struct ExecIFace *IExec = cb->cb_IExec; if(IExec) DebugPrintF(fmt, ##__VA_ARGS__); } while(0)
#else
#define DPRINTF(fmt, ...)
#endif

#endif	/* CLASSBASE_H */
