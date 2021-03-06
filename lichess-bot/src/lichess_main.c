#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lichess_random.h"
#include "lichess_game.h"

typedef struct Buffer
{
    size_t size;
    size_t capacity;
    char *data;
} Buffer;

typedef struct IDBuffer
{
    size_t size;
    size_t capacity;
    char *data;
    char id[8];
} IDBuffer;

typedef struct Challenge
{
    bool accept;
    char id[8];
    struct Challenge *next;
} Challenge;

typedef struct GameStart
{
    char id[8];
    struct GameStart *next;
} GameStart;

typedef struct GameMoves
{
    struct GameMoves *next;
    size_t numMoves;
    char id[8];
    uint16_t moves[];
} GameMoves;

static pthread_mutex_t challengeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gameStartMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gameMovesMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t challengeCond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t gameStartCond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t gameMovesCond = PTHREAD_COND_INITIALIZER;

static Challenge *challengeQueueFront = NULL;
static Challenge *challengeQueueBack = NULL;

static GameStart *gameStartQueueFront = NULL;
static GameStart *gameStartQueueBack = NULL;

static GameMoves *gameMovesQueueFront = NULL;
static GameMoves *gameMovesQueueBack = NULL;

static char headerString[128];

static bool seedRng(RngState *rng)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1)
    {
        printf("Failed to open /dev/urandom: %s\n", strerror(errno));
        return false;
    }
    uint64_t randomBuffer[2];
    if (read(fd, randomBuffer, 16) != 16)
    {
        close(fd);
        return false;
    }
    rng->state = randomBuffer[0];
    rng->inc = randomBuffer[1] | 1;
    close(fd);
    return true;
}

size_t stubCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return size * nmemb;
}

size_t eventCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realSize = size * nmemb;
    Buffer *writeBuffer = userdata;
    if (writeBuffer->size + realSize > writeBuffer->capacity)
    {
        size_t newCapacity = writeBuffer->capacity + writeBuffer->capacity;
        char *newData = realloc(writeBuffer->data, newCapacity);
        if (newData == NULL)
        {
            puts("realloc failed");
            exit(1);
        }
        writeBuffer->capacity = newCapacity;
        writeBuffer->data = newData;
    }
    const char *challengeCompareString = "challenge\"";
    const char *gameStartCompareString = "gameStart\"";
    size_t challengeCompareStringLen = strlen(challengeCompareString);
    size_t gameStartCompareStringLen = strlen(gameStartCompareString);
    for (size_t i = 0; i < realSize; i++)
    {
        char c = ptr[i];
        if (c == '\n')
        {
            if (writeBuffer->size > 72)
            {
                writeBuffer->data[writeBuffer->size] = 0;
                if (memcmp(&writeBuffer->data[9], challengeCompareString, challengeCompareStringLen) == 0)
                {
                    Challenge *challenge = malloc(sizeof(Challenge));
                    if (challenge == NULL)
                    {
                        puts("malloc failed");
                        exit(1);
                    }
                    if (strstr(writeBuffer->data, "variant\":{\"key\":\"standard") != NULL)
                    {
                        challenge->accept = true;
                    }
                    else
                    {
                        challenge->accept = false;
                    }
                    memcpy(challenge->id, &writeBuffer->data[39], 8);
                    challenge->next = NULL;
                    pthread_mutex_lock(&challengeMutex);
                    if (challengeQueueFront == NULL)
                    {
                        challengeQueueFront = challenge;
                    }
                    else
                    {
                        challengeQueueBack->next = challenge;
                    }
                    challengeQueueBack = challenge;
                    pthread_cond_signal(&challengeCond);
                    pthread_mutex_unlock(&challengeMutex);
                }
                else if (memcmp(&writeBuffer->data[9], gameStartCompareString, gameStartCompareStringLen) == 0)
                {
                    GameStart *gameStart = malloc(sizeof(GameStart));
                    if (gameStart == NULL)
                    {
                        puts("malloc failed");
                        exit(1);
                    }
                    memcpy(gameStart->id, &writeBuffer->data[62], 8);
                    gameStart->next = NULL;
                    pthread_mutex_lock(&gameStartMutex);
                    if (gameStartQueueFront == NULL)
                    {
                        gameStartQueueFront = gameStart;
                    }
                    else
                    {
                        gameStartQueueBack->next = gameStart;
                    }
                    gameStartQueueBack = gameStart;
                    pthread_cond_signal(&gameStartCond);
                    pthread_mutex_unlock(&gameStartMutex);
                }
            }
            writeBuffer->size = 0;
        }
        else
        {
            writeBuffer->data[writeBuffer->size] = c;
            writeBuffer->size += 1;
        }
    }
    return realSize;
}

size_t gameStartCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realSize = size * nmemb;
    IDBuffer *writeBuffer = userdata;
    if (writeBuffer->size + realSize > writeBuffer->capacity)
    {
        size_t newCapacity = writeBuffer->capacity + writeBuffer->capacity;
        char *newData = realloc(writeBuffer->data, newCapacity);
        if (newData == NULL)
        {
            puts("realloc failed");
            exit(1);
        }
        writeBuffer->capacity = newCapacity;
        writeBuffer->data = newData;
    }
    const char *movesCompareString = "type\":\"gameState\",\"moves\":\"";
    size_t movesCompareStringLen = strlen(movesCompareString);
    for (size_t i = 0; i < realSize; i++)
    {
        char c = ptr[i];
        if (c == '\n')
        {
            if (writeBuffer->size > movesCompareStringLen)
            {
                writeBuffer->data[writeBuffer->size] = 0;
                printf("%s\n\n", writeBuffer->data);
                char *movesString = strstr(writeBuffer->data, movesCompareString);
                if (movesString != NULL)
                {
                    movesString += movesCompareStringLen;
                    char *movesPtr = movesString;
                    size_t numMoves = 0;
                    while (*movesPtr > 47)
                    {
                        numMoves++;
                        while (*movesPtr > 47)
                        {
                            movesPtr++;
                        }
                        if (*movesPtr == '"')
                        {
                            break;
                        }
                        movesPtr++;
                    }
                    GameMoves *gameMoves = malloc(sizeof(GameMoves) + (sizeof(uint16_t) * numMoves));
                    if (gameMoves == NULL)
                    {
                        puts("malloc failed");
                        exit(1);
                    }
                    gameMoves->next = NULL;
                    gameMoves->numMoves = numMoves;
                    memcpy(gameMoves->id, writeBuffer->id, 8);
                    movesPtr = movesString;
                    for (size_t j = 0; j < numMoves; j++)
                    {
                        uint16_t moveFrom = *movesPtr - 97;
                        movesPtr++;
                        moveFrom += 8 * (8 - (*movesPtr - 48));
                        movesPtr++;
                        uint16_t moveTo = *movesPtr - 97;
                        movesPtr++;
                        moveTo += 8 * (8 - (*movesPtr - 48));
                        movesPtr++;
                        uint16_t move = (moveFrom << MOVE_FROM_SHIFT) | moveTo;
                        switch(*movesPtr)
                        {
                            case 'q':
                                move |= PAWN_PROMOTE_QUEEN;
                                movesPtr++;
                                break;
                            case 'b':
                                move |= PAWN_PROMOTE_BISHOP;
                                movesPtr++;
                                break;
                            case 'r':
                                move |= PAWN_PROMOTE_ROOK;
                                movesPtr++;
                                break;
                            case 'n':
                                move |= PAWN_PROMOTE_KNIGHT;
                                movesPtr++;
                                break;
                        }
                        movesPtr++;
                        gameMoves->moves[j] = move;
                    }
                    pthread_mutex_lock(&gameMovesMutex);
                    if (gameMovesQueueFront == NULL)
                    {
                        gameMovesQueueFront = gameMoves;
                    }
                    else
                    {
                        gameMovesQueueBack->next = gameMoves;
                    }
                    gameMovesQueueBack = gameMoves;
                    pthread_cond_signal(&gameMovesCond);
                    pthread_mutex_unlock(&gameMovesMutex);
                }
            }
            writeBuffer->size = 0;
        }
        else
        {
            writeBuffer->data[writeBuffer->size] = c;
            writeBuffer->size += 1;
        }
    }
    return realSize;
}

void *challengeThreadLoop(void *arg)
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stubCallback);

    const char *urlStart = "https://lichess.org/api/challenge/";
    size_t urlStartLen = strlen(urlStart);

    char url[64];
    memcpy(url, urlStart, urlStartLen);

    while (true)
    {
        pthread_mutex_lock(&challengeMutex);
        while (challengeQueueFront == NULL)
        {
            pthread_cond_wait(&challengeCond, &challengeMutex);
        }
        Challenge *challenge = challengeQueueFront;
        challengeQueueFront = challenge->next;
        pthread_mutex_unlock(&challengeMutex);
        bool accept = challenge->accept;
        memcpy(&url[urlStartLen], challenge->id, 8);
        free(challenge);
        if (accept)
        {
            strcpy(&url[urlStartLen + 8], "/accept");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
        }
        else
        {
            strcpy(&url[urlStartLen + 8], "/decline");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "reason=standard");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (curl_easy_perform(curl) != CURLE_OK)
        {
            printf("curl_easy_perform failed (challenge thread): %s\n", errorBuffer);
        }
    }
    return NULL;
}

void *gameStartThreadLoop(void *arg)
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
    IDBuffer writeBuffer;
    writeBuffer.size = 0;
    writeBuffer.capacity = 4096;
    writeBuffer.data = malloc(writeBuffer.capacity);
    if (writeBuffer.data == NULL)
    {
        puts("malloc failed");
        exit(1);
    }
    char errorBuffer[CURL_ERROR_SIZE];
    memset(errorBuffer, 0, CURL_ERROR_SIZE);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, gameStartCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeBuffer);

    const char *urlStart = "https://lichess.org/api/bot/game/stream/";
    size_t urlStartLen = strlen(urlStart);

    char url[64];
    memset(url, 0, 64);
    memcpy(url, urlStart, urlStartLen);

    while(true)
    {
        pthread_mutex_lock(&gameStartMutex);
        while (gameStartQueueFront == NULL)
        {
            pthread_cond_wait(&gameStartCond, &gameStartMutex);
        }
        GameStart *gameStart = gameStartQueueFront;
        gameStartQueueFront = gameStart->next;
        pthread_mutex_unlock(&gameStartMutex);
        memcpy(&url[urlStartLen], gameStart->id, 8);
        memcpy(writeBuffer.id, gameStart->id, 8);
        free(gameStart);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (curl_easy_perform(curl) != CURLE_OK)
        {
            printf("curl_easy_perform failed (game start thread): %s\n", errorBuffer);
        }
    }
    return NULL;
}

void *makeMoveThreadLoop(void *arg)
{
    RngState rng;
    if (!seedRng(&rng))
    {
        puts("Failed to seed RNG");
        exit(1);
    }
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stubCallback);

    const char *urlStart = "https://lichess.org/api/bot/game/";
    const char *urlEnd = "/move/";
    size_t urlStartLen = strlen(urlStart);
    size_t urlEndLen = strlen(urlEnd);
    size_t urlMove = urlStartLen + urlEndLen + 8;

    char url[64];
    memset(url, 0, 64);
    memcpy(url, urlStart, urlStartLen);
    memcpy(&url[urlStartLen + 8], urlEnd, urlEndLen);

    while(true)
    {
        pthread_mutex_lock(&gameMovesMutex);
        while (gameMovesQueueFront == NULL)
        {
            pthread_cond_wait(&gameMovesCond, &gameMovesMutex);
        }
        GameMoves *gameMoves = gameMovesQueueFront;
        gameMovesQueueFront = gameMoves->next;
        pthread_mutex_unlock(&gameMovesMutex);
        memcpy(&url[urlStartLen], gameMoves->id, 8);
        GameState state;
        initGameState(&state);
        for (size_t i = 0; i < gameMoves->numMoves; i++)
        {
            uint16_t move = gameMoves->moves[i];
            uint8_t moveTo = move & MOVE_TO_MASK;
            uint8_t moveFrom = (move & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
            uint8_t pieceType = state.board[moveFrom] & PIECE_TYPE_MASK;
            if (pieceType == PAWN)
            {
                if ((state.board[moveTo] == 0) && (moveTo % 8 != moveFrom % 8))
                {
                    move |= CASTLE_ENPASSANT_FLAG;
                }
            }
            else if (pieceType == KING)
            {
                int moveDistance = (int)moveFrom - (int)moveTo;
                if (abs(moveDistance) == 2)
                {
                    move |= CASTLE_ENPASSANT_FLAG;
                }
            }
            movePiece(move, &state);
        }
        free(gameMoves);
        uint16_t computerMove = getComputerMove(&state, &rng);
        uint8_t computerMoveTo = computerMove & MOVE_TO_MASK;
        uint8_t computerMoveFrom = (computerMove & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
        uint8_t pawnPromote = (computerMove & PAWN_PROMOTE_MASK) >> PAWN_PROMOTE_SHIFT;
        url[urlMove] = 'a' + (computerMoveFrom % 8);
        url[urlMove + 1] = '8' - (computerMoveFrom / 8);
        url[urlMove + 2] = 'a' + (computerMoveTo % 8);
        url[urlMove + 3] = '8' - (computerMoveTo / 8);
        switch(pawnPromote)
        {
            case QUEEN:
                url[urlMove + 4] = 'q';
                break;
            case BISHOP:
                url[urlMove + 4] = 'b';
                break;
            case ROOK:
                url[urlMove + 4] = 'r';
                break;
            case KNIGHT:
                url[urlMove + 4] = 'n';
                break;
            default:
                url[urlMove + 4] = 0;
                break;
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (curl_easy_perform(curl) != CURLE_OK)
        {
            printf("curl_easy_perform failed (game move thread): %s\n", errorBuffer);
        }
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
    pthread_t thread;
    if (pthread_create(&thread, NULL, challengeThreadLoop, NULL) != 0)
    {
        puts("pthread_create failed");
        return 1;
    }
    for (int i = 0; i < 16; i++)
    {
        if (pthread_create(&thread, NULL, gameStartThreadLoop, NULL) != 0)
        {
            puts("pthread_create failed");
            return 1;
        }
    }
    for (int i = 0; i < 4; i++)
    {
        if (pthread_create(&thread, NULL, makeMoveThreadLoop, NULL) != 0)
        {
            puts("pthread_create failed");
            return 1;
        }
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
    writeBuffer.capacity = 4096;
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, eventCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeBuffer);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (curl_easy_perform(curl) != CURLE_OK)
    {
        printf("curl_easy_perform failed (main thread): %s\n", errorBuffer);
        return 1;
    }
    return 0;
}
