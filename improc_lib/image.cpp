#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define BYTE_BOUND(value) value < 0 ? 0 : (value > 255 ? 255 : value)

#include <cmath>
#include "stb_image.h"
#include "stb_image_write.h"
#include "image.h"

    // constructors
    Image::Image(const char* filename){
        if(read(filename)){
            printf("%s has been read.\n", filename);
            size = width*height*channels;
        }
        else{
            printf("Failed to read %s.\n", filename);
        }
    };

    Image::Image(int width, int height, int channels) : width(width), height(height), channels(channels){
        size = width*height*channels;
        data = new uint8_t[size];
    };

    Image::Image(const Image& img) : Image(img.width, img.height, img.channels){
        memcpy(data, img.data, size);
        
    };

    Image::~Image(){
        stbi_image_free(data);
    };


    // file management
    bool Image::read(const char* filename){
        data = stbi_load(filename, &width, &height, &channels, 0);

        return data != NULL;
    };

    bool Image::write(const char* filename){
        ImageType type = getFileType(filename);
        int success;
        bool boolSuccess = false;

        switch(type){
            case PNG:
                success = stbi_write_png(filename, width, height, channels, data, width*channels);
                break;
            case JPG:
                success = stbi_write_jpg(filename, width, height, channels, data, 100);
                break;
            case BMP:
                success = stbi_write_bmp(filename, width, height, channels, data);
                break;
            case TGA:
                success = stbi_write_tga(filename, width, height, channels, data);
                break;
        }

        if(boolSuccess = (success != 0)){
            printf("%s was written.\n", filename);
        }
        else{
            printf("Failed to write %s.\n", filename);
        }

        return boolSuccess;
    };

    ImageType Image::getFileType(const char* filename){
        const char* extension = strrchr(filename, '.');

        if(extension != nullptr){
            if(strcmp(extension, ".png") == 0){
                return PNG;
            }
            else if(strcmp(extension, ".jpg") == 0){
                return JPG;
            }
            else if(strcmp(extension, ".bmp") == 0){
                return BMP;
            }
            else if(strcmp(extension, ".tga") == 0){
                return TGA;
            }
        }
        
        return PNG;
    }

    // filters
    Image& Image::greyScale_AVG(){
        //r+g+b/3
        if(channels<3){ 
            printf("Image %p has less than 3 channels, therefore it is assumed to already be greyscaled.", this);
        }
        else{
            for(unsigned int i = 0; i < size; i+=channels){
                int grey = (data[i] + data[i+1] + data[i+2])/3;
                memset(data+i, grey, 3);
            }
        }
        return *this;
    }

    Image& Image::greyScale_LUM(){
        //average weighted for human perception
        if(channels<3){ 
            printf("Image %p has less than 3 channels, therefore it is assumed to already be greyscaled.", this);
        }
        else{
            for(unsigned int i = 0; i < size; i+=channels){
                int grey = (0.2126*data[i] + 0.7152*data[i+1] + 0.0722*data[i+2])/3;
                memset(data+i, grey, 3);
            }
        }

        return *this;
    }

    Image& Image::colorMask(float r, float g, float b){
        if(channels<3){printf("\e[31m[ERROR] Color mask requires at least 3 channels, but this image has only %d. \e[0m\n]", channels);}
        else if(r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1){printf("\e[31m[ERROR] R, G and B can only hold values in the [0;1] interval.\n");}
        else{
            for(unsigned int i = 0; i < size; i+=channels){
                data[i] *= r;
                data[i+1] *= g;
                data[i+2] *= b;
            }
        }

        return *this;
    }

    // message encoding
    Image& Image::encodeMessage(const char* message){
        uint32_t len = strlen(message) * sizeof(char)*8;    // message length in bits

        if(len + STEG_HEADER_SIZE > size){
            printf("\e[31m[ERROR]: This message is too large (%d bits / %zu bits).\e[0m\n", len+STEG_HEADER_SIZE, size);
            return *this;
        }

        for(int i = 0; i < STEG_HEADER_SIZE; i++){
            data[i] &= 0xFE;
            data[i] |= (len >> (STEG_HEADER_SIZE - 1 - i)) & 1UL;
        }

        for(int i = 0; i < len; i++){
            data[i+STEG_HEADER_SIZE] &= 0xFE;
            data[i+STEG_HEADER_SIZE] |= (message[i/8] >> ((len-1-i)%8)) & 1;
        }
        
        return *this;
    }

    Image& Image::decodeMessage(char* buffer, size_t* message_length){
        uint32_t len = 0;

        for(int i = 0; i < STEG_HEADER_SIZE; i++){
            len = (len << 1) | (data[i] & 1);
        }

        *message_length = len / 8;

        for(int i = 0; i < len; i++){
            buffer[i/8] = (buffer[i/8] << 1) | (data[i+STEG_HEADER_SIZE] & 1);
        }


        return *this;
    }


    // diffmap
    Image& Image::diffmap(Image& img){
        int compareWidth = fmin(width, img.width); 
        int compareHeight = fmin(height, img.height); 
        int compareChannels = fmin(channels, img.channels);
    
        // iterate through pixels, set the absolute difference of the pixel values for each pixel
        for(unsigned int i = 0; i < compareWidth; i++){
            for(unsigned int j = 0; j < compareHeight; j++){
                for(unsigned int k = 0; k < compareChannels; k++){
                    data[(i*width+j)*channels+k] = BYTE_BOUND(abs(data[(i*width+j)*channels+k] - data[(i*img.width+j)*img.channels+k]));
                }
            }
        }
        return *this;
    }

    Image& Image::diffmapScale(Image& img, uint8_t scale){
        int compareWidth = fmin(width, img.width); 
        int compareHeight = fmin(height, img.height);
        int compareChannels = fmin(channels, img.channels);
        uint8_t largest = 0;
    
        // iterate through pixels, set the absolute difference of the pixel values for each pixel
        for(unsigned int i = 0; i < compareWidth; i++){
            for(unsigned int j = 0; j < compareHeight; j++){
                for(unsigned int k = 0; k < compareChannels; k++){
                    data[(i*width+j)*channels+k] = BYTE_BOUND(abs(data[(i*width+j)*channels+k] - data[(i*img.width+j)*img.channels+k]));
                    largest = fmax(largest, data[i]);
                }
            }
        }
        scale = 255/fmax(1, fmax(scale, largest));
        for(unsigned int i = 0; i < size; i++){
            data[i] *= scale;
        }
        return *this;
    }
