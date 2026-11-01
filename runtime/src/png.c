/* Minimal PNG writer (RGB565 pixels -> RGB888 PNG) using miniz's compressor. */
#include "grdians/board.h"
#include "miniz.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static int write_chunk(FILE *f, const char type[4], const uint8_t *data, uint32_t length)
{
    uint8_t header[8];
    put_be32(header, length);
    memcpy(header + 4, type, 4);
    if (fwrite(header, 1, 8, f) != 8) return 0;
    if (length && fwrite(data, 1, length, f) != length) return 0;
    uint32_t crc = (uint32_t)mz_crc32(MZ_CRC32_INIT, header + 4, 4);
    if (length) crc = (uint32_t)mz_crc32(crc, data, length);
    uint8_t trailer[4];
    put_be32(trailer, crc);
    return fwrite(trailer, 1, 4, f) == 4;
}

int gr_board_save_png(const char *path, const uint16_t *pixels, int width, int height)
{
    size_t stride = (size_t)width * 3 + 1;
    size_t raw_size = stride * (size_t)height;
    uint8_t *raw = (uint8_t *)malloc(raw_size);
    if (!raw) return 0;
    for (int y = 0; y < height; y++) {
        uint8_t *row = raw + (size_t)y * stride;
        row[0] = 0; /* filter: none */
        for (int x = 0; x < width; x++) {
            uint16_t p = pixels[(size_t)y * (size_t)width + (size_t)x];
            /* palette_device::xRGB_555: bits 14-10 red, 9-5 green, 4-0 blue */
            unsigned r = (p >> 10) & 0x1F, g = (p >> 5) & 0x1F, b = p & 0x1F;
            row[1 + x * 3 + 0] = (uint8_t)((r << 3) | (r >> 2));
            row[1 + x * 3 + 1] = (uint8_t)((g << 3) | (g >> 2));
            row[1 + x * 3 + 2] = (uint8_t)((b << 3) | (b >> 2));
        }
    }

    mz_ulong compressed_size = mz_compressBound((mz_ulong)raw_size);
    uint8_t *compressed = (uint8_t *)malloc(compressed_size);
    int ok = compressed && mz_compress(compressed, &compressed_size, raw, (mz_ulong)raw_size) == MZ_OK;
    free(raw);
    if (!ok) { free(compressed); return 0; }

    FILE *f = fopen(path, "wb");
    if (!f) { free(compressed); return 0; }
    static const uint8_t signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    ok = fwrite(signature, 1, 8, f) == 8;
    uint8_t ihdr[13];
    put_be32(ihdr + 0, (uint32_t)width);
    put_be32(ihdr + 4, (uint32_t)height);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    if (ok) ok = write_chunk(f, "IHDR", ihdr, 13);
    if (ok) ok = write_chunk(f, "IDAT", compressed, (uint32_t)compressed_size);
    if (ok) ok = write_chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(compressed);
    return ok;
}
