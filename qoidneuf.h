#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

// #define DEBUG
#ifdef DEBUG
#define dprintf(...)                                                           \
  do {                                                                         \
    printf(__VA_ARGS__);                                                       \
  } while (0)
#else
#define dprintf(...)                                                           \
  do {                                                                         \
  } while (0)
#endif

#ifdef DEBUG2
#define dprintf2(...)                                                          \
  do {                                                                         \
    printf(__VA_ARGS__);                                                       \
  } while (0)
#else
#define dprintf2(...)                                                          \
  do {                                                                         \
  } while (0)
#endif

typedef uint8_t u8;
typedef uint16_t u16;

typedef union {
  u8 data[3];
  struct {
    u8 : 6;
    u8 tag2 : 2;
  };
  struct {
    u8 : 5;
    u8 tag3 : 3;
  };
  struct {
    u8 : 4;
    u8 tag4 : 4;
  };

  struct {
    u8 idx : 6;
    u8 tag : 2;
  } index;

  struct {
    u8 run : 5;
    u8 tag : 3;
  } run8;

  struct {
    u8 run128 : 5;
    u8 tag : 3;
    u8 run70 : 8;
  } run16;

  struct {
    u8 db : 2;
    u8 dg : 2;
    u8 dr : 2;
    u8 tag : 2;
  } diff8;

  struct {
    u8 dr : 5;
    u8 tag : 3;
    u8 db : 4;
    u8 dg : 4;
  } diff16;

  struct {
    u8 dr41 : 4;
    u8 tag : 4;
    u8 db43 : 2;
    u8 dg : 5;
    u8 dr00 : 1;
    u8 da : 5;
    u8 db20 : 3;
  } diff24;

  struct {
    u8 has_a : 1;
    u8 has_b : 1;
    u8 has_g : 1;
    u8 has_r : 1;
    u8 tag : 4;
  } color;
} chunk_t;

typedef struct {
  char magic[4];
  uint32_t width;
  uint32_t height;
  uint8_t channels;
  uint8_t colorspace;
  unsigned char data[0];
  // chunk_t chunks[0];
} qoi_t;

typedef struct {

  int r, g, b, a;

} rgba;

#ifndef QOIDEC_IMPL
typedef struct {
  unsigned int width;
  unsigned int height;
  unsigned char channels;
  unsigned char colorspace;
} qoi_desc;
#endif

typedef struct {
  u8 r, g, b, a;
} rgba_t;

// returns 1 if pixels equal
static int pix_equal(rgba_t *p1, rgba_t *p2, int channels) {
  return 0 == memcmp(p1, p2, channels);
}

void *qoienc(const void *data, const qoi_desc *desc, int *out_len) {
  int channels = desc->channels;
  int psize = desc->width * desc->height;
  int msize = sizeof(qoi_t) + psize * (desc->channels + 1) +
              4; // worst case max size : header +  QOI_COLOR
                 // (channels + 1 bytes) + 4 bytes padding (zeroes)
  qoi_t *qoi = (qoi_t *)calloc(1, msize);
  qoi->magic[0] = 'q';
  qoi->magic[1] = 'o';
  qoi->magic[2] = 'i';
  qoi->magic[3] = 'f';
  qoi->width = htonl(desc->width);
  qoi->height = htonl(desc->height);
  qoi->channels = desc->channels;
  qoi->colorspace = desc->colorspace;
  typedef union {
    u8 *byte;
    rgba_t *rgba;
  } pix_t;
  pix_t pix = {(u8 *)data};
  rgba_t old = {0, 0, 0, 255};
  rgba_t *old_p = &old;
  rgba_t lut_[64];
  memset(lut_, 0, sizeof(lut_));
  rgba_t *lut[64];
  for (int i = 0; i < sizeof(lut) / sizeof(lut[0]); i++) {
    lut[i] = &lut_[i];
  }
  int run = 0;
  int size = 0;
  for (int i = 0; i < psize; i++, pix.byte += channels) {
    chunk_t ch = {0};
    int index = pix.rgba->r ^ pix.rgba->g ^ pix.rgba->b;
    if (channels == 4) {
      index ^= pix.rgba->a;
    } else {
      index ^= 255;
    }
    index %= 64;
    if (pix_equal(old_p, pix.rgba, channels)) {
      if (!run) {
        lut[index] = pix.rgba;
      }
      run++;
      continue;
    }
    if (run) {
      if (run <= 32) {
        ch.run8.tag = 0x2;
        ch.run8.run = run - 1;
        dprintf2("RUN8 p=%d %02x %d\n", 14 + size, ch.data[0], run - 1);
        qoi->data[size++] = ch.data[0];
      } else {
        ch.run16.tag = 0x3;
        ch.run16.run70 = (run - 33) & 0xff;
        ch.run16.run128 = (run - 33) >> 8;
        dprintf2("RUN16 p=%d %02x %d\n", 14 + size, ch.data[0], run - 33);
        qoi->data[size++] = ch.data[0];
        qoi->data[size++] = ch.data[1];
      }
      run = 0;
    }
    // dprintf2("compare index=%d %d %d %d %d with channels=%d %d %d %d %d\n",
    // index,
    //        lut[index]->r, lut[index]->g, lut[index]->b, lut[index]->a,
    //        channels, pix.rgba->r, pix.rgba->g, pix.rgba->b, pix.rgba->a);
    if (pix_equal(lut[index], pix.rgba, channels)) {
      ch.index.tag = 0x0;
      ch.index.idx = index;
      dprintf2("INDEX p=%d %02x %d\n", 14 + size, ch.data[0], index);
      qoi->data[size++] = ch.data[0];
    } else {
      int dr = pix.rgba->r - old_p->r;
      int dg = pix.rgba->g - old_p->g;
      int db = pix.rgba->b - old_p->b;
      int da = 0;
      if (channels == 4) {
        da = pix.rgba->a - old_p->a;
      }
      if (dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1) {
        ch.diff8.tag = 0x2;
        ch.diff8.dr = dr + 2;
        ch.diff8.dg = dg + 2;
        ch.diff8.db = db + 2;
        dprintf2("DIFF8 p=%d %02x %d %d %d\n", 14 + size, ch.data[0], dr, dg,
                 db);
        qoi->data[size++] = ch.data[0];
      } else if (dr >= -16 && dr <= 15 && dg >= -8 && dg <= 7 && db >= -8 &&
                 db <= 7) {
        ch.diff16.tag = 0x6;
        ch.diff16.dr = dr + 16;
        ch.diff16.dg = dg + 8;
        ch.diff16.db = db + 8;
        dprintf2("DIFF16 p=%d %02x %d %d %d\n", 14 + size, ch.data[0], dr, dg,
                 db);
        qoi->data[size++] = ch.data[0];
        qoi->data[size++] = ch.data[1];
      } else if (dr >= -16 && dr <= 15 && dg >= -16 && dg <= 15 && db >= -16 &&
                 db <= 15 && da >= -16 && da <= 15) {
        ch.diff24.tag = 0xe;
        ch.diff24.dr00 = (dr + 16) & 1;
        ch.diff24.dr41 = (dr + 16) >> 1;
        ch.diff24.dg = dg + 16;
        ch.diff24.db20 = (db + 16) & 0x7;
        ch.diff24.db43 = (db + 16) >> 3;
        ch.diff24.da = da + 16;
        dprintf2("DIFF24 p=%d %02x %d %d %d %d\n", 14 + size, ch.data[0], dr,
                 dg, db, da);
        qoi->data[size++] = ch.data[0];
        qoi->data[size++] = ch.data[1];
        qoi->data[size++] = ch.data[2];
      } else {
        ch.color.tag = 0xf;
        if (pix.rgba->r != old_p->r) {
          ch.color.has_r = 1;
        }
        if (pix.rgba->g != old_p->g) {
          ch.color.has_g = 1;
        }
        if (pix.rgba->b != old_p->b) {
          ch.color.has_b = 1;
        }
        if (channels == 4 && pix.rgba->a != old_p->a) {
          ch.color.has_a = 1;
        }
        dprintf2("COLOR p=%d %02x", 14 + size, ch.data[0]);
        qoi->data[size++] = ch.data[0];
        if (ch.color.has_r) {
          dprintf2(" r=%d", pix.rgba->r);
          qoi->data[size++] = pix.rgba->r;
        }
        if (ch.color.has_g) {
          dprintf2(" g=%d", pix.rgba->g);
          qoi->data[size++] = pix.rgba->g;
        }
        if (ch.color.has_b) {
          dprintf2(" b=%d", pix.rgba->b);
          qoi->data[size++] = pix.rgba->b;
        }
        if (ch.color.has_a) {
          dprintf2(" a=%d", pix.rgba->a);
          qoi->data[size++] = pix.rgba->a;
        }
        dprintf2("\n");
      }
    }
    lut[index] = pix.rgba;
    // dprintf2("lut index=%d %d %d %d %d\n", index, pix.rgba->r, pix.rgba->g,
    //        pix.rgba->b, channels == 4 ? pix.rgba->a : 255);
    old_p = pix.rgba;
  }
  if (run) {
    chunk_t ch = {0};
    ch.run8.tag = 0x2;
    ch.run8.run = run - 1;
    dprintf2("RUN8 p=%d %d\n", 14 + size, run);
    run = 0;
    qoi->data[size++] = ch.data[0];
  }
  *out_len = 14 + size + 4;
  return qoi;
}

void *qoidec(const void *data, int size, qoi_desc *desc, int channels) {
  void *ret = 0;
  qoi_t *qoi = (qoi_t *)data;
  if (0 != memcmp(qoi->magic, "qoif", 4)) {
    printf("Invalid qoif magic\n");
    return 0;
  }
  if (channels != qoi->channels) {
    if (!channels) {
      channels = qoi->channels;
      // fprintf(stderr, "spoofing channels to %d\n", channels);
    } else {
      printf("Invalid channels=%d (%d)\n", channels, qoi->channels);
      return 0;
    }
  }
  if (qoi->colorspace != 0) {
    printf("Invalid colorspace=%d\n", qoi->colorspace);
    return 0;
  }
  int w = ntohl(qoi->width);
  int h = ntohl(qoi->height);
  // dprintf("qoi_t %zd\n", sizeof(qoi_t));
  // dprintf("QOIF: %" PRIu32 "x%" PRIu32 " %d ch %d co\n", w, h,
  //         (int)qoi->channels, (int)qoi->colorspace);
  // dprintf("chunk_t=%zd\n", sizeof(chunk_t));
  // chunk_t ch0;
  // dprintf("color=%zd\n", sizeof(ch0.color));
  // dprintf("diff24=%zd\n", sizeof(ch0.diff24));
  // dprintf("diff16=%zd\n", sizeof(ch0.diff16));
  // dprintf("diff8=%zd\n", sizeof(ch0.diff8));
  // dprintf("run16=%zd\n", sizeof(ch0.run16));
  // dprintf("run8=%zd\n", sizeof(ch0.run8));
  // dprintf("index=%zd\n", sizeof(ch0.index));
  int npix = 0;
  int r, g, b = g = r = 0, a = 255;
  //   int old_r = 0, old_g = 0, old_b = 0, old_a = 255;
  rgba lut[64];
  memset(lut, 0, sizeof(lut));
  desc->channels = channels;
  desc->colorspace = qoi->colorspace;
  desc->width = w;
  desc->height = h;
  //   printf("w=%d h=%d\n", *out_w, *out_h);
  unsigned char *pixels = (unsigned char *)malloc(w * h * channels);
  ret = pixels;
  //   printf("ret=%p\n", ret);
  for (int i = 0; i < size;) {
    // printf("i=%3d ", i);
    if (npix >= w * h)
      break;
#ifdef DEBUG
    int p = 14 + i;
#endif
    chunk_t *ch = (chunk_t *)&qoi->data[i];
    // if (old_r != r || old_g != g || old_b != b || old_a != a) {
    int idx = (r ^ g ^ b ^ a) % 64;
    lut[idx] = (rgba){r, g, b, a};
    // old_r = r;
    // old_g = g;
    // old_b = b;
    // old_a = a;
    // dprintf("lut index=%d %d %d %d %d\n", idx, r, g, b, a);
    // }
    switch (ch->tag2) {
    case 0x0:
      dprintf("INDEX p=%d %02x %d\n", p, ch->data[0], ch->index.idx);
      // dprintf("INDEX %d\n", i);
      // dprintf("INDEX\n");
      i += sizeof(ch->index);
      r = lut[ch->index.idx].r;
      g = lut[ch->index.idx].g;
      b = lut[ch->index.idx].b;
      break;
    case 0x1:
      // dprintf("RUN8/16\n");
      switch (ch->tag3) {
      case 0x2:
        // dprintf("RUN8 p=%d\n", p);
        dprintf("RUN8 p=%d %02x %d\n", p, ch->data[0], ch->run8.run);
        i += sizeof(ch->run8);
        // dprintf("RUN8 %d\n", i - _i);
        // dprintf("RUN8\n");
        npix += ch->run8.run + 1 - 1;
        for (int j = 0; j < ch->run8.run + 1 - 1; j++) {
          *pixels++ = r;
          *pixels++ = g;
          *pixels++ = b;
          if (channels == 4)
            *pixels++ = a;
        }
        break;
      case 0x3: {
        int run = (ch->run16.run128 << 8) + ch->run16.run70 - 1;
        dprintf("RUN16 p=%d %02x %u\n", p, ch->data[0], run + 1);
        i += sizeof(ch->run16);
        // dprintf("RUN16 %d\n", i - _i);
        // dprintf("RUN16\n");
        npix += run;
        // printf("npix=%d\n", npix);
        for (int j = 0; j < run; j++) {
          *pixels++ = r;
          *pixels++ = g;
          *pixels++ = b;
          if (channels == 4)
            *pixels++ = a;
        }
        break;
      }
      default:
        printf("unhandled tag3 %d\n", ch->tag3);
        return 0;
      }
      break;
    case 0x2:
      // dprintf("DIFF8\n");
      i += sizeof(ch->diff8);
      r = r + ch->diff8.dr - 2;
      g = g + ch->diff8.dg - 2;
      b = b + ch->diff8.db - 2;
      dprintf("DIFF8 p=%d %02x %d %d %d\n", p, ch->data[0], ch->diff8.dr - 2,
              ch->diff8.dg - 2, ch->diff8.db - 2);
      break;
    case 0x3:
      // dprintf("DIFF16/24/COLOR\n");
      switch (ch->tag3) {
      case 0x6:
        // dprintf("DIFF16\n");
        i += sizeof(ch->diff16);
        r = r + ch->diff16.dr - 16;
        g = g + ch->diff16.dg - 8;
        b = b + ch->diff16.db - 8;
        dprintf("DIFF16 p=%d %02x %d %d %d\n", p, ch->data[0],
                ch->diff16.dr - 16, ch->diff16.dg - 8, ch->diff16.db - 8);
        break;
      default:
        switch (ch->tag4) {
        case 0xe: {
          int dr = ((ch->diff24.dr41 << 1) + (ch->diff24.dr00)) - 16;
          int dg = ch->diff24.dg - 16;
          // dprintf("r 41=%d 00=%d\n", ch->diff24.dr41, ch->diff24.dr00);
          int db = ((ch->diff24.db43 << 3) + ch->diff24.db20) - 16;
          // dprintf("b 43=%d 20=%d\n", ch->diff24.db43, ch->diff24.db20);
          // dprintf("DIFF24\n");
          i += sizeof(ch->diff24);
          // i += 3;
          r = r + dr;
          g = g + dg;
          b = b + db;
          dprintf("DIFF24 p=%d %02x %d %d %d %d\n", p, ch->data[0], dr, dg, db,
                  ch->diff24.da - 16);
          break;
        }
        default: {
          // dprintf("COLOR p=%d\n", p);
          // dprintf("COLOR p=%d", p);
          i++;
          if (ch->color.has_r) {
            //   dprintf(" r=%d", qoi->data[i]);
            r = qoi->data[i];
            i++;
          }
          if (ch->color.has_g) {
            //   dprintf(" g=%d", qoi->data[i]);
            g = qoi->data[i];
            i++;
          }
          if (ch->color.has_b) {
            //   dprintf(" b=%d", qoi->data[i]);
            b = qoi->data[i];
            i++;
          }
          if (ch->color.has_a) {
            //   dprintf(" a=%d", qoi->data[i]);
            a = qoi->data[i];
            i++;
          }
          // dprintf(" %d", idx);
          // dprintf("\n");
          // dprintf("COLOR p=%d %02x\n", p, qoi->data[i]);
          // dprintf("COLOR %d\n", i - _i);
          // dprintf("COLOR\n");
#ifdef DEBUG
          dprintf("COLOR p=%d %02x", p, ch->data[0]);
          if (ch->color.has_r) {
            dprintf(" r=%d", r);
          }
          if (ch->color.has_g) {
            dprintf(" g=%d", g);
          }
          if (ch->color.has_b) {
            dprintf(" b=%d", b);
          }
          if (ch->color.has_a) {
            dprintf(" a=%d", a);
          }
          dprintf("\n");
#endif
          break;
        }
        }
        break;
      }
      break;
    default:
      printf("unhandled tag2 %d\n", ch->tag2);
      return 0;
    }
    npix++;
    *pixels++ = r;
    *pixels++ = g;
    *pixels++ = b;
    if (channels == 4)
      *pixels++ = a;
  }
  //   dprintf("npix=%d\n", npix);
  return ret;
}
