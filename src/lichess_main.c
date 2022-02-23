#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static bool challenge;
static char challengeId[8];

static char headerString[128];

typedef struct Buffer
{
    size_t size;
    size_t capacity;
    char *data;
} Buffer;

size_t writeCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realSize = size * nmemb;
    Buffer *writeBuffer = userdata;
    if (writeBuffer->size + realSize > writeBuffer->capacity)
    {
        size_t newCapacity = writeBuffer->capacity + writeBuffer->capacity;
        char *newData = realloc(writeBuffer->data, newCapacity);
        if (newData == NULL)
        {
            puts("Realloc failed");
            return 0;
        }
        writeBuffer->capacity = newCapacity;
        writeBuffer->data = newData;
    }
    size_t index = 0;
    while (index < realSize)
    {
        if (ptr[index] == '\n')
        {
            if (writeBuffer->size > 48)
            {
                const char *challengeCompareString = "{\"type\":\"challenge\",\"challenge\":{\"id\":\"";
                if (memcmp(writeBuffer->data, challengeCompareString, strlen(challengeCompareString)) == 0)
                {
                    pthread_mutex_lock(&mutex);
                    challenge = true;
                    memcpy(challengeId, &writeBuffer->data[39], 8);
                    pthread_cond_signal(&cond);
                    pthread_mutex_unlock(&mutex);
                }
            }
            writeBuffer->size = 0;
            index += 1;
        }
        else
        {
            writeBuffer->data[writeBuffer->size] = ptr[index];
            writeBuffer->size += 1;
            index += 1;
        }
    }
    return realSize;
}

void *workerThreadLoop(void *arg)
{
    CURL *curl = curl_easy_init();
    if (curl == NULL)
    {
        puts("curl_easy_init failed");
        exit(1);
    }
    struct curl_slist *headers = curl_slist_append(NULL, headerString);
    if (headers == NULL)
    {
        puts("curl_slist_append failed");
        exit(1);
    }
    char errorBuffer[CURL_ERROR_SIZE];
    memset(errorBuffer, 0, CURL_ERROR_SIZE);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);

    const char *urlStart = "https://lichess.org/api/challenge/";
    const char *urlEnd = "/accept";
    size_t urlStartLen = strlen(urlStart);

    char url[64];
    memcpy(url, urlStart, urlStartLen);
    memcpy(&url[urlStartLen + 8], urlEnd, strlen(urlEnd) + 1);

    while (true)
    {
        pthread_mutex_lock(&mutex);
        while (!challenge)
        {
            pthread_cond_wait(&cond, &mutex);
        }
        memcpy(&url[urlStartLen], challengeId, 8);
        challenge = false;
        pthread_mutex_unlock(&mutex);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (curl_easy_perform(curl) != CURLE_OK)
        {
            printf("curl_easy_perform failed: %s\n", errorBuffer);
        }
        puts("Challenge sent");
    }
    return NULL;
}

static bool generateHeaderString(void)
{
    const char *fileName = "lichess.token";
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror(fileName);
        return false;
    }
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0)
    {
        perror(fileName);
        close(fd);
        return false;
    }
    if (fileInfo.st_size < 1)
    {
        puts("lichess.token is less than 1 byte");
        close(fd);
        return false;
    }
    const char *authConstant = "Authorization: Bearer ";
    size_t constantSize = strlen(authConstant);
    size_t stringEnd = constantSize + fileInfo.st_size;
    if (stringEnd > 127)
    {
        puts("lichess.token is too large");
        close(fd);
        return false;
    }
    memcpy(headerString, authConstant, constantSize);
    ssize_t bytesRead = read(fd, &headerString[constantSize], fileInfo.st_size);
    if (bytesRead == -1)
    {
        perror(fileName);
        close(fd);
        return false;
    }
    if (bytesRead != fileInfo.st_size)
    {
        printf("%s: read %zd bytes. %zd expected.\n", fileName, bytesRead, fileInfo.st_size);
        close(fd);
        return false;
    }
    close(fd);
    for (size_t i = constantSize; i < stringEnd; i++)
    {
        if (headerString[i] == '\r' || headerString[i] == '\n')
        {
            headerString[i] = 0;
            break;
        }
    }
    headerString[stringEnd] = 0;
    return true;
}

int main(void)
{
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
    {
        puts("curl_global_init failed");
        return 1;
    }
    if (!generateHeaderString())
    {
        puts("Failed to generate authentication header");
        return 1;
    }
    pthread_t workerThread;
    if (pthread_create(&workerThread, NULL, workerThreadLoop, NULL) != 0)
    {
        puts("pthread_create failed");
        return 1;
    }
    CURL *curl = curl_easy_init();
    if (curl == NULL)
    {
        puts("curl_easy_init failed");
        return 1;
    }
    struct curl_slist *headers = curl_slist_append(NULL, headerString);
    if (headers == NULL)
    {
        puts("curl_slist_append failed");
        return 1;
    }
    Buffer writeBuffer;
    writeBuffer.size = 0;
    writeBuffer.capacity = 16384;
    writeBuffer.data = malloc(writeBuffer.capacity);
    if (writeBuffer.data == NULL)
    {
        puts("malloc failed");
        return 1;
    }
    char errorBuffer[CURL_ERROR_SIZE];
    memset(errorBuffer, 0, CURL_ERROR_SIZE);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_URL, "https://lichess.org/api/stream/event");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeBuffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (curl_easy_perform(curl) != CURLE_OK)
    {
        printf("curl_easy_perform failed: %s\n", errorBuffer);
        return 1;
    }
    return 0;
}
