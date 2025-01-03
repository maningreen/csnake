#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

//please know that the majority of these were written in order of top to bottom
//with some obvious exceptions being main and #defines

// define constants
#define tickTime .1 // remove this when we get scaly thingy
// by scaly thing i mean decrease with time
#define headChar "@"
#define bodyChar "["
#define applChar "S"

void delay(float time) {
  usleep(1000000 * time);
  return;
}

struct positionData {
  int x;
  int y;
};

void movePositionData(struct positionData *in, int x, int y) {
  in->x += x;
  in->y += y;
  return;
}

void setPositionData(struct positionData *in, int x, int y) {
  in->x = x;
  in->y = y;
  return;
}

struct positionData addPositionData(struct positionData a,
                                    struct positionData b) {
  movePositionData(&a, b.x, b.y);
  return a;
}

void drawPositionData(struct positionData *in, char *characters) {
  wmove(stdscr, in->x, in->y);
  printw("%s", characters);
}

int getKeyPress() {
  int keyPressed;
  keyPressed = getch();
  return keyPressed;
}

struct positionData getDirFromKeyCode(int keyPressed) {
  struct positionData dir;
  if (keyPressed == 'a')
    setPositionData(&dir, 0, -1);
  else if (keyPressed == 'd')
    setPositionData(&dir, 0, 1);
  else if (keyPressed == 'w')
    setPositionData(&dir, -1, 0);
  else if (keyPressed == 's')
    setPositionData(&dir, 1, 0);
  else
    setPositionData(&dir, 2, 0);
  return dir;
}

bool isPos(int number) {
  return number >= 0;
}

void wrapPosition(struct positionData *position) {
  int maxX, maxY;               // cool things for max x and y (declar)
  getmaxyx(stdscr, maxX, maxY); // set ^

  if (position->x < maxX && isPos(position->x) && position->y < maxY &&
      isPos(position->y))
    return; // this is where it is nice and good and so we don't touch it
  int retX = position->x; // i learned you could use position->x after all of this.
  int retY = position->y; // i am too lazy to rewrite that

  // imagine a label here called checkX
  if (retX >= maxX && isPos(retX)) {
    retX = retX % maxX;
    goto checkY;
  }
  // now we know retx is negative yeah.
  if (retX <= maxX && !isPos(retX))
    retX = maxX - abs(retX);
checkY:
  if (retY >= maxY && isPos(retY)) {
    retY = retY % maxY;
    goto end;
  }
  // ITS THE SAME AS ^
  if (retY <= maxY && !isPos(retY))
    retY = maxY - abs(retY);
end:
  position->x = retX;
  position->y = retY;
  return;
}

void shmove(struct positionData *character, struct positionData *direction,
            int keyPressed) {
  struct positionData reqDirection;
  reqDirection = getDirFromKeyCode(keyPressed); // store the direction

  if (direction->x != reqDirection.x && direction->y != reqDirection.y &&
      reqDirection.x < 2)
    *direction = reqDirection;

  // adjusting the box (character) position according to the direction
  movePositionData(character, direction->x, direction->y);
  wrapPosition(character); // wrap the player around the limits
}

void setPosRandom(struct positionData *data, int maxX, int maxY) {
  // i doubt this needs explaining but basically we just set the two to random
  // numbers
  srand(time(NULL));
  data->x = rand() % maxX;
  data->y = rand() % maxY;
  return;
}

void adjustRandomPos(struct positionData *data) {
  // this is intended to be a quick adjustment for if its coliding the body
  data->x = -data->x + 1; // add number for a touch of randomness
  data->y = -data->y + 1;
  wrapPosition(data);
}

void initBody(struct positionData *bodyArray, int size) {
  size += -1;
  int maxX, maxY;               // cool things for max x and y (declar)
  getmaxyx(stdscr, maxX, maxY); // set ^
  while (size >= 0) {
    bodyArray[size].x = maxX / 2;
    bodyArray[size].y = maxY / 2;
    size += -1;
  }
}

void drawBody(struct positionData *bodyArray, int size) {
  size += -1;
  while (size >= 0) {
    drawPositionData(bodyArray + size, bodyChar);
    size += -1;
  }
}

void moveBody(struct positionData start, struct positionData *bodyArray,
              int bodyLength) {
  bodyLength += -1;
  while (bodyLength > 0) {
    *(bodyArray + bodyLength) = *(bodyArray + bodyLength - 1);
    bodyLength += -1;
  }
  *bodyArray = start;
}

void drawAll(struct positionData *head, struct positionData *body,
             int bodyLength, struct positionData *apple) {
  drawBody(body, bodyLength);
  drawPositionData(apple, applChar);
  drawPositionData(head, headChar); // draw the box as "0"
}

void addBodySegment(struct positionData **body, int *bodyLength) {
  struct positionData *newBody;
  newBody = (struct positionData *)realloc(*body, sizeof(struct positionData) *
                                                      (*bodyLength + 1));
  newBody[*bodyLength] = newBody[*bodyLength - 1];
  *bodyLength = *bodyLength + 1;
  *body = newBody;
}

bool getColliding(struct positionData a, struct positionData b) {
  movePositionData(&a, -b.x, -b.y);
  return a.x == 0 && a.y == 0;
}

bool getCollidingBody(struct positionData head, struct positionData *body,
                      int bodyLength) {
  bodyLength += -1;
  while (bodyLength > 4) {
    if (getColliding(head, body[bodyLength]))
      return true;
    bodyLength += -1;
  }
  return false;
}

int main() {
  initscr(); // Initiate screen

  int maxX, maxY;               // cool things for max x and y (declar)
  getmaxyx(stdscr, maxX, maxY); // set ^

  int maxLength = (maxX - 1) * (maxY - 1);

  bool won = false;

  noecho();

  nodelay(stdscr, true);

  curs_set(0); // hide the cursor

  struct positionData box;                   // our box friend
  setPositionData(&box, maxX / 2, maxY / 2); // set it to middle of screen

  int bodyLength = 1;
  struct positionData *body;
  body = (struct positionData *)malloc(sizeof(struct positionData) *
                                       bodyLength); // allocate our body
  initBody(body, bodyLength);

  struct positionData direction;
  setPositionData(&direction, 0, -1);

  struct positionData apple;
  setPosRandom(&apple, maxX,
               maxY); // it took me 7 tries to get the & not proud of that

  // main loop
  while (true) {

    int keyPressed = getKeyPress();
    if (keyPressed == 'Q' || keyPressed == 'q') // if its q we quit
      goto end; // pardon the use of gotos my time in assembly forced my hand

    // i like to shmove it shmove it
    shmove(&box, &direction, keyPressed);
    moveBody(box, body, bodyLength);

    if (getColliding(box, apple)) {
      addBodySegment(&body, &bodyLength);
    setRandApple:
      setPosRandom(&apple, maxX, maxY);
      if (bodyLength >= maxLength) {
        won = true;
        goto end;
      }
    checkApple:
      if (getCollidingBody(apple, body, bodyLength) ||
          getColliding(box, apple)) {
        adjustRandomPos(&apple);
        goto checkApple;
      }
    }

    if (getCollidingBody(box, body, bodyLength))
      goto end;

    erase();
    drawAll(&box, body, bodyLength, &apple);
    refresh();       // update screen
    delay(tickTime); // delay .1 seconds
  }
end:
  // ENDPROGRAM
  endwin();
  free(body);
  if (won)
    printf("You won! congrats! your length was %d\n", bodyLength);
  else
    printf("You lost, your length was %d\n", bodyLength);
  // and you too, people snooping around here
  // i doubt anyone is gonna see this but if you do
  // just know
  // you are increadible.
  // and thank you for your time of day
  // and more importantly thank yourself for giving yourself so many skills
  // give yourself some more credit. you deserve it. again i thank you
  // return 0;
  return 0;
}
