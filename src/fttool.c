/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018-2019 wan.junjie
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_WINFONTS_H
#include FT_BITMAP_H

#define FONT_FILE "arial.ttf"

#define outputfile

FT_Face pFTFace = NULL; 
FT_Library pFTLib = NULL; 

struct char_gly{
    unsigned int rows;
    unsigned int width;
    int pitch;
};

int count=0;

int height ;

int FT_tool_init()
{
    FT_Error error = 0;
    error = FT_Init_FreeType( &pFTLib);
    if (error)
    {
        printf("There is some error when Init Library");
        return -1;  
    }
    error = FT_New_Face(pFTLib,FONT_FILE, 0, &pFTFace);

    if( error == FT_Err_Unknown_File_Format)
    {
        printf("file format errot\n");
        return -1;
    }
    else if ( error )
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

FT_BitmapGlyph *fbmp = NULL;

FILE* FT_prealloc(int num)
{
    FILE *fp = fopen("ftsrc.h","w");
    fbmp = malloc(num*sizeof(FT_BitmapGlyph));
    return fp;
}

int FT_convert_code(FILE *fp,unsigned int code)
{
    char stt[12];
    FT_Error error = 0;
    FT_Glyph glyph = NULL;
    FT_GlyphSlot pGlyphSlot;
    //FT_Glyph_Metrics glphy_Metrics;
    FT_BitmapGlyph bitmap_glyph;
    FT_Bitmap bitmap;
    FT_BBox bbox;
    int m=0,n=0;
    static int cnt = 0;
    
    FT_UInt index = FT_Get_Char_Index(pFTFace, code);

    FT_Load_Glyph(pFTFace, index, FT_LOAD_DEFAULT);

    error = FT_Get_Glyph(pFTFace->glyph, &glyph);
    if( error)
    {
        return -1; 
    }
    pGlyphSlot = pFTFace->glyph;
    FT_Glyph_Get_CBox( glyph, FT_GLYPH_BBOX_TRUNCATE, &bbox );

    if (pFTFace->glyph->format != FT_GLYPH_FORMAT_BITMAP) 
    {
        /* this mode pitch equals width */
        FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1);
    }

    bitmap_glyph = (FT_BitmapGlyph)glyph;
    bitmap = bitmap_glyph->bitmap;

    fbmp[cnt++] = bitmap_glyph;

    for(m=0; m<bitmap.rows; ++m)
    {
        fputs("\t\t",fp);
        for(n=0; n<bitmap.width; ++n)
        {
            sprintf(stt,"0x%02X,",bitmap.buffer[m*bitmap.width + n]);
            fputs(stt,fp);
        }
        sprintf(stt,"\n");
        fputs(stt, fp);
    }
    sprintf(stt,"\t},\n");
    fputs(stt, fp);
    //FT_Done_Glyph(glyph);
}

int FT_convert(FILE* fp,unsigned char * string)
{
    int len = strlen(string);
    FT_Error error = 0;
    
    char stt[32];
    int i=0;
    fputs("#ifndef _FTSRC_H_\n",fp);
    fputs("#define _FTSRC_H_\n\n",fp);
    fputs("#ifdef __cplusplus\n",fp);
    fputs("extern \"C\" {\n",fp);
    fputs("#endif\n\n",fp);
    fputs("const unsigned char char_code[][] = {\n",fp);

    for(i=0;i<len;i++)
    {
        FT_convert_code(fp,string[i]);
    }

    sprintf(stt,"};\n\n");
    fputs(stt, fp);

    fputs("/*",fp);
    for(i=0;i<len; i++)
    {
        sprintf(stt," %c,",string[i]);
        fputs(stt,fp);
    }
    fputs("*/\n\n",fp);
    
    fputs("const unsigned int char_rows[] = {\n",fp);
    for(i=0;i<len; i++)
    {
        sprintf(stt," %u,",fbmp[i]->bitmap.rows);
        fputs(stt,fp);
    }
    fputs("\n}\n\n",fp);
    
    fputs("const unsigned int char_width[] = {\n",fp);
    for(i=0;i<len; i++)
    {
        sprintf(stt," %u,",fbmp[i]->bitmap.width);
        fputs(stt,fp);
    }
    fputs("\n}\n\n",fp);
    
    fputs("const int char_pitch[] = {\n",fp);
    for(i=0;i<len; i++)
    {
        sprintf(stt," %u,",fbmp[i]->bitmap.pitch);
        fputs(stt,fp);
    }
    fputs("\n}\n\n",fp);

    fputs("#ifdef __cplusplus\n",fp);
    fputs("}\n",fp);
    fputs("#endif\n\n",fp);
    fputs("#endif /*_FTSRC_H_*/\n",fp);

    fclose(fp);

}

extern int create_bmp(uint32_t w,uint32_t h,uint8_t *buff);

void FT_usage()
{
    fprintf(stdout,"Usage: fthdt <font sting>\n\n");
}

int main(int argc, char* argv[])
{
    if(argc<2||argc>2)
    {
        FT_usage();
        return -1;
    }
    FILE *fp;
    FT_tool_init();
    int len = strlen(argv[1]);
    fp = FT_prealloc(len);
    //printf("string to %d use %s\n",len, argv[1]);
    FT_convert(fp,argv[1]);
    int i =0, j=0,k;
    unsigned char *buff;
    unsigned int w=0, h=height;

    int pos_x = 0;
    int pos_y;

    for(i=0; i<len; i++)
    {
        w += fbmp[i]->bitmap.pitch + fbmp[i]->left;
    }

    buff = calloc(w*h,1);

    for(k=0; k<len; k++)
    {
        pos_x += fbmp[k]->left;
        pos_y = fbmp[k]->bitmap.rows - fbmp[k]->top;
        //printf("top %d, rows %d\n",pos_y,fbmp[k]->bitmap.rows);
        for(i=0; i<fbmp[k]->bitmap.pitch; i++)
        {
            for(j=0; j< fbmp[k]->bitmap.rows; j++)
                *(buff+pos_x+i+(j+pos_y)*w) = *(fbmp[k]->bitmap.buffer+i+j*(fbmp[k]->bitmap.pitch));
        }
        pos_x += fbmp[k]->bitmap.pitch;//+fbmp[k]->left;
    }
    create_bmp( w, h, buff);

    return 0;
}

