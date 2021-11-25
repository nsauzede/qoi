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
  } chunk_t;
  typedef struct
  {
    u8 idx : 6;
    u8 tag : 2;
  } index;
  typedef struct
  {
    u8 run : 5;
    u8 tag : 3;
  } run8;
  typedef struct
  {
    u16 run : 13;
    u8 tag : 3;
  } run16;
  typedef struct
  {
    u8 db : 2;
    u8 dg : 2;
    u8 dr : 2;
    u8 tag : 2;
  } diff8;
  typedef struct
  {
    u8 db : 4;
    u8 dg : 4;
    u8 dr : 5;
    u8 tag : 3;
  } diff16;
  typedef struct
  {
    u8 da : 5;
    u8 db : 5;
    u8 dg : 5;
    u8 dr : 5;
    u8 tag : 4;
  } diff24;
  typedef struct
  {
    u8 a;
    u8 b;
    u8 g;
    u8 r;
    u8 has_a : 1;
    u8 has_b : 1;
    u8 has_g : 1;
    u8 has_r : 1;
    u8 tag : 4;
  } color;
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
  //   printf("color=%zd\n", sizeof(color));
  for (int i = 0; i < size;) {
    int _i = i;
    chunk_t* ch = (chunk_t*)&qoi->data[i];
    switch (ch->tag2) {
      case 0x0:
        // printf("INDEX %d\n", i);
        printf("INDEX\n");
        i += sizeof(index);
        break;
      case 0x1:
        // printf("RUN8/16\n");
        switch (ch->tag3) {
          case 0x2:
            i += sizeof(run8);
            // printf("RUN8 %d\n", i - _i);
            printf("RUN8\n");
            _i = i;
            break;
          case 0x3:
            i += sizeof(run16);
            // printf("RUN16 %d\n", i - _i);
            printf("RUN16\n");
            _i = i;
            break;
          default:
            printf("unhandled tag3 %d\n", ch->tag3);
            exit(1);
        }
        break;
      case 0x2:
        printf("DIFF8\n");
        i += sizeof(diff8);
        break;
      case 0x3:
        // printf("DIFF16/24/COLOR\n");
        switch (ch->tag3) {
          case 0x6:
            printf("DIFF16\n");
            i += sizeof(diff16);
            break;
          default:
            switch (ch->tag4) {
              case 0xe:
                printf("DIFF24\n");
                i += sizeof(diff24);
                break;
              default: {
                color* c = (color*)ch;
                i++;
                if (c->has_r)
                  i++;
                if (c->has_g)
                  i++;
                if (c->has_b)
                  i++;
                if (c->has_a)
                  i++;
                // printf("COLOR %d\n", i - _i);
                printf("COLOR\n");
                _i = _i;
                _i = i;
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
  return 0;
}
