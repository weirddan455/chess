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
	return NULL;
#else
	return linuxLoadFile(fileName);
#endif
}

void debugLog(char *message)
{
#ifdef _WIN32
#else
	linuxDebugLog(message);
#endif
}
