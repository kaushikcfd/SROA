#include <stdio.h>
#include <stdbool.h>

struct Coords {
  int x, y, z;
};

int main()
{
  struct Coords fav_3d_pt;
  struct Coords fav_3d_pt_random;
  fav_3d_pt.x = 14;
  fav_3d_pt.y = 18;
  fav_3d_pt.z = 22;

  bool isNULL = (&fav_3d_pt == &fav_3d_pt_random);

  printf("Is it NULL?: %d.\n", isNULL);

  return 0;
}
