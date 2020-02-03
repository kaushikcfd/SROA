#include <stdio.h>

struct Coords {
  int x, y, z;
};

int main()
{
  struct Coords fav_3d_pt;
  fav_3d_pt.x = 14;
  fav_3d_pt.y = 18;
  fav_3d_pt.z = 22;

  printf("My favorite coordinate are: (%d, %d, %d)\n", fav_3d_pt.x, fav_3d_pt.y, fav_3d_pt.z);

  return 0;
}
