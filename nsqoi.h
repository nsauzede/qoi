
#ifndef QOI_NO_STDIO
int nsqoi_write(const char *filename, const void *data, const qoi_desc *desc);
int nsqoi_pamwrite(const char *filename, const void *data, const qoi_desc *desc);
void *nsqoi_read(const char *filename, qoi_desc *desc, int channels);
void *nsqoi_decode(const void *data, int size, qoi_desc *desc, int channels);
void *nsqoi_encode(const void *data, const qoi_desc *desc, int *out_len);
#endif

#ifdef QOI_IMPLEMENTATION

#ifndef QOI_NO_STDIO

typedef struct nsqoi_header {
char magic[4]; // magic bytes "qoif"
uint32_t width; // image width in pixels (BE)
uint32_t height; // image height in pixels (BE)
uint8_t channels; // 3 = RGB, 4 = RGBA
uint8_t colorspace; // 0 = sRGB with linear alpha
// 1 = all channels linear
char data[0];
}nsqoi_t;

int nsqoi_pamwrite(const char *filename, const void *data, const qoi_desc *desc) {
	int ret = 0;
	FILE *out = fopen(filename, "wt");
	fprintf(out, "P7\n");
	fprintf(out, "WIDTH %d\nHEIGHT %d\n", desc->width, desc->height);
	fprintf(out, "DEPTH %d\n", desc->channels);
	fprintf(out, "MAXVAL 255\n");
	fprintf(out, "TUPLTYPE %s\n", desc->channels == 3 ? "RGB" : "RGB_ALPHA");
	fprintf(out, "ENDHDR\n");
	ret = fwrite(data, 1, desc->width * desc->height * desc->channels, out);
	fclose(out);
	return ret;
}

int nsqoi_write(const char *filename, const void *data, const qoi_desc *desc) {
	return 1;
}

void *nsqoi_read(const char *filename, qoi_desc *desc, int channels) {
	FILE *in = fopen(filename, "rb");
	fseek(in, 0, SEEK_END);
	long len = ftell(in);
	rewind(in);
	void *data = malloc(len);
	size_t ret = fread(data,1,len,in);
	fclose(in);
	if (ret != len) {
		printf("Couldn't read QOIF\n");
		free(data);
		return 0;
	}
	void *pixels = nsqoi_decode(data, len, desc, channels);
	free(data);
	return pixels;
}

#endif

#if 0
#include <arpa/inet.h>
#else
long ntohl(long n_) {
	union {
		long l;
		struct {
			unsigned char a, b, c, d;
		};
	} h, n;
	n.l = n_;
	h.a = n.d;
	h.b = n.c;
	h.c = n.b;
	h.d = n.a;
	return h.l;
}
#endif

typedef uint8_t u8;
typedef uint16_t u16;

typedef union {
  u8 data[5];
  struct {
    u8 : 6;
    u8 tag2 : 2;
  };
  struct {
    u8 tag8;
  };

  struct {
    u8 idx : 6;
    u8 tag : 2;		// 00
  } index;

  struct {
    u8 db : 2;
    u8 dg : 2;
    u8 dr : 2;
    u8 tag : 2;		// 01
  } diff;

  struct {
    u16 dbdg : 4;
    u16 drdg : 4;
    u16 dg : 6;
    u16 tag : 2;	// 10
  } luma;

  struct {
    u8 len : 6;		// neither 111110 nor 111111
    u8 tag : 2;		// 11
  } run;

  struct {
    u8 tag;		// 11111110
    u8 r, g, b;
  } rgb;

  struct {
    u8 tag;		// 11111111
    u8 r, g, b, a;
  } rgba;
} chunk_t;

#define dprintf(...) do{printf("%s:%d: ", __func__, __LINE__);printf(__VA_ARGS__);}while(0)
void *nsqoi_decode(const void *data, int size, qoi_desc *desc, int channels) {
	if (size < sizeof(nsqoi_t)) {
		printf("Too small for QOIF\n");
		return 0;
	}
	nsqoi_t *qoi = (nsqoi_t *)data;
	if (0 != memcmp(qoi->magic, "qoif", 4)) {
		printf("Not QOIF\n");
		return 0;
	}
	desc->width = ntohl(qoi->width);
	desc->height = ntohl(qoi->height);
	desc->channels = qoi->channels;
	desc->colorspace = qoi->colorspace;
	int len = desc->channels * desc->width * desc->height;
//	void *pixels = calloc(1, len);
	typedef struct {
		u8 r, g, b, a;
	} rgba_t;
	typedef union {
		u8 *byte;
		rgba_t *rgba;
	} pix_t;
	rgba_t *pixels = malloc(len);
	void *ret = pixels;
//	memset(pixels,255,len);
	rgba_t seen[64];
	memset(seen, 0, sizeof(seen));
	rgba_t prev_ = {0,0,0,255}, *prev = &prev_;
	for (int i = 0; i < size;) {
		chunk_t *ch = (chunk_t *)&qoi->data[i];
		switch (ch->tag2) {
		case 0x0:{
			dprintf("INDEX %d\n", ch->index.idx);
			i += 1;
			
			break;
		}
		case 0x01:
			dprintf("DIFF\n");
			i += 1;
			break;
		case 0x02:
			dprintf("LUMA\n");
			i += 2;
			break;
		default:
			switch (ch->tag8) {
			case 0xfe:
				dprintf("RGB\n");
				i += 4;
				break;
			case 0xff:
				dprintf("RGBA\n");
				i += 5;
				break;
			default:
				dprintf("RUN %d\n", ch->run.len + 1);
				i += 1;
				break;
			}
			break;
		}
	}
	return ret;
}

#endif
