#include "platform.h"

#ifdef _WIN32
#include "windows_common.h"
#else
#include "linux_common.h"
#endif

void blitToScreen(void)
{
#ifdef _WIN32
	windowsBlitToScreen();
#else
	linuxBlitToScreen();
#endif
}

void *loadFile(const char *fileName)
{
#ifdef _WIN32
	return windowsLoadFile(fileName);
#else
	return linuxLoadFile(fileName);
#endif
}

void debugLog(const char *message)
{
#ifdef _WIN32
	windowsDebugLog(message);
#else
	linuxDebugLog(message);
#endif
}

void makeComputerMove(void)
{
#ifdef _WIN32
	windowsMakeComputerMove();
#else
	linuxMakeComputerMove();
#endif
}
