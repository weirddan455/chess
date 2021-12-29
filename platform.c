#include "platform.h"

#ifdef _WIN32
#include "windows_common.h"
#endif

void blitToScreen(void)
{
#ifdef _WIN32
	windowsBlitToScreen();
#endif
}
