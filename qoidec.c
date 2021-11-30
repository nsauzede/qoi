#include <stdlib.h>
#include <stdio.h>

// #define DEBUG
#include "qoidec.h"
int
main(int argc, char* argv[])
{
  char* inf = 0;
  char* outf = 0;
  int arg = 1;
  if (arg < argc) {
    inf = argv[arg++];
    if (arg < argc) {
      outf = argv[arg++];
    }
  }
  if (!inf || !outf) {
    printf("Usage:\n");
    printf("%s <input.qoi> <output.ppm>\n", argv[0]);
    exit(1);
  }
  FILE* in = fopen(inf, "rb");
  fseek(in, 0, SEEK_END);
  long size = ftell(in);
  void* data = malloc(size);
  rewind(in);
  fread(data, size, 1, in);
  fclose(in);
  int w = 0, h = 0;
  unsigned char *pixels_, *pixels = qoidec(data, size, &w, &h, 4);
  free(data);
  pixels_ = pixels;
  FILE* out = fopen(outf, "wt");
  fprintf(out, "P3\n");
  fprintf(out, "%d %d\n", w, h);
  fprintf(out, "255\n");
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      int r, g, b;
      r = *pixels++;
      g = *pixels++;
      b = *pixels++;
      pixels++;
      fprintf(out, "%d %d %d\n", r, g, b);
    }
  }
  fclose(out);
  free(pixels_);
  return 0;
}
