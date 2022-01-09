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

void *allocateMemory(uint64_t size)
{
#ifdef _WIN32
	return NULL;
#else
	return linuxAllocateMemory(size);
#endif
}

void freeMemory(void *ptr)
{
#ifdef _WIN32
#else
	linuxFreeMemory(ptr);
#endif
}
