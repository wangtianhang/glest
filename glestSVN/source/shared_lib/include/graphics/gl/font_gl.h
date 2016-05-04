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

#ifndef _SHARED_GRAPHICS_GL_FONTGL_H_
#define _SHARED_GRAPHICS_GL_FONTGL_H_

#include "font.h"
#include "opengl.h"

// #include <ft2build.h>
// #include FT_FREETYPE_H
// 
// #include <freetype/freetype.h>
//typedef struct * FT_Face;

//typedef struct FT_FaceRec_*  FT_Face;

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

	int m_font_tex_width;
	int m_font_tex_height;
	GLubyte * m_font_texture_data;

	int initialized;
};

namespace Shared{ namespace Graphics{ namespace Gl{

//struct FT_Library;

// =====================================================
//	class FontGl
// =====================================================

class FontGl{
protected:
	GLuint handle;

public:
	GLuint getHandle() const				{return handle;}
};

// =====================================================
//	class Font2DGl
//
///	OpenGL bitmap font
// =====================================================

class Font2DGl: public Font2D, public FontGl{
	//FT_Library  * m_library;
	//FT_Face * m_facePointer;
	font_t * m_font;
	GLuint m_programObject;

public:
	//FT_Face * GetFTFace();

	virtual void init();
	virtual void end();

	font_t * GetFont() const;
	GLuint GetShaderProgram() const;
};

// =====================================================
//	class Font3DGl
//
///	OpenGL outline font
// =====================================================

// class Font3DGl: public Font3D, public FontGl{
// public:
// 	virtual void init();
// 	virtual void end();
// };

}}}//end namespace

#endif
