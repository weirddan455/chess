#include "linux_common.h"
#include "renderer.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

Display *display;
Window window;
XImage *ximage;
GC gc;

void linuxBlitToScreen(void)
{
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, framebuffer.width, framebuffer.height);
}

void *linuxAllocateMemory(uint64_t size)
{
    void *mem = malloc(size);
    if (mem == NULL)
    {
        puts("malloc failed");
        return NULL;
    }
    return mem;
}

void linuxFreeMemory(void *ptr)
{
    free(ptr);
}

void *linuxLoadFile(const char *fileName)
{
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror(fileName);
        return NULL;
    }
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0)
    {
        perror(fileName);
        close(fd);
        return NULL;
    }
    void *data = malloc(fileInfo.st_size);
    if (data == NULL)
    {
        puts("malloc failed");
        close(fd);
        return NULL;
    }
    ssize_t bytesRead = read(fd, data, fileInfo.st_size);
    close(fd);
    if (bytesRead == -1)
    {
        perror(fileName);
        free(data);
        return NULL;
    }
    if (bytesRead != fileInfo.st_size)
    {
        printf("%s: read %ld bytes. %ld expected.\n", fileName, bytesRead, fileInfo.st_size);
        free(data);
        return NULL;
    }
    return data;
}
