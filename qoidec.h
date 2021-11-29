#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

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

typedef uint8_t u8;
typedef uint16_t u16;
typedef struct
{
  int r, g, b, a;
} rgba;
typedef union
{
  struct
  {
    u8 : 6;
    u8 tag2 : 2;
  };
  struct
  {
    u8 : 5;
    u8 tag3 : 3;
  };
  struct
  {
    u8 : 4;
    u8 tag4 : 4;
  };
  struct
  {
    u8 idx : 6;
    u8 tag : 2;
  } index;
  struct
  {
    u8 run : 5;
    u8 tag : 3;
  } run8;
  struct
  {
    u16 run : 13;
    u8 tag : 3;
  } run16;
  struct
  {
    u8 db : 2;
    u8 dg : 2;
    u8 dr : 2;
    u8 tag : 2;
  } diff8;
  struct
  {
    u8 dr : 5;
    u8 tag : 3;
    u8 db : 4;
    u8 dg : 4;
  } diff16;
  struct
  {
    u8 dr41 : 4;
    u8 tag : 4;
    u8 db43 : 2;
    u8 dg : 5;
    u8 dr00 : 1;
    u8 da : 5;
    u8 db20 : 3;
  } diff24;
  struct
  {
    u8 has_a : 1;
    u8 has_b : 1;
    u8 has_g : 1;
    u8 has_r : 1;
    u8 tag : 4;
  } color;
} chunk_t;
typedef struct
{
  char magic[4];
  unsigned short width;
  unsigned short height;
  unsigned int size;
  unsigned char data[0];
  // chunk_t chunks[0];
} qoi_t;

void*
qoidec(const void* data, int size, int* out_w, int* out_h, int channels)
{
  void* ret = 0;
  qoi_t* qoi = (qoi_t*)data;
  if (0 != memcmp(qoi->magic, "qoif", 4)) {
    printf("Invalid qoif magic\n");
    return 0;
  }
  //   dprintf("QOIF: %dx%d (%d)\n", qoi->width, qoi->height, (int)qoi->size);
  //   dprintf("chunk_t=%zd\n", sizeof(chunk_t));
  //   chunk_t ch0;
  //   dprintf("color=%zd\n", sizeof(ch0.color));
  //   dprintf("diff24=%zd\n", sizeof(ch0.diff24));
  //   dprintf("diff16=%zd\n", sizeof(ch0.diff16));
  //   dprintf("diff8=%zd\n", sizeof(ch0.diff8));
  //   dprintf("run16=%zd\n", sizeof(ch0.run16));
  //   dprintf("run8=%zd\n", sizeof(ch0.run8));
  //   dprintf("index=%zd\n", sizeof(ch0.index));
  int npix = 0;
  int r, g, b = g = r = 0, a = 255;
  //   int old_r = 0, old_g = 0, old_b = 0, old_a = 255;
  rgba lut[64];
  for (int i = 0; i < 64; i++) {
    lut[i] = (rgba){ 128, 128, 128 };
  }
  *out_w = qoi->width;
  *out_h = qoi->height;
  unsigned char* pixels = malloc(*out_w * *out_h * channels);
  ret = pixels;
  for (int i = 0; i < qoi->size;) {
    if (npix >= qoi->width * qoi->height)
      break;
#ifdef DEBUG
    int p = 12 + i;
#endif
    chunk_t* ch = (chunk_t*)&qoi->data[i];
    // if (old_r != r || old_g != g || old_b != b || old_a != a) {
    int idx = (r ^ g ^ b ^ a) % 64;
    lut[idx] = (rgba){ r, g, b, a };
    // old_r = r;
    // old_g = g;
    // old_b = b;
    // old_a = a;
    //   dprintf("lut index=%d\n", idx);
    // }
    switch (ch->tag2) {
      case 0x0:
        dprintf("INDEX p=%d %d\n", p, ch->index.idx);
        // dprintf("INDEX %d\n", i);
        // dprintf("INDEX\n");
        i += sizeof(ch->index);
        npix++;
        r = lut[ch->index.idx].r;
        g = lut[ch->index.idx].g;
        b = lut[ch->index.idx].b;
        break;
      case 0x1:
        // dprintf("RUN8/16\n");
        switch (ch->tag3) {
          case 0x2:
            // dprintf("RUN8 p=%d\n", p);
            dprintf("RUN8 p=%d %d\n", p, ch->run8.run);
            i += sizeof(ch->run8);
            // dprintf("RUN8 %d\n", i - _i);
            // dprintf("RUN8\n");
            npix += ch->run8.run + 1;
            for (int j = 0; j < ch->run8.run + 1 - 1; j++) {
              *pixels++ = r;
              *pixels++ = g;
              *pixels++ = b;
              *pixels++ = a;
            }
            break;
          case 0x3:
            dprintf("RUN16 p=%d %d\n", p, ch->run16.run);
            i += sizeof(ch->run16);
            // dprintf("RUN16 %d\n", i - _i);
            // dprintf("RUN16\n");
            npix += ch->run16.run + 33;
            for (int j = 0; j < ch->run16.run + 33 - 1; j++) {
              *pixels++ = r;
              *pixels++ = g;
              *pixels++ = b;
              *pixels++ = a;
            }
            break;
          default:
            printf("unhandled tag3 %d\n", ch->tag3);
            return 0;
        }
        break;
      case 0x2:
        // dprintf("DIFF8\n");
        i += sizeof(ch->diff8);
        npix++;
        r = r + ch->diff8.dr - 1;
        g = g + ch->diff8.dg - 1;
        b = b + ch->diff8.db - 1;
        // if (old_r != r || old_g != g || old_b != b || old_a != a) {
        //   int idx = (r ^ g ^ b ^ a) % 64;
        //   lut[idx] = (rgba){ r, g, b, a };
        //   old_r = r;
        //   old_g = g;
        //   old_b = b;
        //   old_a = a;
        //   dprintf("lut index=%d\n", idx);
        // }
        dprintf("DIFF8 p=%d %d %d %d\n",
                p,
                ch->diff8.dr - 1,
                ch->diff8.dg - 1,
                ch->diff8.db - 1);
        break;
      case 0x3:
        // dprintf("DIFF16/24/COLOR\n");
        switch (ch->tag3) {
          case 0x6:
            // dprintf("DIFF16\n");
            i += sizeof(ch->diff16);
            npix++;
            r = r + ch->diff16.dr - 15;
            g = g + ch->diff16.dg - 7;
            b = b + ch->diff16.db - 7;
            // if (old_r != r || old_g != g || old_b != b || old_a != a) {
            //   int idx = (r ^ g ^ b ^ a) % 64;
            //   lut[idx] = (rgba){ r, g, b, a };
            //   old_r = r;
            //   old_g = g;
            //   old_b = b;
            //   old_a = a;
            //   dprintf("lut index=%d\n", idx);
            // }
            dprintf("DIFF16 p=%d %d %d %d\n",
                    p,
                    ch->diff16.dr - 15,
                    ch->diff16.dg - 7,
                    ch->diff16.db - 7);
            break;
          default:
            switch (ch->tag4) {
              case 0xe: {
                int dr = ((ch->diff24.dr41 << 1) + (ch->diff24.dr00)) - 15;
                int dg = ch->diff24.dg - 15;
                // dprintf("r 41=%d 00=%d\n", ch->diff24.dr41, ch->diff24.dr00);
                int db = ((ch->diff24.db43 << 3) + ch->diff24.db20) - 15;
                // dprintf("b 43=%d 20=%d\n", ch->diff24.db43, ch->diff24.db20);
                // dprintf("DIFF24\n");
                // i += sizeof(ch->diff24);
                i += 3;
                npix++;
                r = r + dr;
                g = g + dg;
                b = b + db;
                // if (old_r != r || old_g != g || old_b != b || old_a != a) {
                //   int idx = (r ^ g ^ b ^ a) % 64;
                //   lut[idx] = (rgba){ r, g, b, a };
                //   old_r = r;
                //   old_g = g;
                //   old_b = b;
                //   old_a = a;
                //   dprintf("lut index=%d\n", idx);
                // }
                dprintf("DIFF24 p=%d %d %d %d %d\n",
                        p,
                        dr,
                        dg,
                        db,
                        ch->diff24.da - 15);
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
                // if (old_r != r || old_g != g || old_b != b || old_a != a) {
                //   int idx = (r ^ g ^ b ^ a) % 64;
                //   lut[idx] = (rgba){ r, g, b, a };
                //   old_r = r;
                //   old_g = g;
                //   old_b = b;
                //   old_a = a;
                //   dprintf("lut index=%d\n", idx);
                // }
                dprintf("COLOR p=%d", p);
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
                npix++;
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
    *pixels++ = r;
    *pixels++ = g;
    *pixels++ = b;
    *pixels++ = a;
  }
  //   dprintf("npix=%d\n", npix);
  return ret;
}
