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
#include <assert.h>

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

static int FT_load_code(unsigned long code, FT_BitmapGlyph *fbmp, int idx) {
    FT_Error error = 0;
    FT_Glyph glyph = NULL;
    FT_BitmapGlyph bitmap_glyph;

    FT_UInt index = FT_Get_Char_Index(pFTFace, code);

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

    fbmp[idx] = bitmap_glyph;
    return error;
}

static int FT_convert_code(FILE *fp, FT_BitmapGlyph fbmp) {
    char stt[12];

    FT_Bitmap *bitmap = &fbmp->bitmap;

    fputs("\t{\n", fp);
    for (uint32_t m = 0; m < bitmap->rows; ++m) {
        fputs("\t\t", fp);
        for (uint32_t n = 0; n < bitmap->width; ++n) {
        sprintf(stt, "0x%02X,", bitmap->buffer[m * bitmap->width + n]);
        fputs(stt, fp);
        }
        fputs("\n", fp);
    }
    fputs("\t},\n", fp);
    return bitmap->rows * bitmap->width;
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
            assert(0);
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

    char stt[128];
    int i = 0;
    int count = 0;
    unsigned int maxsize = 0;
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
    for (i = 0; i < count; i++) {
        FT_load_code(codes[i], fbmp, i);
        maxsize = fbmp[i]->bitmap.rows * fbmp[i]->bitmap.width > maxsize
                    ? fbmp[i]->bitmap.rows * fbmp[i]->bitmap.width
                    : maxsize;
    }
    sprintf(stt, "const unsigned char char_code[NUM_OF_CHAR][%d] = {\n",
            maxsize);
    fputs(stt, fp);
    for (i = 0; i < count; i++) {
        FT_convert_code(fp, fbmp[i]);
    }
    fputs("};\n\n", fp);
    fputs("/*", fp);
    for (i = 0; i < count; i++) {
        sprintf(stt, " %c,", codes[i]);
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
    fprintf(stdout,"Usage: fonthdr -f <font file> -s <font sting>\n\n");
}

int main(int argc, char* argv[])
{
    if(argc !=5)
    {
        FT_usage();
        return -1;
    }
    int opt;
    char fontname[1024];
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
    if (count == 0) {
        fclose(fp);
        free(fbmp);
        return -1;
    }

    fclose(fp);
    free(fbmp);
    return 0;
}
