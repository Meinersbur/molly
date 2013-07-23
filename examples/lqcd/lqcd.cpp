
#include <molly.h>
#include <complex>


typedef  std::complex<double> complex;

molly::array<complex, 32, 32, 32, 32, 4, 3> latticeIn;
molly::array<complex, 32, 32, 32, 32, 4, 3> latticeOut;
molly::array<complex, 32, 32, 32, 32, 4, 3, 3> gauge;

#define DIR_T 0
#define DIR_X 1
#define DIR_Y 2
#define DIR_Z 3

void HoppingMatrix() {

  for (int t = 0; t < 32; t+=1) {
    for (int x = 0; x < 32; x+=1) {
      for (int y = 0; y < 32; y+=1) {
        for (int z = 0; z < 32; z+=1) {

          // T+
          complex[2][3] projected;
          for (auto i =0; i < 3; i+=1) {
            projected[0,i] = latticeIn[t+1,x,y,z,0,i] + latticeIn[t+1,x,y,z,2,i];
            projected[1,i] = latticeIn[t+1,x,y,z,1,i] + latticeIn[t+1,x,y,z,3,i];
          }

          complex[2][3] multiplied;
          for (auto i =0; i < 3; i+=1) {
            multiplied[0][i] = 0;
            multiplied[1][i] = 0;
            for (auto j =0; j < 3; j+=1) {
              multiplied[0][i] += projected[0][j] * gauge[t,x,y,z,DIR_T,i,j];
            }
          }

          complex[4][3] expanded;
          for (auto i = 0; i < 3; i+=1) {
            expanded[0][i] = multiplied[0][i];
            expanded[1][i] = multiplied[1][i];
            expanded[2][i] = multiplied[0][i];
            expanded[3][i] = multiplied[1][i];
          }

          // Writeback
          for (auto s =0; s < 4; s+=1) {
            for (auto v =0; v < 3; v+=1) {
              latticeOut[t,x,y,z,s,v] = expanded[s,v];
            }
          }
        }
      }
    }
  }
}


int main(int argc, char *argv[]) {
  HoppingMatrix();
  return 0;
}
