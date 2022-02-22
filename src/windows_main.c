#include <windows.h>
#include <windowsx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "renderer.h"
#include "pcgrandom.h"
#include "events.h"
#include "windows_common.h"
#include "assets.h"
#include "fonts.h"

typedef BOOLEAN (WINAPI *WindowsSystemRng)(PVOID, ULONG);

static HBITMAP framebufferDIB;
static bool playerGame;

static bool newFramebuffer(int width, int height)
{
	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -height; // negative value to make the framebuffer top-down (origin in top left corner)
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biClrImportant = 0;
	framebufferDIB = CreateDIBSection(frameBufferDC, &bitmapInfo, DIB_RGB_COLORS, (void **)&framebuffer.data, NULL, 0);
	if (framebufferDIB == NULL)
	{
		OutputDebugStringA("CreateDIBSection Failed\r\n");
		return false;
	}
	SelectObject(frameBufferDC, framebufferDIB);
	return true;
}

LRESULT CALLBACK WindowsCallback(_In_ HWND hwnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (Msg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC hdc = BeginPaint(hwnd, &paint);
			BitBlt(hdc, 0, 0, framebuffer.width, framebuffer.height, frameBufferDC, 0, 0, SRCCOPY);
			EndPaint(hwnd, &paint);
			return 0;
		}
		case WM_SIZE:
		{
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			if (width < 1 || height < 1)
			{
				return 0;
			}
			if (width != framebuffer.width || height != framebuffer.height)
			{
				if (DeleteObject(framebufferDIB) == 0)
				{
					OutputDebugStringA("Failed to delete framebuffer DIB\r\n");
				}
				if (!newFramebuffer(width, height))
				{
					OutputDebugStringA("Failed to resize framebuffer\r\n");
					ExitProcess(1);
				}
				else
				{
					framebuffer.width = width;
					framebuffer.height = height;
					renderFrame();
				}
			}
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			leftClickEvent(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), playerGame);
			renderFrame();
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			rightClickEvent();
			renderFrame();
			return 0;
		}
		case WM_USER:
		{
			uint16_t move = wParam;
			movePiece(move, &gameState);
			highlighted[0] = (move & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
			highlighted[1] = move & MOVE_TO_MASK;
			numHightlighted = 2;
			bool gameOver = handleGameOver();
			renderFrame();
			if (playerGame || gameOver)
			{
				AIisThinking = false;
			}
			else
			{
				windowsMakeComputerMove();
			}
			return 0;
		}
	}
	return DefWindowProcA(hwnd, Msg, wParam, lParam);
}

static DWORD WINAPI AIThreadLoop(_In_ LPVOID lpParameter)
{
	HWND window = lpParameter;
	while (true)
	{
		WaitForSingleObject(event, INFINITE);
		uint16_t move = getComputerMove();
		PostMessageA(window, WM_USER, move, 0);
	}
	return 0;
}

static bool seedRng(void)
{
	bool success = false;
	uint64_t randomBuffer[2];
	HMODULE library = LoadLibraryA("advapi32.dll");
	if (library != NULL)
	{
		// Calls RtlGenRandom with dynamic function pointer per MSDN: https://docs.microsoft.com/en-us/windows/win32/api/ntsecapi/nf-ntsecapi-rtlgenrandom
		WindowsSystemRng rng = (WindowsSystemRng)GetProcAddress(library, "SystemFunction036");
		if (rng != NULL)
		{
			if (rng(randomBuffer, 16))
			{
				rngState.state = randomBuffer[0];
				rngState.inc = randomBuffer[1] | 1;
				success = true;
			}
		}
		FreeLibrary(library);
	}
	return success;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	if (!seedRng())
	{
		OutputDebugStringA("Failed to seed RNG\r\n");
		return 1;
	}
	initZobrist();
	if (strcmp(lpCmdLine, "-test") == 0)
	{
		runTests(false);
		return 0;
	}
	else if (strcmp(lpCmdLine, "-test -verbose") == 0)
	{
		runTests(true);
		return 0;
	}
	frameBufferDC = CreateCompatibleDC(NULL);
	if (frameBufferDC == NULL)
	{
		OutputDebugStringA("CreateCompatibleDC Failed");
		return 1;
	}
	framebuffer.width = 720;
	framebuffer.height = 720;
	DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	RECT windowSize;
	windowSize.left = 0;
	windowSize.top = 0;
	windowSize.right = framebuffer.width;
	windowSize.bottom = framebuffer.height;
	if (AdjustWindowRect(&windowSize, windowStyle, FALSE) == 0)
	{
		OutputDebugStringA("AdjustWindowRect Failed\r\n");
		return 1;
	}
	int windowWidth = windowSize.right - windowSize.left;
	int windowHeight = windowSize.bottom - windowSize.top;
	WNDCLASSA windowClass;
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = WindowsCallback;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInstance;
	windowClass.hIcon = NULL;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = "Chess Window Class";
	if (RegisterClassA(&windowClass) == 0)
	{
		OutputDebugStringA("RegisterClassA Failed\r\n");
		return 1;
	}
	HWND window = CreateWindowA("Chess Window Class", "Chess", windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);
	if (window == NULL)
	{
		OutputDebugStringA("CreateWindowA Failed\r\n");
		return 1;
	}
	windowDC = GetDC(window);
	if (windowDC == NULL)
	{
		OutputDebugStringA("Failed to get windowDC\r\n");
		return 1;
	}
	if (!newFramebuffer(framebuffer.width, framebuffer.height))
	{
		OutputDebugStringA("Failed to initalize framebuffer\r\n");
		return 1;
	}
	loadImages();
	loadFont();
	initGameState();
	renderFrame();
	if (strcmp(lpCmdLine, "-ai") == 0)
	{
		AIisThinking = true;
	}
	else
	{
		playerGame = true;
	}
	event = CreateEventA(NULL, FALSE, AIisThinking, NULL);
	if (CreateThread(NULL, 0, AIThreadLoop, window, 0, NULL) == NULL)
	{
		OutputDebugStringA("CreateThread failed\r\n");
		return 1;
	}
	while (true)
	{
		MSG message;
		BOOL messageReturn = GetMessageA(&message, window, 0, 0);
		if (messageReturn == 0 || messageReturn == -1)
		{
			break;
		}
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
	return 0;
}
