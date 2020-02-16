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

  int fav_3d_pt_2norm_sq = fav_3d_pt.x*fav_3d_pt.x + fav_3d_pt.y*fav_3d_pt.y + fav_3d_pt.z*fav_3d_pt.z;

  printf("My favorite coordinate are: (%d, %d, %d)\n", fav_3d_pt.x, fav_3d_pt.y, fav_3d_pt.z);
  printf("And its distance is: %d.\n", fav_3d_pt_2norm_sq);

  return 0;
}
