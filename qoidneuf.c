#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG2
#include "qoidneuf.h"
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
  enum { ibad, iqoi } itype = ibad;
  enum { obad, oppm, oqoi } otype = obad;
  int usage = 0;
  if (!inf || !outf || strlen(inf) < 5 || strlen(outf) < 5) {
    usage = 1;
  } else {
    // printf("inf=%s outf=%s\n", inf, outf);
    if (!strcmp(inf + strlen(inf) - 4, ".qoi")) {
      itype = iqoi;
    }
    if (!strcmp(outf + strlen(outf) - 4, ".ppm")) {
      otype = oppm;
    } else if (!strcmp(outf + strlen(outf) - 4, ".qoi")) {
      otype = oqoi;
    }
  }
  // printf("itype=%d otype=%d\n", itype, otype);
  if (usage || itype == ibad || otype == obad) {
    printf("Usage:\n");
    printf("%s <input.qoi> <output.ppm>\n", argv[0]);
    printf("%s <input.qoi> <output.qoi>\n", argv[0]);
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

  unsigned char *pixels = 0;
  qoi_desc desc = {.height = 0, .width = 0, .channels = 4, .colorspace = 0};
  if (itype == iqoi) {
    pixels = qoidec(data, size, &desc, 3);
  }
  free(data);
  if (!pixels) {
    printf("decode error\n");
    exit(1);
  }
  if (otype == oppm) {
    unsigned char *pixels_ = pixels;
    FILE *out = fopen(outf, "wt");
    fprintf(out, "P3\n");
    fprintf(out, "%d %d\n", desc.width, desc.height);
    fprintf(out, "255\n");
    for (int j = 0; j < desc.height; j++) {
      for (int i = 0; i < desc.width; i++) {
        int r, g, b;
        r = *pixels_++;
        g = *pixels_++;
        b = *pixels_++;
        if (desc.channels == 4)
          pixels_++;
        fprintf(out, "%d %d %d\n", r, g, b);
      }
    }
    fclose(out);
  } else if (otype == oqoi) {
    int enc_size = 0;
    printf("\nEncoding....\n");
    void *enc_p = qoienc(pixels, &desc, &enc_size);
    printf("enc_p=%p, enc_size=%d\n", enc_p, enc_size);
    FILE *out = fopen(outf, "wb");
    fwrite(enc_p, enc_size, 1, out);
    fclose(out);
    free(enc_p);
  }
  free(pixels);
  return 0;
}
