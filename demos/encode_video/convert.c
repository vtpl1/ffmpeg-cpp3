#include <stdint.h>
#include <stdlib.h>


// source : https://pastebin.com/mDcwqJV3

enum ImageType {
    ImageType_RGB24 = 1,
    ImageType_GRAY8 = 2,
    ImageType_JPEG  = 3
};

struct Image {
    Image() : size_x(0), size_y(0), type(ImageType_GRAY8)
    {}

    int size_x;
    int size_y;

    int type; // @see ImageType

    std::vector<uint8x> pixels;
};

static inline
void saturate(int& value, int min_val, int max_val)
{
  if (value < min_val) value = min_val;
  if (value > max_val) value = max_val;
}

void convert_YUYV_to_RGB24(int size_x, int size_y, const uint8x* YUYV_ptr, Image& out)
{
  const int K1 = int(1.402f * (1 << 16));
  const int K2 = int(0.714f * (1 << 16));
  const int K3 = int(0.334f * (1 << 16));
  const int K4 = int(1.772f * (1 << 16));

  // convert to RGB24
  out.size_x = size_x;
  out.size_y = size_y;
  out.type = ImageType_RGB24;

  out.pixels.resize(size_x * size_y * 3); // 3 bytes per RGB24 pixel

  typedef uint8x T;
  T* out_ptr = &out.pixels[0];
  const int pitch = size_x * 2; // 2 bytes per one YU-YV pixel

  for (int y=0; y<size_y; y++) {
    const uint8x* src = YUYV_ptr + pitch * y;
    for (int x=0; x<size_x*2; x+=4) { // Y1 U Y2 V
      uint8x Y1 = src[x + 0];
      uint8x U  = src[x + 1];
      uint8x Y2 = src[x + 2];
      uint8x V  = src[x + 3];

      int8x uf = U - 128;
      int8x vf = V - 128;

      int R = Y1 + (K1*vf >> 16);
      int G = Y1 - (K2*vf >> 16) - (K3*uf >> 16);
      int B = Y1 + (K4*uf >> 16);

      saturate(R, 0, 255);
      saturate(G, 0, 255);
      saturate(B, 0, 255);

      *out_ptr++ = T(R);
      *out_ptr++ = T(G);
      *out_ptr++ = T(B);

      R = Y2 + (K1*vf >> 16);
      G = Y2 - (K2*vf >> 16) - (K3*uf >> 16);
      B = Y2 + (K4*uf >> 16);

      saturate(R, 0, 255);
      saturate(G, 0, 255);
      saturate(B, 0, 255);

      *out_ptr++ = T(R);
      *out_ptr++ = T(G);
      *out_ptr++ = T(B);
    }

  }
}


static uint8_t CLAMP(int16_t value) { return value<0 ? 0 : (value>255 ? 255 : value); }

static uint8_t* yuv420p_to_rgb2(const uint8_t* y, const uint8_t* u, const uint8_t* v, const int width, const int height)
{
    const int size = width * height;
    uint8_t* rgb = (uint8_t*)calloc((size * 3), sizeof(uint8_t));

    int uv_index = 0, pass = 0;
    int b,g,r;
    uint8_t* ptr = rgb;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int yy = y[(j * width) + i];
            int uu = u[((j / 2) * (width / 2)) + (i / 2)];
            int vv = v[((j / 2) * (width / 2)) + (i / 2)];

            r = 1.164 * (yy - 16) + 1.596 * (vv - 128);
            g = 1.164 * (yy - 16) - 0.813 * (vv - 128) - 0.391 * (uu - 128);
            b = 1.164 * (yy - 16) + 2.018 * (uu - 128);
            *ptr++ = CLAMP(r);
            *ptr++ = CLAMP(g);
            *ptr++ = CLAMP(b);
        }
    }

    return rgb;
}