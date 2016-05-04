// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti駉 Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "text_renderer_gl.h"

#include "opengl.h"
#include "font_gl.h"
#include "leak_dumper.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <freetype/freetype.h>

const int VERTEX_POS_INDX = 0;
const int VERTEX_TEXCOORD_INDX  = 1;

const int VERTEX_POS_SIZE = 3;
const int VERTEX_TEXCOORD_SIZE = 2;

extern bool g_useOldFont;

namespace Shared{ namespace Graphics{ namespace Gl{

// =====================================================
//	class TextRenderer2DGl
// =====================================================

TextRenderer2DGl::TextRenderer2DGl(){
	rendering= false;
}

void TextRenderer2DGl::begin(const Font2D *font){
	assert(!rendering);
	rendering= true;
	
	this->m_font= static_cast<const Font2DGl*>(font);
}

void RenderTexture(GLfloat * vertices, GLfloat * texture_coords, GLshort * indices, int texture, int numIndices, GLuint programObject)
{
	int blend_enabled;
	glGetIntegerv(GL_BLEND, &blend_enabled);
	if(!blend_enabled)
	{
		glEnable(GL_BLEND);
	}

	//glEnable(GL_TEXTURE_2D);

	//opengles只能自己管理blend状态
	//int gl_blend_src, gl_blend_dst;
	//glGetIntegerv(GL_BLEND_SRC, &gl_blend_src);
	//glGetIntegerv(GL_BLEND_DST, &gl_blend_dst);

	//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	//glIsEnabled(GL_TEXTURE_2D);

	glUseProgram(programObject);
	
	GLint samplerLoc = glGetUniformLocation(programObject, "s_texture");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(samplerLoc, 0);

	assertGl();

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	assertGl();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	assertGl();

	glEnableVertexAttribArray(VERTEX_POS_INDX);
	assertGl();
	glEnableVertexAttribArray(VERTEX_TEXCOORD_INDX);
	assertGl();

	glVertexAttribPointer(VERTEX_POS_INDX, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, VERTEX_POS_SIZE * sizeof(float), vertices);
	assertGl();
	glVertexAttribPointer(VERTEX_TEXCOORD_INDX, VERTEX_TEXCOORD_SIZE, GL_FLOAT, GL_FALSE, VERTEX_TEXCOORD_SIZE * sizeof(float), texture_coords);
	assertGl();

	glDrawElements(GL_TRIANGLES, numIndices * 3, GL_UNSIGNED_SHORT, indices);
	assertGl();

	glDisableVertexAttribArray(VERTEX_POS_INDX);
	assertGl();
	glDisableVertexAttribArray(VERTEX_TEXCOORD_INDX);
	assertGl();

	glUseProgram(0);

	//glDisable(GL_TEXTURE_2D);

	// todo 恢复blend属性

	if (!blend_enabled) {
		glDisable(GL_BLEND);
	}
}

void TextRenderer2DGl::render(const string &text, int x, int y, bool centered){
	assert(rendering);
	
	assertGl();

	if(g_useOldFont)
	{
		int line=0;
		int size= m_font->getSize();
		const unsigned char *utext= reinterpret_cast<const unsigned char*>(text.c_str());

		Vec2f rasterPos;
		const FontMetrics *metrics= m_font->getMetrics();
		if(centered){
			rasterPos.x= x-metrics->getTextWidth(text)/2.f;
			rasterPos.y= y+metrics->getHeight()/2.f;
		}
		else{
			rasterPos= Vec2f(static_cast<float>(x), static_cast<float>(y));
		}
		glRasterPos2f(rasterPos.x, rasterPos.y);

		for (int i=0; utext[i]!='\0'; ++i) {
			switch(utext[i]){
			case '\t':
				rasterPos= Vec2f((rasterPos.x/size+3.f)*size, y-(size+1.f)*line);
				glRasterPos2f(rasterPos.x, rasterPos.y);
				break;
			case '\n':
				line++;
				rasterPos= Vec2f(static_cast<float>(x), y-(metrics->getHeight()*2.f)*line);
				glRasterPos2f(rasterPos.x, rasterPos.y);
				break;
			default:
				glCallList(m_font->getHandle()+utext[i]);
			}
		}
	}
	else
	{
		// 	{
		// 		char buffer[256];
		// 		sprintf_s(buffer, "%s %d %d %s\n", text.c_str(), x, y, centered?"true":"false");
		// 		OutputDebugString(buffer);
		// 	}

		int halfWidth = m_width / 2;
		int halfHeight = m_height / 2;

		x -= halfWidth;
		y -= halfHeight;

		font_t * SelfFont = m_font->GetFont();
		if (!SelfFont)
		{
			return;
		}

		if(!SelfFont)
		{
			assert(false);
		}

		if(!SelfFont->initialized)
		{
			assert(false);
		}

		const char * msg = text.c_str();
		if(!msg)
		{
			assert(false);
		}

		float length = 0;
		for(int i = 0; i < strlen(msg); ++i) 
		{
			char c = msg[i];
			length += SelfFont->advance[c];
		}
		x -= length / 2;

		GLfloat * vertices = (GLfloat*) malloc(sizeof(GLfloat) * 12 * strlen(msg));
		GLfloat * texture_coords = (GLfloat*) malloc(sizeof(GLfloat) * 8 * strlen(msg));
		GLshort * indices = (GLshort*) malloc(sizeof(GLfloat) * 6 * strlen(msg));

		float pen_x = 0;
		int numIndices = 0;
		for(int i = 0; i < strlen(msg); ++i) {
			char c = msg[i];

			vertices[12 * i + 0] = x + pen_x + SelfFont->offset_x[c];
			vertices[12 * i + 1] = y + SelfFont->offset_y[c];
			vertices[12 * i + 2] = 0;

			vertices[12 * i + 3] = vertices[12 * i + 0] + SelfFont->width[c];
			vertices[12 * i + 4] = vertices[12 * i + 1];
			vertices[12 * i + 5] = 0;

			vertices[12 * i + 6] = vertices[12 * i + 0];
			vertices[12 * i + 7] = vertices[12 * i + 1] + SelfFont->height[c];
			vertices[12 * i + 8] = 0;

			vertices[12 * i + 9] = vertices[12 * i + 3];
			vertices[12 * i + 10] = vertices[12 * i + 7];
			vertices[12 * i + 11] = 0;

			texture_coords[8 * i + 0] = SelfFont->tex_x1[c];
			texture_coords[8 * i + 1] = SelfFont->tex_y2[c];
			texture_coords[8 * i + 2] = SelfFont->tex_x2[c];
			texture_coords[8 * i + 3] = SelfFont->tex_y2[c];
			texture_coords[8 * i + 4] = SelfFont->tex_x1[c];
			texture_coords[8 * i + 5] = SelfFont->tex_y1[c];
			texture_coords[8 * i + 6] = SelfFont->tex_x2[c];
			texture_coords[8 * i + 7] = SelfFont->tex_y1[c];

			indices[i * 6 + 0] = 4 * i + 0;
			indices[i * 6 + 1] = 4 * i + 1;
			indices[i * 6 + 2] = 4 * i + 2;
			indices[i * 6 + 3] = 4 * i + 2;
			indices[i * 6 + 4] = 4 * i + 1;
			indices[i * 6 + 5] = 4 * i + 3;

			numIndices++;

			/* Assume we are only working with typewriter fonts */
			pen_x += SelfFont->advance[c];
		}

		for(int i = 0; i < numIndices * 12; ++i)
		{
			if(i % 3 == 0)
			{
				vertices[i] = vertices[i] / halfWidth ; // 屏幕半宽度;
			}
			else if(i % 3 == 1)
			{
				vertices[i] = vertices[i] / halfHeight ; // 屏幕半高度;
			}
			else
			{

			}
		}

		// 	for(int i = 0; i < numIndices * 12; i += 3)
		// 	{
		// 		char buffer[256];
		// 		sprintf_s(buffer, "vertex %d   %f   %f   %f\n", i / 3, vertices[i], vertices[i + 1], vertices[i + 2]);
		// 		OutputDebugString(buffer);
		// 	}
		// 
		// 	for(int i = 0; i < numIndices * 8; i += 2)
		// 	{
		// 		char buffer[256];
		// 		sprintf_s(buffer, "textureCoord %d   %f   %f\n", i / 2, texture_coords[i], texture_coords[i + 1]);
		// 		OutputDebugString(buffer);
		// 	}
		// 
		// 	for(int i = 0; i < numIndices * 6; i += 3)
		// 	{
		// 		char buffer[256];
		// 		sprintf_s(buffer, "indices %d   %d   %d  %d\n", i / 3, indices[i], indices[i + 1], indices[i + 2]);
		// 		OutputDebugString(buffer);
		// 	}

// 		GLuint tmp;
// 		glGenTextures(1, &tmp);
// 		glBindTexture(GL_TEXTURE_2D, tmp);
// 		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
// 		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

//		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, font->m_font_tex_width, font->m_font_tex_height, 0, GL_LUMINANCE_ALPHA , GL_UNSIGNED_BYTE, font->m_font_texture_data);


		RenderTexture(vertices, texture_coords, indices, SelfFont->font_texture, 2 * numIndices, m_font->GetShaderProgram());

//		glDeleteTextures(1, &tmp);

		free(vertices);
		free(texture_coords);
		free(indices);
	}

	assertGl();
}

void TextRenderer2DGl::end(){
	assert(rendering);
	rendering= false;
}

void TextRenderer2DGl::init( int width, int height )
{
	m_width = width;
	m_height = height;
}

// =====================================================
//	class TextRenderer3DGl
// =====================================================

// TextRenderer3DGl::TextRenderer3DGl(){
// 	rendering= false;
// }
// 
// void TextRenderer3DGl::begin(const Font3D *font){
// 	assert(!rendering);
// 	rendering= true;
// 	
// 	this->font= static_cast<const Font3DGl*>(font);
// 
// 	assertGl();
// 
// 	//load color
// 	glPushAttrib(GL_TRANSFORM_BIT);
// 
// 	assertGl();
// }
// 
// void TextRenderer3DGl::render(const string &text, float  x, float y, float size, bool centered){
// 	assert(rendering);
// 	
// 	assertGl();
// 
// 	const unsigned char *utext= reinterpret_cast<const unsigned char*>(text.c_str());
// 
// 	glMatrixMode(GL_MODELVIEW);
// 	glPushMatrix();
// 	glPushAttrib(GL_POLYGON_BIT);
// 	float scale= size/10.f;
// 	if(centered){
// 		const FontMetrics *metrics= font->getMetrics();
// 		glTranslatef(x-scale*metrics->getTextWidth(text)/2.f, y-scale*metrics->getHeight()/2.f, 0);
// 	}
// 	else{
// 		glTranslatef(x-scale, y-scale, 0);
// 	}
// 	glScalef(scale, scale, scale);
//                      
// 	for (int i=0; utext[i]!='\0'; ++i) {
// 		glCallList(font->getHandle()+utext[i]);
// 	}
// 
// 	glPopMatrix();
// 	glPopAttrib();
// 
// 	assertGl();
// }
// 
// void TextRenderer3DGl::end(){
// 	assert(rendering);
// 	rendering= false;
// 
// 	assertGl();
// 
// 	glPopAttrib();
// 
// 	assertGl();
// }

}}}//end namespace
