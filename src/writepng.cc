// Copyright (c) 2014 Emil Mikulic <emikulic@gmail.com>
// Write out a 24-bit uncompressed PNG file.
//
// Please run the output through a PNG optimizer before storing or
// transmitting it!
//
// References:
//   http://en.wikipedia.org/wiki/Portable_Network_Graphics
//   http://www.w3.org/TR/PNG/

#include "writepng.h"

#include <arpa/inet.h>  // htonl()
#include <err.h>
#include <cassert>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "image.h"

namespace {

constexpr uint16_t kUInt16Max = 0xFFFF;

// This implements a CRC-32 (not CRC-32C)
// Polynomial = 11101101101110001000001100100000b
// Little-endian: 0xED B8 83 20 (CRC-32 / ANSI X3.66, ITU-T V.42)
// Bit-reversed:  0x04 C1 1D B7
// Intel's CRC32 opcode uses 0x11 ED C6 F4 1 (CRC-32C)
//
// {{{ Start of code taken from www.w3.org/TR/PNG/#D-CRCAppendix
// Table of CRCs of all 8-bit messages.
uint32_t crc_table[256];

int crc_table_computed = 0;

// Make the table for a fast CRC.
void make_crc_table(void) {
  unsigned long c;
  int n, k;

  for (n = 0; n < 256; n++) {
    c = (unsigned long)(n);
    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}

static uint32_t update_crc(uint32_t crc, const uint8_t* buf, int len) {
  uint32_t c = crc;
  if (!crc_table_computed) {
    make_crc_table();
  }
  for (int n = 0; n < len; ++n) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}
// }}} End code taken from www.w3.org/TR/PNG/#D-CRCAppendix

void xwrite(const void* ptr, size_t len, FILE* fp, uint32_t* crc) {
  size_t ret = fwrite(ptr, 1, len, fp);
  if (ret != len) {
    err(1, "fwrite() returned %zd, expected %zd", ret, len);
  }
  if (crc) {
    *crc = update_crc(*crc, (const uint8_t*)ptr, len);
  }
}

class PNGChunk {
 public:
  PNGChunk(const char* chunk_type) : chunk_type_(chunk_type) {}

  void add(std::string s) { data_ << s; }

  void add32_be(uint32_t i) {
    // Big endian. (used by png and zlib format)
    uint32_t u = htonl(i);
    data_ << std::string((const char*)(&u), 4);
  }

  void add16_le(uint16_t i) {
    // Little endian. (used by deflate format)
    data_ << std::string((const char*)(&i), 2);
  }

  void add8(uint8_t i) { data_ << std::string((const char*)(&i), 1); }

  void write_to_file(FILE* fp) {
    std::string s = data_.str();
    {
      uint32_t len = htonl(s.length());
      xwrite(&len, sizeof(len), fp, NULL);
    }
    uint32_t crc = ~0;
    xwrite(chunk_type_, 4, fp, &crc);
    xwrite(s.data(), s.length(), fp, &crc);
    crc = htonl(crc ^ (~0));
    xwrite(&crc, sizeof(crc), fp, NULL);
  }

 private:
  const char* chunk_type_;
  std::stringstream data_;
};

constexpr uint32_t initial_adler = 1;

uint32_t update_adler32(uint32_t adler, const uint8_t* buf, int len) {
  uint16_t s1 = adler & 0xffff;
  uint16_t s2 = (adler >> 16) & 0xffff;

  for (int i = 0; i < len; ++i) {
    s1 = (s1 + buf[i]) % 65521;
    s2 = (s2 + s1) % 65521;
  }
  return (s2 << 16) | s1;
}

// This isn't deflate, it just wraps uncompressed data in the deflate block
// format.
class Deflate {
 public:
  Deflate() : adler_(initial_adler) {}

  void add8(uint8_t i) {
    data_ << std::string((const char*)(&i), 1);
    adler_ = update_adler32(adler_, &i, 1);
  }

  void write_to_chunk(PNGChunk* chunk) {
    // CMF
    unsigned int cm = 8;     // Deflate, mandated by PNG.
    unsigned int cinfo = 7;  // 32K window size, max allowed by PNG.
    uint8_t cmf = cm | (cinfo << 4);

    // FLG
    unsigned int fcheck;
    unsigned int fdict = 0;   // No dict present.
    unsigned int flevel = 0;  // Fastest algorithm.
    uint8_t flg = (fdict << 5) | (flevel << 6);
    // choose fcheck so that (CMF*256 + FLG) is a multiple of 31.
    fcheck = 31 - ((cmf * 256 + flg) % 31);
    flg |= (fcheck & 31);
    assert((cmf * 256 + flg) % 31 == 0);
    chunk->add8(cmf);
    chunk->add8(flg);

    // Produce deflate blocks of uncompressed data.
    const size_t max_len = 65535;  // Specified as maximum.
    std::string s = data_.str();
    size_t pos = 0;
    while (pos < s.length()) {
      int bfinal = 0;
      int btype = 0;  // No compression.
      if (pos + max_len >= s.length()) {
        bfinal = 1;
      }
      uint8_t header = bfinal | (btype << 2);  // Little endian.
      chunk->add8(header);

      size_t len = s.length() - pos;
      if (len > max_len) {
        len = max_len;
      }
      assert(len <= kUInt16Max);
      uint16_t len1 = uint16_t(len);
      uint16_t nlen = ~len1;
      chunk->add16_le(len1);
      chunk->add16_le(nlen);
      chunk->add(std::string(s.data() + pos, len));
      pos += len;
    }
    chunk->add32_be(adler_);
  }

 private:
  uint32_t adler_;
  std::stringstream data_;
};

void write_png(const char* filename, uint32_t width, uint32_t height,
               const uint8_t* image, uint32_t gamma_times_100000) {
  FILE* fp = fopen(filename, "wb");
  if (fp == NULL) {
    err(1, "fopen(\"%s\") failed", filename);
  }

  const uint8_t magic[] = {
      0x89,  // Has the high bit set to detect transmission systems that do not
             // support 8 bit data and to reduce the chance that a text file is
             // mistakenly interpreted as a PNG, or vice versa
      'P', 'N', 'G', 0x0d, 0x0a,  // DOS line ending
      0x1a,  // end-of-file: stops display of the file under DOS when the
             // command "type" has been used.
      0x0a,  // Unix-style line ending (LF) to detect Unix-DOS line ending
             // conversion.
  };
  xwrite(magic, sizeof(magic), fp, NULL);

  // A chunk consists of four parts: length (4 bytes, network order), chunk
  // type/name (4 bytes), chunk data and CRC (network-byte-order CRC-32 computed
  // over the chunk type and chunk data, but not the length.

  // Chunks we want: IHDR, gAMA, IDAT, IEND

  // IHDR
  {
    int bit_depth = 8;
    int color_type = 2;          // Truecolor
    int compression_method = 0;  // The only standard one.
    int filter_method = 0;       // The only standard one.
    int interlace_method = 0;    // No interlace.
    PNGChunk ihdr("IHDR");
    ihdr.add32_be(width);
    ihdr.add32_be(height);
    ihdr.add8(bit_depth);
    ihdr.add8(color_type);
    ihdr.add8(compression_method);
    ihdr.add8(filter_method);
    ihdr.add8(interlace_method);
    ihdr.write_to_file(fp);
  }

  {
    PNGChunk gama("gAMA");
    gama.add32_be(gamma_times_100000);
    gama.write_to_file(fp);
  }

  {
    PNGChunk idat("IDAT");
    Deflate deflate;
    // Input is 32 bits but we need 24 bits.
    size_t pos = 0;
    for (int y = 0; y < height; ++y) {
      // Emit filter type at start of scanline.
      deflate.add8(0);
      for (int x = 0; x < width; ++x) {
        for (int c = 0; c < 3; ++c) {
          deflate.add8(image[pos++]);
        }
      }
    }
    deflate.write_to_chunk(&idat);
    idat.write_to_file(fp);
  }

  {
    PNGChunk iend("IEND");
    iend.write_to_file(fp);
  }

  fclose(fp);
}

}  // namespace

void Writepng(const Image& img, const char* filename) {
  const int w = img.width_;
  const int h = img.height_;
  std::unique_ptr<uint8_t[]> data(new uint8_t[w * h * 3]);

  const double* src = img.data_.get();
  uint8_t* dst = data.get();
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      dst[0] = Image::from_float(src[0]);
      dst[1] = Image::from_float(src[1]);
      dst[2] = Image::from_float(src[2]);
      dst += 3;
      src += 3;
    }
  write_png(filename, w, h, data.get(), 45455);
}
