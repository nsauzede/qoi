#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG2
#include "qoidneuf.h"

// #define DEBUG3
#ifdef DEBUG3
#define dprintf3(...)                                                          \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#else
#define dprintf3(...)                                                          \
  do {                                                                         \
  } while (0)
#endif

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
  enum { ibad, iqoi, ipam } itype = ibad;
  enum { obad, oqoi, opam, oppm } otype = obad;
  int usage = 0;
  if (!inf || !outf || strlen(inf) < 5 || strlen(outf) < 5) {
    usage = 1;
  } else {
    // printf("inf=%s outf=%s\n", inf, outf);
    if (!strcmp(inf + strlen(inf) - 4, ".qoi")) {
      itype = iqoi;
    } else if (!strcmp(inf + strlen(inf) - 4, ".pam")) {
      itype = ipam;
    }
    if (!strcmp(outf + strlen(outf) - 4, ".pam")) {
      otype = opam;
    } else if (!strcmp(outf + strlen(outf) - 4, ".ppm")) {
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
    pixels = qoidec(data, size, &desc, 0);
  } else if (itype == ipam) {
    char *ptr = data;
    char *endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (strcmp(ptr, "P7")) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM P7\n");
    ptr = endl;
    endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    int w, h, d, m;
    if (1 != sscanf(ptr, "WIDTH %d", &w)) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM width %d\n", w);
    ptr = endl;
    endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (1 != sscanf(ptr, "HEIGHT %d", &h)) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM height %d\n", h);
    ptr = endl;
    endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (1 != sscanf(ptr, "DEPTH %d", &d)) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM depth %d\n", d);
    ptr = endl;
    endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (1 != sscanf(ptr, "MAXVAL %d", &m)) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM maxval %d\n", m);
    ptr = endl;
    endl = strchr(ptr, ' ');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (strcmp(ptr, "TUPLTYPE")) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM TUPLTYPE\n");
    ptr = endl;
    endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (0 == strcmp(ptr, "RGB")) {
      if (d != 3) {
        printf("IPAM error %d\n", __LINE__);
        exit(1);
      }
      dprintf3("PAM RGB\n");
    } else if (0 == strcmp(ptr, "RGB_ALPHA")) {
      if (d != 4) {
        printf("IPAM error %d\n", __LINE__);
        exit(1);
      }
      dprintf3("PAM RGB_ALPHA\n");
    } else {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    ptr = endl;
    endl = strchr(ptr, '\n');
    if (!endl) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    *endl++ = 0;
    if (strcmp(ptr, "ENDHDR")) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    dprintf3("PAM ENDHDR\n");
    ptr = endl;

    if (m != 255) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    long rem = size - ((intptr_t)ptr - (intptr_t)data);
    long psize = w * h * d;
    dprintf3("psize=%ld rem=%ld\n", psize, rem);
    if (rem != psize) {
      printf("IPAM error %d\n", __LINE__);
      exit(1);
    }
    desc.width = w;
    desc.height = h;
    desc.channels = d;
    pixels = malloc(psize);
    memcpy(pixels, ptr, psize);
  }
  free(data);
  if (!pixels) {
    printf("decode error\n");
    exit(1);
  }
  if (desc.channels != 3 && desc.channels != 4) {
    printf("channels error %d\n", desc.channels);
    exit(1);
  }
  if (otype == opam) {
    FILE *out = fopen(outf, "wt");
    fprintf(out, "P7\n");
    fprintf(out, "WIDTH %d\nHEIGHT %d\n", desc.width, desc.height);
    fprintf(out, "DEPTH %d\n", desc.channels);
    fprintf(out, "MAXVAL 255\n");
    fprintf(out, "TUPLTYPE %s\n", desc.channels == 3 ? "RGB" : "RGB_ALPHA");
    fprintf(out, "ENDHDR\n");
    fwrite(pixels, desc.width * desc.height * desc.channels, 1, out);
    fclose(out);
  } else if (otype == oppm) {
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
