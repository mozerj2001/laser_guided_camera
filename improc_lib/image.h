#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cmath>

#define STEG_HEADER_SIZE sizeof(uint32_t)*8

enum ImageType{
    PNG, JPG, BMP, TGA
};


struct Image{
    uint8_t* data = NULL;
    size_t size = 0;
    int width;
    int height;
    int channels;           // how many color values per pixel

    // constructors
    Image(const char* filename);
    Image(int width, int height, int channels);
    Image(const Image& img);
    ~Image();

    // file management
    bool read(const char* filename);
    bool write(const char* filename);
    ImageType getFileType(const char* filename);

    // filters
    Image& greyScale_AVG();
    Image& greyScale_LUM();
    Image& colorMask(float r, float g, float b);

    // message encoding into an image
    Image& encodeMessage(const char* message);
    Image& decodeMessage(char* buffer, size_t* message_length);

    // diffmap
    Image& diffmap(Image& img);
    Image& diffmapScale(Image& img, uint8_t scale=0);

};
