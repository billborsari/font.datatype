/*
**	dispatch.h - dispatcher interface for Font DataType class
**	Copyright © 1995-96 Michael Letowski
*/

#ifndef DISPATCH_H
#define DISPATCH_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef CLASSBASE_H
#include "classbase.h"
#endif

/*
**	Public constants
*/
#define DATATYPENAME	"font.datatype"
#define SUPERCLASSNAME	PICTUREDTCLASS

/*
**	Public functions prototypes
*/
Class *InitClass(struct ClassBase *cb);

#endif	/* DISPATCH_H */
