/*
 
 File: Shaders.h
 
 Abstract: Shader utilities for compiling, linking and validating shaders. 
 It is important to check the result status.
 
 Code based on Apple's OpenGLES sample 
*/

#ifndef SHADERS_H
#define SHADERS_H

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>


/* Shader Utilities */
GLint compileShader(GLuint *shader, GLenum type, const char* sources);
GLint linkProgram(GLuint prog);
GLint validateProgram(GLuint prog);
void destroyShaders(GLuint vertShader, GLuint fragShader, GLuint prog);

#endif /* SHADERS_H */
