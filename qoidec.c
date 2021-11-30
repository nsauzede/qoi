#include <stdio.h>
#include <stdlib.h>

// #define DEBUG
#include "qoidec.h"
int main(int argc, char *argv[]) {
  char *inf = 0;
  char *outf = 0;
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
  FILE *in = fopen(inf, "rb");
  fseek(in, 0, SEEK_END);
  long size = ftell(in);
  void *data = malloc(size);
  rewind(in);
  if (fread(data, 1, size, in) < size) {
    printf("read error\n");
    exit(1);
  }
  fclose(in);
  qoi_desc desc = {.height = 0, .width = 0, .channels = 3, .colorspace = 0};
  unsigned char *pixels_, *pixels = qoidec(data, size, &desc, 3);
  free(data);
  pixels_ = pixels;
  FILE *out = fopen(outf, "wt");
  fprintf(out, "P3\n");
  fprintf(out, "%d %d\n", desc.width, desc.height);
  fprintf(out, "255\n");
  for (int j = 0; j < desc.height; j++) {
    for (int i = 0; i < desc.width; i++) {
      int r, g, b;
      r = *pixels++;
      g = *pixels++;
      b = *pixels++;
      if (desc.channels == 4)
        pixels++;
      fprintf(out, "%d %d %d\n", r, g, b);
    }
  }
  fclose(out);
  free(pixels_);
  return 0;
}
