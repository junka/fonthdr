#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push,2)
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
#pragma (pop)


unsigned char* alloc_bmp_with_head(uint32_t w,uint32_t h)
{
    struct bmp_header *bmp_p;
    long file_length = w*h*4;
    unsigned char*d_ptr = malloc(file_length+54);

    bmp_p = (struct bmp_header *)d_ptr;
    bmp_p-> file_type = 0x4D42;
    bmp_p-> file_size = 54+w*h*4;
    bmp_p-> reserved1 = 0x0;
    bmp_p-> reserved2 = 0x0;
    bmp_p-> offset = 0x36;

    //bmp info head
    bmp_p-> biSize = 0x28;
    bmp_p-> biWidth = w;
    bmp_p-> biHeight = -h;
    bmp_p-> biPlanes = 0x01;
    bmp_p-> biBitCount = 32;
    bmp_p-> biCompression = 0;
    bmp_p-> biSizeImage = w*h*4;
    bmp_p-> biXPelsPerMeter = 0x60;
    bmp_p-> biYPelsPerMeter = 0x60;
    bmp_p-> biClrUsed = 2;
    bmp_p-> biClrImportant = 0;
    
    return d_ptr;
}

void destroy_bmp(unsigned char* ptr)
{
    free(ptr);
}

int create_bmp(uint32_t w,uint32_t h,uint8_t *buff)
{
    FILE *fd;
    static char *file_name =NULL;
    long file_length =w*h*4+54;
    unsigned char *file_p = NULL;
    unsigned char *file_p_tmp = NULL;
    unsigned char *byte_copy_p = NULL;
    unsigned char byte_copy = 0;
    int i = 0;
    file_name = "font.bmp";
    
    file_p = alloc_bmp_with_head(w,h); ;
    file_p_tmp = file_p;
    file_p_tmp += 54;
    for(i=54;i<w*h*4;i++)
    {
        *file_p_tmp= *(buff+(i-54)/4);
        file_p_tmp++;
    }
    fd = fopen(file_name, "w");
    fwrite(file_p, file_length, 1,fd);
    destroy_bmp(file_p);
    fclose(fd);
    return (0);
}

