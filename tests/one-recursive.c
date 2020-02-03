#include <stdio.h>


int main()
{
  struct Coords
  {
    float x, y, z;
  };
  struct Coords fav_3d_pt = {0.33333, 0.66667, 0.66667};
  struct Coords inverse_fav = {-fav_3d_pt.x, -fav_3d_pt.y, -fav_3d_pt.z};

  printf("The inverse of my favorite coordinates are: (%f, %f, %f)\n", inverse_fav.x, inverse_fav.y, inverse_fav.z);

  return 0;
}
