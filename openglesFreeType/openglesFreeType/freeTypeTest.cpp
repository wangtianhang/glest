#include <assert.h>

#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "ft2build.h"
#include FT_FREETYPE_H

#include <freetype/freetype.h>

#include "freeTypeTest.h"

struct font_t {
	unsigned int font_texture;
	float pt;
	float advance[128];
	float width[128];
	float height[128];
	float tex_x1[128];
	float tex_x2[128];
	float tex_y1[128];
	float tex_y2[128];
	float offset_x[128];
	float offset_y[128];
	int initialized;
};

//font_t * g_font;
int g_charCount = 128;
int g_num_segment_x = 16;
int g_num_segment_y = 8;

static inline int
	nextp2(int x)
{
	int val = 1;
	while(val < x) val <<= 1;
	return val;
}

font_t * LoadFont(std::string path, int pointSize, int dpi)
{
	FT_Library library;
	FT_Face face;
	int ftError = FT_Init_FreeType(&library);
	if(ftError) 
	{
		assert(false);
	}

	ftError = FT_New_Face(library, path.c_str(), 0, &face);
	if (ftError) 
	{
		assert(false);
	}

	ftError = FT_Set_Char_Size ( face, pointSize * 64, pointSize * 64, dpi, dpi);
	if(ftError) 
	{
		assert(false);
	}

	font_t * font = new font_t();
	font->initialized = 0;

	int segment_size_x = 0;
	int segment_size_y = 0;

	//FT_GlyphSlot glyphSlot;
	//FT_Bitmap bmp;

	glGenTextures(1, &(font->font_texture));

	for(int c = 0; c < g_charCount; c++)
	{
		ftError = FT_Load_Char(face, c, FT_LOAD_RENDER);
		if(ftError)
		{
			assert(false);
		}

		FT_GlyphSlot glyphSlot = face->glyph;
		FT_Bitmap bmp = glyphSlot->bitmap;

		if(bmp.width > segment_size_x)
		{
			segment_size_x = bmp.width;
		}
		if(bmp.rows > segment_size_y)
		{
			segment_size_y = bmp.rows;
		}
	}

	int font_tex_width = nextp2(g_num_segment_x * segment_size_x);
	int font_tex_height = nextp2(g_num_segment_y * segment_size_y);

	int textureSize = sizeof(GLubyte) * 2 * font_tex_width * font_tex_height;
	GLubyte * font_texture_data = (GLubyte *)malloc(textureSize);
	memset((void *)font_texture_data, 0, textureSize);

	for(int c = 0; c < g_charCount; c++)
	{
		ftError = FT_Load_Char(face, c, FT_LOAD_RENDER);
		if(ftError)
		{
			assert(false);
		}

		FT_GlyphSlot glyphSlot = face->glyph;
		FT_Bitmap bmp = glyphSlot->bitmap;

		int glyph_width = nextp2(bmp.width);
		int glyph_height = nextp2(bmp.rows);

		div_t temp = div(c, g_num_segment_x);

		int bitmap_offset_x = segment_size_x * temp.rem;
		int bitmap_offset_y = segment_size_y * temp.quot;

		for (int j = 0; j < glyph_height; j++) {
			for (int i = 0; i < glyph_width; i++) {
				font_texture_data[2 * ((bitmap_offset_x + i) + (j + bitmap_offset_y) * font_tex_width) + 0] =
				font_texture_data[2 * ((bitmap_offset_x + i) + (j + bitmap_offset_y) * font_tex_width) + 1] =
					(i >= bmp.width || j >= bmp.rows)? 0 : bmp.buffer[i + bmp.width * j];
			}
		}

		font->advance[c] = (float)(glyphSlot->advance.x >> 6);
		font->tex_x1[c] = (float)bitmap_offset_x / (float) font_tex_width;
		font->tex_x2[c] = (float)(bitmap_offset_x + bmp.width) / (float)font_tex_width;
		font->tex_y1[c] = (float)bitmap_offset_y / (float) font_tex_height;
		font->tex_y2[c] = (float)(bitmap_offset_y + bmp.rows) / (float)font_tex_height;
		font->width[c] = bmp.width;
		font->height[c] = bmp.rows;
		font->offset_x[c] = (float)glyphSlot->bitmap_left;
		font->offset_y[c] =  (float)((glyphSlot->metrics.horiBearingY-face->glyph->metrics.height) >> 6);
	}

	glBindTexture(GL_TEXTURE_2D, font->font_texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, font_tex_width, font_tex_height, 0, GL_LUMINANCE_ALPHA , GL_UNSIGNED_BYTE, font_texture_data);
	int glErr = glGetError();
	if(glErr)
	{
		assert(false);
	}

	free(font_texture_data);

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	font->initialized = 1;

	return font;
}

void InitFreeType()
{
	LoadFont("c:\\windows\\fonts\verdana.ttf", 15, 72);
}

void RenderText( std::string text, int x, int y )
{

}
