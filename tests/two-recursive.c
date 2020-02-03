#include <stdio.h>


int main()
{
  struct Coords
  {
    float x, y, z;
  };
  struct Coords fav_3d_pt = {0.33333, 0.66667, 0.66667};
  struct Coords inverse_fav = {-fav_3d_pt.x, -fav_3d_pt.y, -fav_3d_pt.z};
  struct Coords origin = {inverse_fav.x+fav_3d_pt.x, inverse_fav.y+fav_3d_pt.y, inverse_fav.z+fav_3d_pt.z};

  printf("Since cartesian coords comprise an algebraic space, x+(-x) should be (%f, %f, %f)\n", origin.x, origin.y, origin.z);

  return 0;
}
