#pragma once

#include <string.h>

#ifdef __APPLE__
#include <unistd.h>
#include <sys/resource.h>

#include <OpenGLES/ES2/gl.h>
#else // __ANDROID__ or _WIN32
#include <GLES2/gl2.h>
#endif

class Renderer
{
public:
	Renderer();
	~Renderer();
	void Draw();
	void LoadCircle(GLfloat, GLfloat, GLfloat, GLfloat);
	void LoadColumn(GLfloat x, GLfloat y);
	void LoadFail();
	void UpdateWindowSize(GLsizei width, GLsizei height);
	bool click;
	bool failFlag;

private:
	GLuint mProgram;
	GLsizei mWindowWidth;
	GLsizei mWindowHeight;

	GLint mPositionAttribLocation;
	GLint mColorAttribLocation;

	GLint mModelUniformLocation;
	GLint mViewUniformLocation;
	GLint mProjUniformLocation;

	GLuint mVertexPositionBuffer;
	GLuint mVertexColorBuffer;

	GLfloat down;
	GLfloat xMoveStep;
	GLfloat randomHeight;
};
