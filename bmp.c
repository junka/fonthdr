/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018-2019 wan.junjie
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack (push, 2)

struct bmp_header {
    //bmp file header
    uint16_t file_type;
    uint32_t file_size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;

    //bmp info head
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

unsigned char* alloc_bmp_with_head(uint32_t w, uint32_t h)
{
    struct bmp_header *bmp_p;
    long file_length = w * h * 4;
    unsigned char* d_ptr = malloc(file_length + 54);

    bmp_p = (struct bmp_header *)d_ptr;
    bmp_p->file_type = 0x4D42;
    bmp_p->file_size = 54 + w * h * 4;
    bmp_p->reserved1 = 0x0;
    bmp_p->reserved2 = 0x0;
    bmp_p->offset = 0x36;

    //bmp info head
    bmp_p->biSize = 0x28;
    bmp_p->biWidth = w;
    bmp_p->biHeight = -h;
    bmp_p->biPlanes = 0x01;
    bmp_p->biBitCount = 32;
    bmp_p->biCompression = 0;
    bmp_p->biSizeImage = w*h*4;
    bmp_p->biXPelsPerMeter = 0x60;
    bmp_p->biYPelsPerMeter = 0x60;
    bmp_p->biClrUsed = 2;
    bmp_p->biClrImportant = 0;

    return d_ptr;
}

//using ARGB
static unsigned int front_color = 0xFFFFFFFF;
static unsigned int back_color = 0xFFFFFFFF;

void set_back_color(unsigned int color)
{
	back_color = color;
}

void set_front_color(unsigned int color)
{
    front_color = color;
}

#define COLOR_R ((front_color>>16)&0xFF)
#define COLOR_G ((front_color>>8)&0xFF)
#define COLOR_B (front_color&0xFF)


int create_bmp(uint32_t w, uint32_t h, uint8_t *buff)
{
    FILE *fd;
    long file_length = w*h*4+54;
    unsigned char *file_p_tmp = NULL;
    unsigned char byte_copy = 0;
    const char *file_name = "font.bmp";

    unsigned char *file_p = alloc_bmp_with_head(w,h);
    file_p_tmp = file_p;
    file_p_tmp += 54;
    for (uint32_t i = 54; i < w * h * 4; i++) {
        byte_copy = *(buff+(i-54)/4);
        if((i-54)%4==0)
            *file_p_tmp = byte_copy;
        else if((i-54)%4==1)
            *file_p_tmp = ((byte_copy)*COLOR_R>>8);
        else if((i-54)%4==2)
            *file_p_tmp = ((byte_copy)*COLOR_G>>8);
        else
            *file_p_tmp = ((byte_copy)*COLOR_B>>8);

        file_p_tmp++;
    }
    fd = fopen(file_name, "w");
    fwrite(file_p, file_length, 1,fd);
    free(file_p);
    fclose(fd);
    return (0);
}

