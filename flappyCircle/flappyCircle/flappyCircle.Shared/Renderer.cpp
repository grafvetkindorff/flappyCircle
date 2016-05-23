//
// This file is used by the template to render a basic scene using GL.
//
#include "pch.h"

#include "Renderer.h"
#include "MathHelper.h"

// These are used by the shader compilation methods.
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>

GLuint CompileShader(GLenum type, const std::string &source)
{
	GLuint shader = glCreateShader(type);

	const char *sourceArray[1] = { source.c_str() };
	glShaderSource(shader, 1, sourceArray, NULL);
	glCompileShader(shader);

	GLint compileResult;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

	if (compileResult == 0)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		std::vector<GLchar> infoLog(infoLogLength);
		glGetShaderInfoLog(shader, (GLsizei)infoLog.size(), NULL, infoLog.data());

		std::string errorMessage = std::string("Shader compilation failed: ");
		errorMessage += std::string(infoLog.begin(), infoLog.end());

		throw std::runtime_error(errorMessage.c_str());
	}

	return shader;
}

GLuint CompileProgram(const std::string &vsSource, const std::string &fsSource)
{
	GLuint program = glCreateProgram();

	if (program == 0)
	{
		throw std::runtime_error("Program creation failed");
	}

	GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
	GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);

	if (vs == 0 || fs == 0)
	{
		glDeleteShader(fs);
		glDeleteShader(vs);
		glDeleteProgram(program);
		return 0;
	}

	glAttachShader(program, vs);
	glDeleteShader(vs);

	glAttachShader(program, fs);
	glDeleteShader(fs);

	glLinkProgram(program);

	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

	if (linkStatus == 0)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		std::vector<GLchar> infoLog(infoLogLength);
		glGetProgramInfoLog(program, (GLsizei)infoLog.size(), NULL, infoLog.data());

		std::string errorMessage = std::string("Program link failed: ");
		errorMessage += std::string(infoLog.begin(), infoLog.end());

		throw std::runtime_error(errorMessage.c_str());
	}

	return program;
}

Renderer::Renderer() :
	click(false),
	failFlag(false),
	mWindowWidth(0),
	mWindowHeight(0),
	down(0.0f),
	xMoveStep(0.0f)
{
	// Vertex Shader source
	const std::string vs = R"(
        uniform mat4 uModelMatrix;
        uniform mat4 uViewMatrix;
        uniform mat4 uProjMatrix;
        attribute vec4 aPosition;
        attribute vec4 aColor;
        varying vec4 vColor;
        void main()
        {
            gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * aPosition;
            vColor = aColor;
        }
    )";

	// Fragment Shader source
	const std::string fs = R"(
        precision mediump float;
        varying vec4 vColor;
        void main()
        {
            gl_FragColor = vColor;
        }
    )";

	// Set up the shader and its uniform/attribute locations.
	mProgram = CompileProgram(vs, fs);
	mPositionAttribLocation = glGetAttribLocation(mProgram, "aPosition");
	mColorAttribLocation = glGetAttribLocation(mProgram, "aColor");
	mModelUniformLocation = glGetUniformLocation(mProgram, "uModelMatrix");
	mViewUniformLocation = glGetUniformLocation(mProgram, "uViewMatrix");
	mProjUniformLocation = glGetUniformLocation(mProgram, "uProjMatrix");

	randomHeight = MathHelper::get_random(0.0f, 1.8f);
}

Renderer::~Renderer()
{
	if (mProgram != 0)
	{
		glDeleteProgram(mProgram);
		mProgram = 0;
	}

	if (mVertexPositionBuffer != 0)
	{
		glDeleteBuffers(1, &mVertexPositionBuffer);
		mVertexPositionBuffer = 0;
	}

	if (mVertexColorBuffer != 0)
	{
		glDeleteBuffers(1, &mVertexColorBuffer);
		mVertexColorBuffer = 0;
	}
}

void Renderer::LoadFail()
{
	GLfloat vertexPositions[] =
	{
		1.0f,  2.0f, 0.0f,
		-1.0f,  2.0f, 0.0f,
		-1.0f, -2.0f, 0.0f,
		-1.0f, -2.0f, 0.0f,
		1.0f, -2.0f, 0.0f,
		1.0f,  2.0f, 0.0f,
	};


	glGenBuffers(1, &mVertexPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

	GLfloat vertexColors[] =
	{
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};

	glGenBuffers(1, &mVertexColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
	glEnableVertexAttribArray(mPositionAttribLocation);
	glVertexAttribPointer(mPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
	glEnableVertexAttribArray(mColorAttribLocation);
	glVertexAttribPointer(mColorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	MathHelper::Matrix4 modelMatrix = MathHelper::MoveModelMatrix(0, -0.05f);
	glUniformMatrix4fv(mModelUniformLocation, 1, GL_FALSE, &(modelMatrix.m[0][0]));

	MathHelper::Matrix4 viewMatrix = MathHelper::SimpleViewMatrix();
	glUniformMatrix4fv(mViewUniformLocation, 1, GL_FALSE, &(viewMatrix.m[0][0]));

	MathHelper::Matrix4 projectionMatrix = MathHelper::SimpleProjectionMatrix(float(mWindowWidth) / float(mWindowHeight));
	glUniformMatrix4fv(mProjUniformLocation, 1, GL_FALSE, &(projectionMatrix.m[0][0]));

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::LoadColumn(GLfloat x, GLfloat y)
{
	GLfloat vertexPositions[] =
	{
		// First column
		1.5f - xMoveStep, 1.0f, 0.0f,
		1.0f - xMoveStep, 1.0f, 0.0f,
		1.0f - xMoveStep, 1.0f - randomHeight, 0.0f,
		1.0f - xMoveStep, 1.0f - randomHeight, 0.0f,
		1.5f - xMoveStep, 1.0f - randomHeight, 0.0f,
		1.5f - xMoveStep, 1.0f, 0.0f,

		// Second column
		1.5f - xMoveStep, 1.0f - randomHeight - 0.5f, 0.0f,
		1.0f - xMoveStep, 1.0f - randomHeight - 0.5f, 0.0f,
		1.0f - xMoveStep, -1.5f, 0.0f,
		1.0f - xMoveStep, -1.5f, 0.0f,
		1.5f - xMoveStep, -1.5f, 0.0f,
		1.5f - xMoveStep, 1.0f - randomHeight - 0.5f, 0.0f,
	};


	glGenBuffers(1, &mVertexPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

	GLfloat vertexColors[] =
	{
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
	};

	glGenBuffers(1, &mVertexColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
	glEnableVertexAttribArray(mPositionAttribLocation);
	glVertexAttribPointer(mPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
	glEnableVertexAttribArray(mColorAttribLocation);
	glVertexAttribPointer(mColorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	MathHelper::Matrix4 modelMatrix = MathHelper::MoveModelMatrix(0, -0.05f);
	glUniformMatrix4fv(mModelUniformLocation, 1, GL_FALSE, &(modelMatrix.m[0][0]));

	MathHelper::Matrix4 viewMatrix = MathHelper::SimpleViewMatrix();
	glUniformMatrix4fv(mViewUniformLocation, 1, GL_FALSE, &(viewMatrix.m[0][0]));

	MathHelper::Matrix4 projectionMatrix = MathHelper::SimpleProjectionMatrix(float(mWindowWidth) / float(mWindowHeight));
	glUniformMatrix4fv(mProjUniformLocation, 1, GL_FALSE, &(projectionMatrix.m[0][0]));

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDrawArrays(GL_TRIANGLES, 6, 6);

	xMoveStep += 0.012f;
	if (xMoveStep >= 2.5f) {
		xMoveStep = 0.01f;
		randomHeight = MathHelper::get_random(0.0f, 1.8f);
	}
}

void Renderer::LoadCircle(GLfloat center_x
	, GLfloat center_y
	, GLfloat center_z
	, GLfloat radius)
{
	// Then set up the circle geometry.
	GLint vertexCount = 361;

	// Create a buffer for vertex data
	GLfloat vertexPositions[vertexCount * 3]; // (x,y,z) for each vertex
	GLint idx = 0;

	// Center vertex
	vertexPositions[idx++] = center_x;
	vertexPositions[idx++] = center_y;
	vertexPositions[idx++] = center_z;

	for (GLint angle = 1; angle <= 360; ++angle) {
		vertexPositions[idx++] = center_x + sin(angle) * radius;
		vertexPositions[idx++] = center_y + cos(angle) * radius;
		vertexPositions[idx++] = 0.0f;
	}

	glGenBuffers(1, &mVertexPositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);

	idx = 0;
	GLfloat vertexColors[vertexCount * 3];
	for (GLint i = 0; i < 361; ++i) {
		vertexColors[idx++] = 1.0f;
		vertexColors[idx++] = 0.0f;
		vertexColors[idx++] = 0.0f;
	}

	glGenBuffers(1, &mVertexColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
	glEnableVertexAttribArray(mPositionAttribLocation);
	glVertexAttribPointer(mPositionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
	glEnableVertexAttribArray(mColorAttribLocation);
	glVertexAttribPointer(mColorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

	MathHelper::Matrix4 modelMatrix = MathHelper::MoveModelMatrix(0, -0.05f);
	glUniformMatrix4fv(mModelUniformLocation, 1, GL_FALSE, &(modelMatrix.m[0][0]));

	MathHelper::Matrix4 viewMatrix = MathHelper::SimpleViewMatrix();
	glUniformMatrix4fv(mViewUniformLocation, 1, GL_FALSE, &(viewMatrix.m[0][0]));

	MathHelper::Matrix4 projectionMatrix = MathHelper::SimpleProjectionMatrix(float(mWindowWidth) / float(mWindowHeight));
	glUniformMatrix4fv(mProjUniformLocation, 1, GL_FALSE, &(projectionMatrix.m[0][0]));

	glDrawArrays(GL_POINTS, 0, 361);

	if (click) {
		down -= 0.03f;
		click = false;
	}
	else
		down += 0.02f;
}

void Renderer::Draw()
{

	const GLfloat circleRadius = 0.05f;
	const GLfloat circleXPos = -0.5f;

	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mProgram == 0)
		return;

	glUseProgram(mProgram);

	if (failFlag || // do not draw if failFlag is true;
		(  -down + circleRadius >= 1.0f - randomHeight
	    || -down - circleRadius <= 1.0f - randomHeight - 0.5f)
		&& !(1.0f - xMoveStep + 0.5f <= circleXPos - circleRadius)
		&& !(1.0f - xMoveStep        >= circleXPos + circleRadius)) {
		failFlag = true;
		xMoveStep = 0.0f;
		down = 0.0f;
		LoadFail();
		return;
	}

	LoadCircle(circleXPos, -down, 0.0f, circleRadius);
	LoadColumn(0.0f, 0.0f);
}

void Renderer::UpdateWindowSize(GLsizei width, GLsizei height)
{
	glViewport(0, 0, width, height);
	mWindowWidth = width;
	mWindowHeight = height;
}
