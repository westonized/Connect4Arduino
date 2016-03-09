#include <stdlib.h>
#include "connect4game.h"
#include "display.h"

Connect4Game *CreateConnect4Game() {
  //Serial.println("Created new game");
  Connect4Game *n = calloc(1, sizeof(Connect4Game));
  n->red = createBoard();
  n->green = createBoard();
  n->both = createBoard();
  n->winBoard = createBoard();
  return n;
}

int getTurnColour(Connect4Game *thiz) {
  return thiz->turn == TURN_RED ? RED : GREEN;
}

#define PER_STEP_SPEED_MS (100)
#define MOVES_MUST_BE_APART_BY_MS (500)

void animate(Connect4Game *thiz, unsigned long timeMs) {
  MoveAnimation *animation = thiz->animations;
  while (animation) {
    long t = timeMs - animation->startTime;
    int progress = t / PER_STEP_SPEED_MS;
    int y = progress;
    if (y > animation->targetY) {
      y = animation->targetY;
      animation->complete = 1;
    }
    display[animation->x][animation->targetY + 1] = OFF;
    display[animation->x][y + 1] = animation->colour;
    animation = animation->next;
  }
}

void tidyCompleteAnimations(Connect4Game *thiz) {
  MoveAnimation *animation = thiz->animations;
  while (animation && animation->complete) {
    free(animation);
    animation = animation->next;
    thiz->animations = animation;
  }
}

void resetGame(Connect4Game *thiz) {
  reset(thiz->red);
  reset(thiz->green);
  reset(thiz->both);
  reset(thiz->winBoard);
  thiz->pos = 0;
  thiz->turn = 0;
  thiz->lockedOutUntil = 0;
  thiz->winnerColour = 0;
}

void Connect4Game_processMove(Connect4Game *thiz, unsigned long timeMs, ButtonStates *states);

void Connect4Game_loop(Connect4Game *thiz, unsigned long timeMs, ButtonStates *states) {
  if (timeMs > thiz->lockedOutUntil && thiz->winnerColour == 0) {
    for (int x = 0; x < 8; x++)
      display[x][0] = OFF;
    display[thiz->pos][0] = getTurnColour(thiz);

    Connect4Game_processMove(thiz, timeMs, states);
  }

  draw(thiz->red, RED);
  draw(thiz->green, GREEN);

  animate(thiz, timeMs);
  tidyCompleteAnimations(thiz);

  if (!thiz->animations) {
    if (thiz->winnerColour) {
      if (timeMs % 600 > 400) {
        draw(thiz->winBoard, ORANGE);
      }
      int i = (timeMs / 100) % 7;
      if (i > 3) i = 6 - i;
      for (int x = i; x < 8 - i; x++)
        display[x][0] = thiz->winnerColour;
      int mode = readButtons(states, timeMs);
      if (mode & BTN_DOWN_CENTRE) {
        resetGame(thiz);
        thiz->lockedOutUntil = timeMs + MOVES_MUST_BE_APART_BY_MS;
      }
    }
  }
}

void MoveAnimation_addAnimation(MoveAnimation *thiz, MoveAnimation *animation) {
  while (thiz->next) {
    thiz = thiz->next;
  }
  thiz->next = animation;
}

void Connect4Game_addAnimation(Connect4Game *thiz, MoveAnimation *animation) {
  if (!thiz->animations) {
    thiz->animations = animation;
  }
  else {
    MoveAnimation_addAnimation(thiz->animations, animation);
  }
}

void Connect4Game_processMove(Connect4Game *thiz, unsigned long timeMs, ButtonStates *states) {
  int mode = readButtons(states, timeMs);

  if (mode & BTN_DOWN_LEFT) {
    thiz->pos = (thiz->pos + (CONNECT4_WIDTH - 1)) % CONNECT4_WIDTH;
  }

  if (mode & BTN_DOWN_RIGHT) {
    thiz->pos = (thiz->pos + 1) % CONNECT4_WIDTH;
  }

  if (mode & BTN_DOWN_CENTRE) {
    Board *b = thiz->turn == TURN_RED ? thiz->red : thiz->green;
    int turnColour = getTurnColour(thiz);
    for (int y = CONNECT4_HEIGHT - 1; y >= 0; y--) {
      if (!pos(thiz->both, thiz->pos, y)) {
        mark(b, thiz->pos, y);
        MoveAnimation *animation = calloc(1, sizeof(MoveAnimation));
        animation->x = thiz->pos;
        animation->targetY = y;
        animation->colour = turnColour;
        animation->startTime = timeMs;
        Connect4Game_addAnimation(thiz, animation);
        thiz->turn = 1 - thiz->turn;
        thiz->lockedOutUntil = timeMs + MOVES_MUST_BE_APART_BY_MS;
        break;
      }
    }
    createCombined(thiz->both, thiz->red, thiz->green);
    if (checkWin(b, thiz->winBoard)) {
      thiz->winnerColour = turnColour;
    }
  }
}

