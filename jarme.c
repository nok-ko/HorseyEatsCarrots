#include <stdlib.h>
#include <stdio.h>

int neilrox(long seed, int cx, int cz){
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "java -jar new.jar overworld %ld %d %d", seed, cx, cz);
  int x,y,z;
  y=-1;
  FILE *fp = popen(buffer,"r");
  fscanf(fp, "%d %d %d", &x, &y, &z);
  pclose(fp);
  return y;
}

int main(void){
  long seed = 12345;//-9186205505753427990L;
  int y = neilrox(seed, 2, 3);
  printf("gotcha: %d\n",y);
  return 0;
}