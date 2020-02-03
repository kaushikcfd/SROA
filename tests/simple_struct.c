#include <stdio.h>

struct Coords {
  float x, y, z;
};

int main()
{
  struct Coords fav_3d_pt;
  fav_3d_pt.x = 1.0/3.0;
  fav_3d_pt.y = 2.0/3.0;
  fav_3d_pt.z = 2.0/3.0;

  printf("My favorite coordinate are: (%f, %f, %f)\n", fav_3d_pt.x, fav_3d_pt.y, fav_3d_pt.z);
  printf("Reason begin its 2-norm is: %f\n", fav_3d_pt.x*fav_3d_pt.x + fav_3d_pt.y*fav_3d_pt.y + fav_3d_pt.z*fav_3d_pt.z);

  return 0;
}
