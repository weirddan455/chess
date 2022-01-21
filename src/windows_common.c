#include "windows_common.h"
#include "renderer.h"
#include "platform.h"
#include "game.h"
#include "events.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

HDC windowDC;
HDC frameBufferDC;

CONDITION_VARIABLE cond = CONDITION_VARIABLE_INIT;
SRWLOCK lock = SRWLOCK_INIT;
volatile bool AIThreadWakeup;

void windowsBlitToScreen(void)
{
	BitBlt(windowDC, 0, 0, framebuffer.width, framebuffer.height, frameBufferDC, 0, 0, SRCCOPY);
}

void *windowsLoadFile(const char *fileName)
{
	char errorBuffer[LOG_SIZE];
	char formatMessageBuffer[128];
	HANDLE file = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, formatMessageBuffer, 128, NULL);
		snprintf(errorBuffer, LOG_SIZE, "%s: %s\r\n", fileName, formatMessageBuffer);
		OutputDebugStringA(errorBuffer);
		return NULL;
	}
	LARGE_INTEGER fileSize;
	if (GetFileSizeEx(file, &fileSize) == 0)
	{
		snprintf(errorBuffer, LOG_SIZE, "%s: GetFileSizeEx Failed\r\n", fileName);
		OutputDebugStringA(errorBuffer);
		CloseHandle(file);
		return NULL;
	}
	// janky ass Windows ReadFile can only take a 32 bit int.
	// We're probably never reading in a file larger than 4GB, so just use the low part.
	if (fileSize.LowPart != fileSize.QuadPart)
	{
		OutputDebugStringA("File is too large\r\n");
		CloseHandle(file);
		return NULL;
	}
	if (fileSize.LowPart < 1)
	{
		OutputDebugStringA("File size is less than 1 byte\r\n");
		CloseHandle(file);
		return NULL;
	}
	void *data = malloc(fileSize.LowPart);
	if (data == NULL)
	{
		OutputDebugStringA("malloc failed\r\n");
		CloseHandle(file);
		return NULL;
	}
	DWORD bytesRead = 0;
	if (ReadFile(file, data, fileSize.LowPart, &bytesRead, NULL) == 0)
	{
		DWORD error = GetLastError();
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, formatMessageBuffer, 128, NULL);
		snprintf(errorBuffer, LOG_SIZE, "%s: %s\r\n", fileName, formatMessageBuffer);
		OutputDebugStringA(errorBuffer);
		CloseHandle(file);
		free(data);
		return NULL;
	}
	if (bytesRead != fileSize.LowPart)
	{
		snprintf(errorBuffer, LOG_SIZE, "%s: read %d bytes. %d expected.\r\n", fileName, bytesRead, fileSize.LowPart);
		OutputDebugStringA(errorBuffer);
		CloseHandle(file);
		free(data);
		return NULL;
	}
	CloseHandle(file);
	return data;
}

void windowsDebugLog(const char *message)
{
	char messageCopy[LOG_SIZE];
	int len = 0;
	while (len < LOG_SIZE - 3)
	{
		char c = message[len];
		if (c == 0)
		{
			break;
		}
		messageCopy[len] = c;
		len++;
	}
	messageCopy[len] = '\r';
	messageCopy[len + 1] = '\n';
	messageCopy[len + 2] = 0;
	OutputDebugStringA(messageCopy);
}

void windowsMakeComputerMove(void)
{
	AcquireSRWLockExclusive(&lock);
	AIThreadWakeup = true;
	ReleaseSRWLockExclusive(&lock);
	WakeConditionVariable(&cond);
}
