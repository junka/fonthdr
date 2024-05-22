/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018-2019 wan.junjie
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <iconv.h>

#include "ft2build.h"

#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_WINFONTS_H
#include FT_BITMAP_H

FT_Face pFTFace = NULL; 
FT_Library pFTLib = NULL; 

struct char_gly {
    unsigned int rows;
    unsigned int width;
    int pitch;
};

int count=0;
int height ;

int FT_tool_init(const char* fontame)
{
    if (fontame == NULL)
        return -1;

    FT_Error error = FT_Init_FreeType(&pFTLib);
    if (error)
    {
        printf("There is some error when Init Library");
        return -1;
    }
    error = FT_New_Face(pFTLib,fontame, 0, &pFTFace);

    if (error == FT_Err_Unknown_File_Format)
    {
        printf("file format errot\n");
        return -1;
    }
    else if (error)
    {
        printf("can not open the file\n");
        return -1;
    }

    //printf("face num: %d\n",pFTFace->num_faces);
    //printf("the face has %d glyphs\n", pFTFace->num_glyphs);

    FT_Set_Char_Size(pFTFace,0, 32*32, 400,400);
    height = pFTFace->size->metrics.height/64 ;
    //printf("height %d\n",height);

    return 0;
}

static int FT_convert_code(FILE *fp, unsigned long code, FT_BitmapGlyph *fbmp, int idx) {
    char stt[12];
    FT_Error error = 0;
    FT_Glyph glyph = NULL;
    FT_BitmapGlyph bitmap_glyph;
    FT_Bitmap bitmap;

    FT_UInt index = FT_Get_Char_Index(pFTFace, code);
    // printf("code %lu, index %u\n", code, index);

    FT_Load_Glyph(pFTFace, index, FT_LOAD_DEFAULT | FT_LOAD_COLOR);

    error = FT_Get_Glyph(pFTFace->glyph, &glyph);
    if (error) {
        return -1;
    }

    if (pFTFace->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        /* this mode pitch equals width */
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
    }

    bitmap_glyph = (FT_BitmapGlyph)glyph;
    bitmap = bitmap_glyph->bitmap;

    fbmp[idx] = bitmap_glyph;

    fputs("\t{\n", fp);
    for (uint32_t m = 0; m < bitmap.rows; ++m) {
        fputs("\t\t", fp);
        for (uint32_t n = 0; n < bitmap.width; ++n) {
        sprintf(stt, "0x%02X,", bitmap.buffer[m * bitmap.width + n]);
        fputs(stt, fp);
        }
        fputs("\n", fp);
    }
    fputs("\t},\n", fp);
    return 0;
}


static uint32_t *utf8_to_unicode(const unsigned char *utf8_str, int *out_len) {
    size_t utf8_len = strlen((const char *)utf8_str);
    int unicode_len = 0;
    uint32_t *unicode_arr = malloc(utf8_len * sizeof(uint32_t));

    for (size_t i = 0; i < utf8_len; i++) {
        uint32_t code_point = 0;
        int bytes = 0;

        // Determine the number of bytes for the current UTF-8 character
        if ((utf8_str[i] & 0x80) == 0x00) {
            bytes = 1;
            code_point = (uint32_t)(utf8_str[i] & 0x7F);
        } else if ((utf8_str[i] & 0xE0) == 0xC0) {
            bytes = 2;
            code_point = (uint32_t)(utf8_str[i] & 0x1F);
        } else if ((utf8_str[i] & 0xF0) == 0xE0) {
            bytes = 3;
            code_point = (uint32_t)(utf8_str[i] & 0x0F);
        } else if ((utf8_str[i] & 0xF8) == 0xF0) {
            bytes = 4;
            code_point = (uint32_t)(utf8_str[i] & 0x07);
        } else {
            // Invalid UTF-8 sequence, skip this character
            continue;
        }

        // Decode the rest of the UTF-8 sequence
        for (int j = 1; j < bytes; j++) {
            code_point <<= 6;
            code_point |= (uint32_t)(utf8_str[i + j] & 0x3F);
        }

        // Add the code point to the output array
        unicode_arr[unicode_len++] = code_point;
        i += bytes - 1;
    }

    *out_len = unicode_len;
    return unicode_arr;
}

static int FT_convert(FILE *fp, const unsigned char *str, FT_BitmapGlyph *fbmp) {
    if (fp == NULL || str == NULL)
        return -1;

    char stt[32];
    int i = 0;
    int count = 0;
    uint32_t *codes = utf8_to_unicode(str, &count);

    fputs("#ifndef _FTSRC_H_\n", fp);
    fputs("#define _FTSRC_H_\n\n", fp);
    fputs("#ifdef __cplusplus\n", fp);
    fputs("extern \"C\" {\n", fp);
    fputs("#endif\n\n", fp);
    sprintf(stt, "#define NUM_OF_CHAR (%d)\n\n", count);
    fputs(stt, fp);
    fputs("const unsigned int char_height = ", fp);
    sprintf(stt, "%u;\n\n", height);
    fputs(stt, fp);
    fputs("const unsigned char char_code[NUM_OF_CHAR][] = {\n", fp);
    for (i = 0; i < count; i++) {
        FT_convert_code(fp, codes[i], fbmp, i);
    }
    fputs("};\n\n", fp);
    fputs("/*", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %c,", str[i]);
        fputs(stt, fp);
    }
    fputs("*/\n\n", fp);

    fputs("const unsigned int char_rows[NUM_OF_CHAR] = {\n", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %u,", fbmp[i]->bitmap.rows);
        fputs(stt, fp);
    }
    fputs("\n};\n\n", fp);

    fputs("const unsigned int char_width[NUM_OF_CHAR] = {\n", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %u,", fbmp[i]->bitmap.width);
        fputs(stt, fp);
    }
    fputs("\n};\n\n", fp);

    fputs("const int char_pitch[NUM_OF_CHAR] = {\n", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %u,", fbmp[i]->bitmap.pitch);
        fputs(stt, fp);
    }
    fputs("\n};\n\n", fp);

    fputs("const int char_top[NUM_OF_CHAR] = {\n", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %u,", fbmp[i]->top);
        fputs(stt, fp);
    }
    fputs("\n};\n\n", fp);

    fputs("const int char_left[NUM_OF_CHAR] = {\n", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %d,", fbmp[i]->left);
        fputs(stt, fp);
    }
    fputs("\n};\n\n", fp);

    fputs("#ifdef __cplusplus\n", fp);
    fputs("}\n", fp);
    fputs("#endif\n\n", fp);
    fputs("#endif /*_FTSRC_H_*/\n", fp);

    fclose(fp);
    free(codes);
    return count;
}

void FT_usage(void)
{
    fprintf(stdout,"Usage: fthdr -f <font file> -s <font sting>\n\n");
}

#define BMP_TEST
#ifdef BMP_TEST
extern int create_bmp(uint32_t w,uint32_t h,uint8_t *buff);
int set_front_color(unsigned int color);
#endif

int main(int argc, char* argv[])
{
    if(argc !=5)
    {
        FT_usage();
        return -1;
    }
    int opt;
    char fontname[32];
    char *fontstring = NULL;
    while ((opt = getopt( argc, argv, "f:s:")) != EOF) {
        switch(opt){
        case 'f':
            strcpy(fontname, optarg);
            break;
        case 's':
            // strcpy(fontstring, optarg);
            fontstring = optarg;
            break;
        case '?':
            printf("Unknown option: %d\n", opt);
            FT_usage();
            return -1;
        default:
            printf("Other option: %d\n", opt);
            break;
        }
    }

    FT_tool_init(fontname);
    int len = strlen(fontstring);
    if (len <= 0)
        return -1;

    FILE *fp = fopen("ftsrc.h", "w");
    FT_BitmapGlyph *fbmp = malloc(len * sizeof(FT_BitmapGlyph));

    int count = FT_convert(fp, (const unsigned char *)fontstring, fbmp);

#ifdef BMP_TEST

    unsigned char *buff;
    unsigned int w = 0, h = height;

    int pos_x = 0;
    int pos_y;

    if (fbmp[0]->left < 0)
        w -= fbmp[0]->left;

    for (int i = 0; i < count; i++) {
        w += fbmp[i]->bitmap.pitch + fbmp[i]->left;
    }
    w += 5;//add some space to right

    buff = calloc(w*h,1);

    if (fbmp[0]->left < 0)
        pos_x -= fbmp[0]->left;

    for (int k = 0; k < count; k++) {
        pos_x += fbmp[k]->left;
        pos_y = 2 * height / 3 - fbmp[k]->top ;
        
        for (int i = 0; i < fbmp[k]->bitmap.pitch; i++) {
            for (uint32_t j = 0; j < fbmp[k]->bitmap.rows; j++) {
                //avoid overlaping
                if(*(buff+pos_x+i+(j+pos_y)*w) == 0)
                    *(buff+pos_x+i+(j+pos_y)*w) = *(fbmp[k]->bitmap.buffer+i+j*(fbmp[k]->bitmap.pitch));
            }
        }
        pos_x += fbmp[k]->bitmap.pitch;
    }

    set_front_color(0x7F2ABA);
    create_bmp( w, h, buff);
    free(buff);
#endif

    return 0;
}

