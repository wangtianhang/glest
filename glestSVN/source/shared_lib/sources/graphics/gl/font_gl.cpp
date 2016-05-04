// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "font_gl.h"

#include "opengl.h"
#include "gl_wrap.h"
#include "leak_dumper.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <freetype/freetype.h>

bool g_useOldFont = false;

namespace Shared{ namespace Graphics{ namespace Gl{

using namespace Platform;

// =====================================================
//	class Font2DGl
// =====================================================

//font_t * g_font = NULL;
//GLuint g_programObject = 0;

int g_charCount = 128;
int g_num_segment_x = 16;
int g_num_segment_y = 8;



//const int strSize = 256;

int	nextp2(int x)
{
	int val = 1;
	while(val < x) val <<= 1;
	return val;
}

int CalculateDpi()
{
	return 96;
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

	int fontBold = 2;///???

	int textureSize = sizeof(GLubyte) * fontBold * font_tex_width * font_tex_height;
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
				font_texture_data[fontBold * ((bitmap_offset_x + i) + (j + bitmap_offset_y) * font_tex_width) + 0] =
					font_texture_data[fontBold * ((bitmap_offset_x + i) + (j + bitmap_offset_y) * font_tex_width) + 1] =
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

	glGenTextures(1, &(font->font_texture));
	glBindTexture(GL_TEXTURE_2D, font->font_texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	//font->m_font_tex_width = font_tex_width;
	//font->m_font_tex_height = font_tex_height;
	//font->m_font_texture_data = font_texture_data;

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

GLuint LoadShader(GLenum type, const char * shaderSrc)
{
	GLuint shader = glCreateShader(type);
	if(shader == 0)
	{
		return 0;
	}

	glShaderSource(shader, 1, &shaderSrc, NULL);

	glCompileShader(shader);

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if(!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1)
		{
			char * infoLog = (char *)malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			printf("%s", infoLog);
			assert(false);
			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

void Font2DGl::init(){
	assertGl();

// 	if(!inited){
// 		handle= glGenLists(charCount);
// 		//createGlFontBitmaps(handle, type, size, width, charCount, metrics);
// 		inited= true;
// 	}

	if(!inited)
	{
		if(g_useOldFont)
		{
			handle= glGenLists(charCount);
			createGlFontBitmaps(handle, type, size, width, charCount, metrics);
		}
		else
		{
			m_font = NULL;

			int pointSize = size * 0.9f;
			int dpi = CalculateDpi();
			m_font = LoadFont("c:\\windows\\fonts\\verdana.ttf", pointSize, dpi);

			GLchar vShader[] = 
				"attribute vec4 vPosition;\n"
				"attribute vec2 vTexCoord;\n"
				"varying vec2 ToFragmentTexCoord;\n"
				"void main(void)\n"
				"{\n"
				"	gl_Position = vec4(vPosition.x, vPosition.y, vPosition.z, vPosition.w);\n"
				"	ToFragmentTexCoord = vTexCoord;\n"
				"}\n";

			GLchar fShader[] = 
				//"precision mediump float;\n"
				"uniform sampler2D s_texture;\n"
				"varying vec2 ToFragmentTexCoord;\n"
				"void main(void)\n"
				"{\n"
				"	gl_FragColor = texture2D(s_texture, ToFragmentTexCoord);\n"
				"}\n";

			GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vShader);
			GLuint fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShader);

			m_programObject = glCreateProgram();

			glAttachShader(m_programObject, vertexShader);
			glAttachShader(m_programObject, fragmentShader);

			glLinkProgram(m_programObject);

			GLint linked;
			glGetProgramiv(m_programObject, GL_LINK_STATUS, &linked);
			if(!linked)
			{
				GLint infoLen = 0;
				glGetProgramiv(m_programObject, GL_INFO_LOG_LENGTH, &infoLen);
				if(infoLen > 1)
				{
					char * infoLog = (char *)malloc(sizeof(char) * infoLen);
					glGetProgramInfoLog(m_programObject, infoLen, NULL, infoLog);
					printf("%s", infoLog);
					assert(false);
					free(infoLog);
				}
			}

			//glUseProgram(m_programObject);
			//GLint samplerLoc = glGetUniformLocation(m_programObject, "s_texture");
			//assertGl();
			//glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, m_font->font_texture);
			//glUniform1i(samplerLoc, 0);
			//assertGl();
			//glUseProgram(0);
		}

		inited = true;
	}

	assertGl();
}

void Font2DGl::end(){
	assertGl();

	if(inited){
		if(g_useOldFont)
		{
			assert(glIsList(handle));
			glDeleteLists(handle, 1);
		}
		inited= false;
	}

	assertGl();
}

font_t * Font2DGl::GetFont() const
{
	return m_font;
}

GLuint Font2DGl::GetShaderProgram() const
{
	return m_programObject;
}

// FT_Face * Font2DGl::GetFTFace()
// {
// 	return m_facePointer;
// }

// =====================================================
//	class Font3DGl
// =====================================================

// void Font3DGl::init(){
// 	assertGl();
// 
// 	if(!inited){
// 		handle= glGenLists(charCount);
// 		createGlFontOutlines(handle, type, width, depth, charCount, metrics);
// 		inited= true;
// 	}
// 
// 	assertGl();
// }
// 
// void Font3DGl::end(){
// 	assertGl();
// 
// 	if(inited){
// 		assert(glIsList(handle));
// 		glDeleteLists(handle, 1);
// 	}
// 
// 	assertGl();
// }

}}}//end namespace
