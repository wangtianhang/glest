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

// typedef struct
// {
// 	GLuint programObject;
// 
// } UserData;

font_t * g_font = NULL;
int g_charCount = 128;
int g_num_segment_x = 16;
int g_num_segment_y = 8;
//int g_surface_width = 0;
//int g_surface_height = 0;
GLuint g_programObject = 0;
//GLuint g_vboVertexPos = 0;
//GLuint g_vboVertexCoord = 0;
//GLuint g_vboVertexIndex = 0;

#define VERTEX_POS_INDX 0
#define VERTEX_TEXCOORD_INDX 1

#define VERTEX_POS_SIZE 2
#define VERTEX_TEXCOORD_SIZE 2

static inline int
	nextp2(int x)
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

void InitFreeType()
{
	int pointSize = 15;
	int dpi = CalculateDpi();
	g_font = LoadFont("c:\\windows\\fonts\\verdana.ttf", pointSize, dpi);

	//EGLint surface_width, surface_height;
// 	EGLDisplay egl_disp = 0;
// 	EGLSurface egl_surf = 0;
// 	eglQuerySurface(egl_disp, egl_surf, EGL_WIDTH, &g_surface_width);
// 	eglQuerySurface(egl_disp, egl_surf, EGL_HEIGHT, &g_surface_height);
// 	EGLint err = eglGetError();
// 	if (err != 0x3000) 
// 	{
// 		//fprintf(stderr, "Unable to query EGL surface dimensions\n");
// 		//return EXIT_FAILURE;
// 		assert(false);
// 	}

	GLchar vShader[] = 
		"attribute vec2 vPosition;\n"
		"attribute vec2 vTexCoord;\n"
		"varying vec2 ToFragmentTexCoord;\n"
		"void main(void)\n"
		"{\n"
		"	gl_Position = vec4(vPosition.x, vPosition.y, 0, 0);\n"
		"	ToFragmentTexCoord = vTexCoord;\n"
		"}\n";

	GLchar fShader[] = 
		"precision mediump float;\n"
		"uniform sampler2D textureSampler;\n"
		"varying vec2 ToFragmentTexCoord;\n"
		"void main(void)\n"
		"{\n"
		"	gl_FragColor = texture2D(textureSampler, ToFragmentTexCoord);\n"
		"}\n";
	//LoadShader();

	GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vShader);
	GLuint fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShader);

	g_programObject = glCreateProgram();

	glAttachShader(g_programObject, vertexShader);
	glAttachShader(g_programObject, fragmentShader);

	glLinkProgram(g_programObject);

	GLint linked;
	glGetProgramiv(g_programObject, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLint infoLen = 0;
		glGetProgramiv(g_programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1)
		{
			char * infoLog = (char *)malloc(sizeof(char) * infoLen);

			glGetProgramInfoLog(g_programObject, infoLen, NULL, infoLog);
			printf("%s", infoLog);
			assert(false);

			free(infoLog);
		}
	}

	GLint samplerLoc = glGetUniformLocation(g_programObject, "textureSampler");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_font->font_texture);
	glUniform1i(samplerLoc, 0);
}

void RenderTexture(GLfloat * vertices, GLfloat * texture_coords, GLshort * indices, int texture, int numIndices)
{
	int blend_enabled;
	glGetIntegerv(GL_BLEND, &blend_enabled);
	if(!blend_enabled)
	{
		glEnable(GL_BLEND);
	}

	//opengles只能自己管理blend状态
	//int gl_blend_src, gl_blend_dst;
	//glGetIntegerv(GL_BLEND_SRC, &gl_blend_src);
	//glGetIntegerv(GL_BLEND_DST, &gl_blend_dst);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(g_programObject);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glEnableVertexAttribArray(VERTEX_POS_INDX);
	glEnableVertexAttribArray(VERTEX_TEXCOORD_INDX);

	glVertexAttribPointer(VERTEX_POS_INDX, VERTEX_POS_SIZE, GL_FLOAT, GL_FALSE, VERTEX_POS_SIZE * sizeof(float), vertices);
	glVertexAttribPointer(VERTEX_TEXCOORD_INDX, VERTEX_TEXCOORD_SIZE, GL_FLOAT, GL_FALSE, VERTEX_TEXCOORD_SIZE * sizeof(float), texture_coords);

	glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indices);
	
	glDisableVertexAttribArray(VERTEX_POS_INDX);
	glDisableVertexAttribArray(VERTEX_TEXCOORD_INDX);
	
	// todo 恢复blend属性

	if (!blend_enabled) {
		glDisable(GL_BLEND);
	}
}

void RenderText( const char * msg, int x, int y )
{
	// 设为2d渲染
	//glViewport(0, 0, (int) g_surface_width, (int) g_surface_height);
	// 调用shader渲染

	if(!g_font)
	{
		assert(false);
	}

	if(!g_font->initialized)
	{
		assert(false);
	}

	if(!msg)
	{
		assert(false);
	}

// 	int texture_enabled;
// 	glGetIntegerv(GL_TEXTURE_2D, &texture_enabled);
// 	if(!texture_enabled)
// 	{
// 		glEnable(GL_TEXTURE_2D);
// 	}



	//int vertex_array_enabled;
	//glGetIntegerv(GL_VERTEX_ATTRIB_ARRAY_ENABLED, &vertex_array_enabled);

	GLfloat * vertices = (GLfloat*) malloc(sizeof(GLfloat) * 8 * strlen(msg));
	GLfloat * texture_coords = (GLfloat*) malloc(sizeof(GLfloat) * 8 * strlen(msg));
	GLshort * indices = (GLshort*) malloc(sizeof(GLfloat) * 5 * strlen(msg));

	float pen_x = 0;
	int numIndices = 0;
	for(int i = 0; i < strlen(msg); ++i) {
		char c = msg[i];

		vertices[8 * i + 0] = x + pen_x + g_font->offset_x[c];
		vertices[8 * i + 1] = y + g_font->offset_y[c];
		vertices[8 * i + 2] = vertices[8 * i + 0] + g_font->width[c];
		vertices[8 * i + 3] = vertices[8 * i + 1];
		vertices[8 * i + 4] = vertices[8 * i + 0];
		vertices[8 * i + 5] = vertices[8 * i + 1] + g_font->height[c];
		vertices[8 * i + 6] = vertices[8 * i + 2];
		vertices[8 * i + 7] = vertices[8 * i + 5];

		texture_coords[8 * i + 0] = g_font->tex_x1[c];
		texture_coords[8 * i + 1] = g_font->tex_y2[c];
		texture_coords[8 * i + 2] = g_font->tex_x2[c];
		texture_coords[8 * i + 3] = g_font->tex_y2[c];
		texture_coords[8 * i + 4] = g_font->tex_x1[c];
		texture_coords[8 * i + 5] = g_font->tex_y1[c];
		texture_coords[8 * i + 6] = g_font->tex_x2[c];
		texture_coords[8 * i + 7] = g_font->tex_y1[c];

		indices[i * 6 + 0] = 4 * i + 0;
		indices[i * 6 + 1] = 4 * i + 1;
		indices[i * 6 + 2] = 4 * i + 2;
		indices[i * 6 + 3] = 4 * i + 2;
		indices[i * 6 + 4] = 4 * i + 1;
		indices[i * 6 + 5] = 4 * i + 3;

		numIndices++;

		/* Assume we are only working with typewriter fonts */
		pen_x += g_font->advance[c];
	}

	// Enable the user-defined vertex array
	//glEnableVertexAttribArray(VertexArray);
	RenderTexture(vertices, texture_coords, indices, g_font->font_texture, numIndices);
	



// 	if (!texture_enabled) {
// 		glDisable(GL_TEXTURE_2D);
// 	}

	free(vertices);
	free(texture_coords);
	free(indices);
}


