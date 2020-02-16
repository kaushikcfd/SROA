#include <stdio.h>

struct Vec2D {
  int x, y;
};

struct Particle {
  struct Vec2D pos;
  struct Vec2D vel;
};

int main()
{
  struct Particle ball;
  ball.pos.x = 62;
  ball.pos.y = 82;

  ball.vel.x = -56;
  ball.vel.y = -26;

  printf("The ball is at '(%d, %d)' and is moving with '(%d, %d)'.\n", ball.pos.x, ball.pos.y, ball.vel.x, ball.vel.y);
  return 0;
}
