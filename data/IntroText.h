#include "Lang.h"
static const wchar_t *introTextData = TEXT_BLOCK_INTRO_TEXT
#ifndef BETA
	TEXT_BLOCK_INTRO_STABLE
#else
	TEXT_BLOCK_INTRO_BETA
#endif
	"\n"
	"\bt" MTOS(SAVE_VERSION) "." MTOS(MINOR_VERSION) "." MTOS(BUILD_NUM) " " IDENT_PLATFORM " "
#ifdef SNAPSHOT
	TEXT_BLOCK_INTRO_SNAPSHOT MTOS(SNAPSHOT_ID) " "
#endif
#ifdef X86
	"X86 "
#endif
#ifdef X86_SSE
	"X86_SSE "
#endif
#ifdef X86_SSE2
	"X86_SSE2 "
#endif
#ifdef X86_SSE3
	"X86_SSE3 "
#endif
#ifdef LUACONSOLE
	TEXT_BLOCK_INTRO_LUACONSOLE
#endif
#ifdef GRAVFFT
	TEXT_BLOCK_INTRO_GRAVFFT
#endif
#ifdef REALISTIC
	TEXT_BLOCK_INTRO_REALISTIC
#endif
	;
