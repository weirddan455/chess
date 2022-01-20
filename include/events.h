#ifndef EVENTS_H
#define EVENTS_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool AIisThinking;

void leftClickEvent(int x, int y, bool playerGame);
void rightClickEvent(void);

#endif
