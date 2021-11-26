#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
  typedef uint8_t u8;
  typedef uint16_t u16;
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
      u8 db : 4;
      u8 dg : 4;
      u8 dr : 5;
      u8 tag : 3;
    } diff16;
    struct
    {
      u16 da : 5;
      u16 db : 5;
      u16 dg : 5;
      u16 dr : 5;
      u16 tag : 4;
    } diff24;
    struct
    {
      u8 has_a : 1;
      u8 has_b : 1;
      u8 has_g : 1;
      u8 has_r : 1;
      u8 tag : 4;
      u8 a;
      u8 b;
      u8 g;
      u8 r;
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
  qoi_t* qoi = malloc(size);
  rewind(in);
  fread(qoi, size, 1, in);
  fclose(in);
  if (0 != memcmp(qoi->magic, "qoif", 4)) {
    printf("Invalid qoif magic\n");
    exit(1);
  }
  //   printf("QOIF: %dx%d (%d)\n", qoi->width, qoi->height, (int)qoi->size);
  //   printf("chunk_t=%zd\n", sizeof(chunk_t));
  //   chunk_t ch0;
  //   printf("color=%zd\n", sizeof(ch0.color));
  //   printf("diff24=%zd\n", sizeof(ch0.diff24));
  //   printf("diff16=%zd\n", sizeof(ch0.diff16));
  //   printf("diff8=%zd\n", sizeof(ch0.diff8));
  //   printf("run16=%zd\n", sizeof(ch0.run16));
  //   printf("run8=%zd\n", sizeof(ch0.run8));
  //   printf("index=%zd\n", sizeof(ch0.index));
  int npix = 0;
  for (int i = 0; i < qoi->size;) {
    if (npix >= qoi->width * qoi->height)
      break;
    int p = 12 + i;
    chunk_t* ch = (chunk_t*)&qoi->data[i];
    switch (ch->tag2) {
      case 0x0:
        printf("INDEX p=%d\n", p);
        // printf("INDEX %d\n", i);
        // printf("INDEX\n");
        i += sizeof(ch->index);
        npix++;
        break;
      case 0x1:
        // printf("RUN8/16\n");
        switch (ch->tag3) {
          case 0x2:
            // printf("RUN8 p=%d\n", p);
            printf("RUN8 p=%d %d\n", p, ch->run8.run);
            i += sizeof(ch->run8);
            // printf("RUN8 %d\n", i - _i);
            // printf("RUN8\n");
            npix += ch->run8.run + 1;
            break;
          case 0x3:
            printf("RUN16 p=%d\n", p);
            i += sizeof(ch->run16);
            // printf("RUN16 %d\n", i - _i);
            // printf("RUN16\n");
            npix += ch->run16.run + 33;
            break;
          default:
            printf("unhandled tag3 %d\n", ch->tag3);
            exit(1);
        }
        break;
      case 0x2:
        printf("DIFF8 p=%d\n", p);
        // printf("DIFF8\n");
        i += sizeof(ch->diff8);
        npix++;
        break;
      case 0x3:
        // printf("DIFF16/24/COLOR\n");
        switch (ch->tag3) {
          case 0x6:
            printf("DIFF16 p=%d\n", p);
            // printf("DIFF16\n");
            i += sizeof(ch->diff16);
            npix++;
            break;
          default:
            switch (ch->tag4) {
              case 0xe:
                printf("DIFF24 p=%d\n", p);
                // printf("DIFF24\n");
                // i += sizeof(ch->diff24);
                i += 3;
                npix++;
                break;
              default: {
                // printf("COLOR p=%d\n", p);
                printf("COLOR p=%d", p);
                i++;
                if (ch->color.has_r) {
                  printf(" r=%d", qoi->data[i]);
                  i++;
                }
                if (ch->color.has_g) {
                  printf(" g=%d", qoi->data[i]);
                  i++;
                }
                if (ch->color.has_b) {
                  printf(" b=%d", qoi->data[i]);
                  i++;
                }
                if (ch->color.has_a) {
                  printf(" a=%d", qoi->data[i]);
                  i++;
                }
                printf("\n");
                // printf("COLOR p=%d %02x\n", p, qoi->data[i]);
                // printf("COLOR %d\n", i - _i);
                // printf("COLOR\n");
                npix++;
                break;
              }
            }
            break;
        }
        break;
      default:
        printf("unhandled tag2 %d\n", ch->tag2);
        exit(1);
    }
  }
  //   printf("npix=%d\n", npix);
  return 0;
}
