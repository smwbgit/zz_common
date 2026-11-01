#pragma once

#include <windows.h>

namespace utils::windows::undocumented::unwind
{
	typedef enum _UNWIND_OP_CODES {
		UWOP_PUSH_NONVOL = 0,
		UWOP_ALLOC_LARGE,
		UWOP_ALLOC_SMALL,
		UWOP_SET_FPREG, 
		UWOP_SAVE_NONVOL,     
		UWOP_SAVE_NONVOL_FAR,
		UWOP_EPILOG,
		UWOP_SAVE_XMM128 = 8, 
		UWOP_SAVE_XMM128_FAR, 
		UWOP_PUSH_MACHFRAME   
	} UNWIND_CODE_OPS;

	typedef unsigned char UBYTE;

	typedef union _UNWIND_CODE {
		struct {
			UBYTE CodeOffset;
			UBYTE UnwindOp : 4;
			UBYTE OpInfo   : 4;
		};
		USHORT FrameOffset;
	} UNWIND_CODE, *PUNWIND_CODE;

	typedef struct _UNWIND_INFO {
		UBYTE Version       : 3;
		UBYTE Flags         : 5;
		UBYTE SizeOfProlog;
		UBYTE CountOfCodes;
		UBYTE FrameRegister : 4;
		UBYTE FrameOffset   : 4;
		UNWIND_CODE UnwindCode[1];
	/*  UNWIND_CODE MoreUnwindCode[((CountOfCodes + 1) & ~1) - 1];
	*   union {
	*       OPTIONAL ULONG ExceptionHandler;
	*       OPTIONAL ULONG FunctionEntry;
	*   };
	*   OPTIONAL ULONG ExceptionData[]; */
	} UNWIND_INFO, *PUNWIND_INFO;
}