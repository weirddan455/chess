#include <windows.h>
#include <windowsx.h>
#include <bcrypt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "renderer.h"
#include "pcgrandom.h"
#include "events.h"
#include "windows_common.h"

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 720

HANDLE heap;

LRESULT CALLBACK WindowsCallback(_In_ HWND hwnd, _In_ UINT Msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (Msg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			RECT rect;
			GetClientRect(hwnd, &rect);
			HDC hdc = BeginPaint(hwnd, &paint);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, frameBufferDC, 0, 0, SRCCOPY);
			EndPaint(hwnd, &paint);
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			if (!leftClickEvent(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
			{
				PostQuitMessage(0);
			}
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			rightClickEvent();
			return 0;
		}
	}
	return DefWindowProcA(hwnd, Msg, wParam, lParam);
}

static void loadBmp(const char *fileName, Image *image)
{
	char errorBuffer[128];
	char formatMessageBuffer[128];
	HANDLE file = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, formatMessageBuffer, 128, NULL);
		snprintf(errorBuffer, 128, "%s: %s", fileName, formatMessageBuffer);
		OutputDebugStringA(errorBuffer);
		return;
	}
	LARGE_INTEGER fileSize;
	if (GetFileSizeEx(file, &fileSize) == 0)
	{
		snprintf(errorBuffer, 128, "%s: GetFileSizeEx Failed\n", fileName);
		OutputDebugStringA(errorBuffer);
		CloseHandle(file);
		return;
	}
	if (fileSize.QuadPart < 1)
	{
		OutputDebugStringA("File size is less than 1 byte\n");
		CloseHandle(file);
		return;
	}
	uint8_t *bmpData = HeapAlloc(heap, 0, fileSize.QuadPart);
	if (bmpData == NULL)
	{
		OutputDebugStringA("HeapAlloc Failed\n");
		CloseHandle(file);
		return;
	}
	DWORD bytesRead = 0;
	ReadFile(file, bmpData, fileSize.QuadPart, &bytesRead, NULL);
	if (bytesRead != fileSize.QuadPart)
	{
		snprintf(errorBuffer, 128, "%s: Did not read the entire file\n", fileName);
		OutputDebugStringA(errorBuffer);
		CloseHandle(file);
		HeapFree(heap, 0, bmpData);
		return;
	}
	CloseHandle(file);
	uint32_t startingPixelIndex = *(uint32_t *)&bmpData[10];
    int32_t width = *(int32_t *)&bmpData[18];
    int32_t height = *(int32_t *)&bmpData[22];
    uint32_t pixelDataSize = *(uint32_t *)&bmpData[34];
    uint32_t expectedDataSize = width * height * 4;
	if (expectedDataSize != pixelDataSize)
	{
		snprintf(errorBuffer, 128, "%s: Expected %u bytes. Header reads %u\n", fileName, expectedDataSize, pixelDataSize);
		OutputDebugStringA(errorBuffer);
		HeapFree(heap, 0, bmpData);
		return;
	}
	uint8_t *pixelData = HeapAlloc(heap, 0, pixelDataSize);
	if (pixelData == NULL)
	{
		OutputDebugStringA("HeapAlloc Failed\n");
		HeapFree(heap, 0, bmpData);
		return;
	}
	// BMP images have origin at bottom left.
    // Move data so that origin is top left.
    uint32_t bmpDataIndex = startingPixelIndex;
    uint32_t pixelDataIndex = pixelDataSize - (width * 4);
    for (int i = 0; i < height; i++)
    {
        memcpy(&pixelData[pixelDataIndex], &bmpData[bmpDataIndex], width * 4);
        bmpDataIndex += (width * 4);
        pixelDataIndex -= (width * 4);
    }
	HeapFree(heap, 0, bmpData);
    image->width = width;
    image->height = height;
    image->data = pixelData;
}

static void loadImages(void)
{
    loadBmp("images/black-bishop.bmp", &blackBishop);
    loadBmp("images/black-king.bmp", &blackKing);
    loadBmp("images/black-knight.bmp", &blackKnight);
    loadBmp("images/black-pawn.bmp", &blackPawn);
    loadBmp("images/black-queen.bmp", &blackQueen);
    loadBmp("images/black-rook.bmp", &blackRook);
    loadBmp("images/white-bishop.bmp", &whiteBishop);
    loadBmp("images/white-king.bmp", &whiteKing);
    loadBmp("images/white-knight.bmp", &whiteKnight);
    loadBmp("images/white-pawn.bmp", &whitePawn);
    loadBmp("images/white-queen.bmp", &whiteQueen);
    loadBmp("images/white-rook.bmp", &whiteRook);
}

static bool initFrameBuffer(void)
{
	frameBufferDC = CreateCompatibleDC(NULL);
	if (frameBufferDC == NULL)
	{
		OutputDebugStringA("CreateCompatibleDC Failed");
		return false;
	}
	BITMAPINFO bitmapInfo;
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = FRAMEBUFFER_WIDTH;
	bitmapInfo.bmiHeader.biHeight = -(FRAMEBUFFER_HEIGHT); // negative value to make the framebuffer top-down (origin in top left corner)
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = 0;
	bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
	bitmapInfo.bmiHeader.biClrUsed = 0;
	bitmapInfo.bmiHeader.biClrImportant = 0;
	HBITMAP DIBSection = CreateDIBSection(frameBufferDC, &bitmapInfo, DIB_RGB_COLORS, &frameBuffer, NULL, 0);
	if (DIBSection == NULL)
	{
		OutputDebugStringA("CreateDIBSection Failed\n");
		return false;
	}
	SelectObject(frameBufferDC, DIBSection);
	return true;
}

static bool seedRng(void)
{
	uint64_t randomBuffer[2];
	if (BCryptGenRandom(NULL, randomBuffer, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
	{
		return false;
	}
    rngState.state = randomBuffer[0];
    rngState.inc = randomBuffer[1] | 1;
    return true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	heap = GetProcessHeap();
	if (heap == NULL)
	{
		OutputDebugStringA("GetProcessHeap Failed\n");
		return 1;
	}
	if (!seedRng())
	{
		OutputDebugStringA("Failed to seed RNG");
		return 1;
	}
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
		OutputDebugStringA("RegisterClassA Failed\n");
		return 1;
	}
	HWND window = CreateWindowA("Chess Window Class", "Chess", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
	if (window == NULL)
	{
		OutputDebugStringA("CreateWindowA Failed\n");
		return 1;
	}
	windowDC = GetDC(window);
	if (windowDC == NULL)
	{
		OutputDebugStringA("Failed to get windowDC\n");
		return 1;
	}
	if (!initFrameBuffer())
	{
		OutputDebugStringA("Failed to initalize framebuffer\n");
		return 1;
	}
	loadImages();
	initGameState();
	renderFrame(NULL, 0);
	while (1)
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
