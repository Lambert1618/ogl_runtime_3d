// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and 
// any modifications thereto. Any use, reproduction, disclosure, or 
// distribution of this software and related documentation without an express 
// license agreement from NVIDIA Corporation is strictly prohibited.
// 
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise Under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2012 NVIDIA Corporation. All rights reserved.

/*
** Copyright (c) 2013-2016 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/
/*
** This header is generated from the Khronos OpenGL / OpenGL ES XML
** API Registry. The current version of the Registry, generator scripts
** used to make the header, and the header can be found at
**   http://www.opengl.org/registry/
*/


/// @file glfuncdef.h
///       this is our internal OpenGL function header define

#ifndef __gl_func_def_h_
#define __gl_func_def_h_

#include <GLES2/gl2platform.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------
 * GL2 core functions.
 *-----------------------------------------------------------------------*/

GL_APICALL void         GL_APIENTRY glActiveTexture (GLenum texture);
GL_APICALL void         GL_APIENTRY glAttachShader (GLuint program, GLuint shader);
GL_APICALL void         GL_APIENTRY glBindAttribLocation (GLuint program, GLuint index, const char* name);
GL_APICALL void         GL_APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GL_APICALL void         GL_APIENTRY glBindFramebuffer (GLenum target, GLuint framebuffer);
GL_APICALL void         GL_APIENTRY glBindRenderbuffer (GLenum target, GLuint renderbuffer);
GL_APICALL void         GL_APIENTRY glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GL_APICALL void         GL_APIENTRY glBlendEquation ( GLenum mode );
GL_APICALL void         GL_APIENTRY glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha);
GL_APICALL void         GL_APIENTRY glBlendFuncSeparate (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
GL_APICALL void         GL_APIENTRY glBufferData (GLenum target, GLsizeiptr size, const void* data, GLenum usage);
GL_APICALL void         GL_APIENTRY glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
GL_APICALL GLenum       GL_APIENTRY glCheckFramebufferStatus (GLenum target);
GL_APICALL void         GL_APIENTRY glClearDepthf (GLclampf depth);
GL_APICALL void         GL_APIENTRY glCompileShader (GLuint shader);
GL_APICALL void         GL_APIENTRY glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
GL_APICALL void         GL_APIENTRY glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
GL_APICALL GLuint       GL_APIENTRY glCreateProgram (void);
GL_APICALL GLuint       GL_APIENTRY glCreateShader (GLenum type);
GL_APICALL void         GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint* buffers);
GL_APICALL void         GL_APIENTRY glDeleteFramebuffers (GLsizei n, const GLuint* framebuffers);
GL_APICALL void         GL_APIENTRY glDeleteProgram (GLuint program);
GL_APICALL void         GL_APIENTRY glDeleteRenderbuffers (GLsizei n, const GLuint* renderbuffers);
GL_APICALL void         GL_APIENTRY glDeleteShader (GLuint shader);
GL_APICALL void         GL_APIENTRY glDepthRangef (GLclampf zNear, GLclampf zFar);
GL_APICALL void         GL_APIENTRY glDetachShader (GLuint program, GLuint shader);
GL_APICALL void         GL_APIENTRY glDisableVertexAttribArray (GLuint index);
GL_APICALL void		    GL_APIENTRY glDrawBuffers( GLsizei n, const GLenum *bufs);
GL_APICALL void         GL_APIENTRY glEnableVertexAttribArray (GLuint index);
GL_APICALL void         GL_APIENTRY glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GL_APICALL void         GL_APIENTRY glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_APICALL void         GL_APIENTRY glGenBuffers (GLsizei n, GLuint* buffers);
GL_APICALL void         GL_APIENTRY glGenerateMipmap (GLenum target);
GL_APICALL void         GL_APIENTRY glGenFramebuffers (GLsizei n, GLuint* framebuffers);
GL_APICALL void         GL_APIENTRY glGenRenderbuffers (GLsizei n, GLuint* renderbuffers);
GL_APICALL void         GL_APIENTRY glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
GL_APICALL void         GL_APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
GL_APICALL void         GL_APIENTRY glGetAttachedShaders (GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
GL_APICALL int          GL_APIENTRY glGetAttribLocation (GLuint program, const char* name);
GL_APICALL void         GL_APIENTRY glGetBufferParameteriv (GLenum target, GLenum pname, GLint* params);
GL_APICALL void         GL_APIENTRY glGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint* params);
GL_APICALL void         GL_APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint* params);
GL_APICALL void         GL_APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
GL_APICALL void         GL_APIENTRY glGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint* params);
GL_APICALL void         GL_APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint* params);
GL_APICALL void         GL_APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
GL_APICALL void         GL_APIENTRY glGetShaderPrecisionFormat (GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
GL_APICALL void         GL_APIENTRY glGetShaderSource (GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
GL_APICALL void         GL_APIENTRY glGetUniformfv (GLuint program, GLint location, GLfloat* params);
GL_APICALL void         GL_APIENTRY glGetUniformiv (GLuint program, GLint location, GLint* params);
GL_APICALL int          GL_APIENTRY glGetUniformLocation (GLuint program, const char* name);
GL_APICALL void         GL_APIENTRY glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat* params);
GL_APICALL void         GL_APIENTRY glGetVertexAttribiv (GLuint index, GLenum pname, GLint* params);
GL_APICALL void         GL_APIENTRY glGetVertexAttribPointerv (GLuint index, GLenum pname, void** pointer);
GL_APICALL GLboolean    GL_APIENTRY glIsBuffer (GLuint buffer);
GL_APICALL GLboolean    GL_APIENTRY glIsFramebuffer (GLuint framebuffer);
GL_APICALL GLboolean    GL_APIENTRY glIsProgram (GLuint program);
GL_APICALL GLboolean    GL_APIENTRY glIsRenderbuffer (GLuint renderbuffer);
GL_APICALL GLboolean    GL_APIENTRY glIsShader (GLuint shader);
GL_APICALL void         GL_APIENTRY glLinkProgram (GLuint program);
GL_APICALL void         GL_APIENTRY glReleaseShaderCompiler (void);
GL_APICALL void         GL_APIENTRY glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GL_APICALL void         GL_APIENTRY glSampleCoverage (GLclampf value, GLboolean invert);
GL_APICALL void         GL_APIENTRY glShaderBinary (GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length);
GL_APICALL void         GL_APIENTRY glShaderSource (GLuint shader, GLsizei count, const char** string, const GLint* length);
GL_APICALL void         GL_APIENTRY glStencilFuncSeparate (GLenum face, GLenum func, GLint ref, GLuint mask);
GL_APICALL void         GL_APIENTRY glStencilMaskSeparate (GLenum face, GLuint mask);
GL_APICALL void         GL_APIENTRY glStencilOpSeparate (GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
GL_APICALL void         GL_APIENTRY glUniform1f (GLint location, GLfloat x);
GL_APICALL void         GL_APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat* v);
GL_APICALL void         GL_APIENTRY glUniform1i (GLint location, GLint x);
GL_APICALL void         GL_APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint* v);
GL_APICALL void         GL_APIENTRY glUniform2f (GLint location, GLfloat x, GLfloat y);
GL_APICALL void         GL_APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat* v);
GL_APICALL void         GL_APIENTRY glUniform2i (GLint location, GLint x, GLint y);
GL_APICALL void         GL_APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint* v);
GL_APICALL void         GL_APIENTRY glUniform3f (GLint location, GLfloat x, GLfloat y, GLfloat z);
GL_APICALL void         GL_APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat* v);
GL_APICALL void         GL_APIENTRY glUniform3i (GLint location, GLint x, GLint y, GLint z);
GL_APICALL void         GL_APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint* v);
GL_APICALL void         GL_APIENTRY glUniform4f (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_APICALL void         GL_APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat* v);
GL_APICALL void         GL_APIENTRY glUniform4i (GLint location, GLint x, GLint y, GLint z, GLint w);
GL_APICALL void         GL_APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint* v);
GL_APICALL void         GL_APIENTRY glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
GL_APICALL void         GL_APIENTRY glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
GL_APICALL void         GL_APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
GL_APICALL void         GL_APIENTRY glUseProgram (GLuint program);
GL_APICALL void         GL_APIENTRY glValidateProgram (GLuint program);
GL_APICALL void         GL_APIENTRY glVertexAttrib1f (GLuint indx, GLfloat x);
GL_APICALL void         GL_APIENTRY glVertexAttrib1fv (GLuint indx, const GLfloat* values);
GL_APICALL void         GL_APIENTRY glVertexAttrib2f (GLuint indx, GLfloat x, GLfloat y);
GL_APICALL void         GL_APIENTRY glVertexAttrib2fv (GLuint indx, const GLfloat* values);
GL_APICALL void         GL_APIENTRY glVertexAttrib3f (GLuint indx, GLfloat x, GLfloat y, GLfloat z);
GL_APICALL void         GL_APIENTRY glVertexAttrib3fv (GLuint indx, const GLfloat* values);
GL_APICALL void         GL_APIENTRY glVertexAttrib4f (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_APICALL void         GL_APIENTRY glVertexAttrib4fv (GLuint indx, const GLfloat* values);
GL_APICALL void         GL_APIENTRY glVertexAttribPointer (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);

/*-------------------------------------------------------------------------
 * GL3 core functions.
 *-----------------------------------------------------------------------*/

GL_APICALL void				GL_APIENTRY glBeginQuery (GLenum target, GLuint id);
GL_APICALL void		        GL_APIENTRY glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
GL_APICALL void		        GL_APIENTRY glBindFragDataLocation (GLuint program, GLuint colorNumber, const GLchar * name);
GL_APICALL void				GL_APIENTRY glBindVertexArray (GLuint array);
GL_APICALL void				GL_APIENTRY glBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
GL_APICALL GLenum			GL_APIENTRY glClientWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout);
GL_APICALL void				GL_APIENTRY glDeleteQueries (GLsizei n, const GLuint *ids);
GL_APICALL void				GL_APIENTRY glDeleteSync (GLsync sync);
GL_APICALL void				GL_APIENTRY glDeleteVertexArrays (GLsizei n, const GLuint *arrays);
GL_APICALL void				GL_APIENTRY glEndQuery (GLenum target);
GL_APICALL GLsync			GL_APIENTRY glFenceSync (GLenum condition, GLbitfield flags);
GL_APICALL void				GL_APIENTRY glFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
GL_APICALL void				GL_APIENTRY glGenQueries (GLsizei n, GLuint *ids); 
GL_APICALL void				GL_APIENTRY glGenVertexArrays (GLsizei n, GLuint *arrays);
GL_APICALL void				GL_APIENTRY glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
GL_APICALL void				GL_APIENTRY glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
GL_APICALL void				GL_APIENTRY glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
GL_APICALL void				GL_APIENTRY glGetQueryObjectuiv (GLuint id, GLenum pname, GLuint *params); 
GL_APICALL void				GL_APIENTRY glGetQueryObjectui64v (GLuint id, GLenum pname, GLuint64 *params);
GL_APICALL const GLubyte*	GL_APIENTRY glGetStringi (GLenum name, GLuint index);
GL_APICALL void				GL_APIENTRY glGetSynciv (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values); 
GL_APICALL GLuint		    GL_APIENTRY glGetUniformBlockIndex( GLuint program, const GLchar *uniformBlockName);
GL_APICALL GLvoid*			GL_APIENTRY glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
GL_APICALL void				GL_APIENTRY glQueryCounter (GLuint id, GLenum target);
GL_APICALL void				GL_APIENTRY glTexImage2DMultisample (GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
GL_APICALL void				GL_APIENTRY glTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
GL_APICALL void				GL_APIENTRY glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels); 
GL_APICALL void				GL_APIENTRY glUniformBlockBinding (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding); 
GL_APICALL GLboolean		GL_APIENTRY glUnmapBuffer (GLenum target);
GL_APICALL void				GL_APIENTRY glWaitSync (GLsync sync, GLbitfield flags, GLuint64 timeout);

/*-------------------------------------------------------------------------
 * GL4 core functions.
 *-----------------------------------------------------------------------*/

GL_APICALL void				GL_APIENTRY glActiveShaderProgram (GLuint pipeline, GLuint program); 
GL_APICALL void				GL_APIENTRY glBindImageTexture (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
GL_APICALL void				GL_APIENTRY glBindProgramPipeline (GLuint pipeline); 
GL_APICALL GLuint			GL_APIENTRY glCreateShaderProgramv (GLenum type, GLsizei count, const GLchar *const*strings);
GL_APICALL void				GL_APIENTRY glDeleteProgramPipelines (GLsizei n, const GLuint *pipelines); 
GL_APICALL void				GL_APIENTRY glDispatchCompute (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
GL_APICALL void				GL_APIENTRY glDispatchComputeIndirect (GLintptr indirect);
GL_APICALL void				GL_APIENTRY glDrawArraysIndirect (GLenum mode, const void *indirect);
GL_APICALL void				GL_APIENTRY glDrawElementsIndirect (GLenum mode, GLenum type, const void *indirect); 
GL_APICALL void				GL_APIENTRY glGenProgramPipelines (GLsizei n, GLuint *pipelines); 
GL_APICALL void				GL_APIENTRY glGetMultisamplefv (GLenum pname, GLuint index, GLfloat *val );
GL_APICALL void				GL_APIENTRY glGetProgramInterfaceiv (GLuint program, GLenum programInterface, GLenum pname, GLint *params);
GL_APICALL void				GL_APIENTRY glProgramParameteri (GLuint program, GLenum pname, GLint value);
GL_APICALL void				GL_APIENTRY glGetProgramPipelineInfoLog (GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog); 
GL_APICALL void				GL_APIENTRY glGetProgramPipelineiv (GLuint pipeline, GLenum pname, GLint *params); 
GL_APICALL GLuint			GL_APIENTRY glGetProgramResourceIndex (GLuint program, GLenum programInterface, const GLchar *name);
GL_APICALL void				GL_APIENTRY glGetProgramResourceiv (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);
GL_APICALL GLint			GL_APIENTRY glGetProgramResourceLocation (GLuint program, GLenum programInterface, const GLchar *name);
GL_APICALL GLint			GL_APIENTRY glGetProgramResourceLocationIndex (GLuint program, GLenum programInterface, const GLchar *name);
GL_APICALL void				GL_APIENTRY glGetProgramResourceName (GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name);
GL_APICALL void				GL_APIENTRY glMemoryBarrier (GLbitfield barriers);
GL_APICALL void				GL_APIENTRY glMinSampleShading (GLfloat value );
GL_APICALL void				GL_APIENTRY glPatchParameteri (GLenum pname, GLint value );
GL_APICALL void				GL_APIENTRY glPatchParameterfv (GLenum pname, GLint value );
GL_APICALL void				GL_APIENTRY glTexStorage2D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
GL_APICALL void				GL_APIENTRY glTexStorage2DMultisample (GLint target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
GL_APICALL void				GL_APIENTRY glProgramUniform1i (GLuint program, GLint location, GLint v0);
GL_APICALL void				GL_APIENTRY glProgramUniform2i (GLuint program, GLint location, GLint v0, GLint v1);
GL_APICALL void				GL_APIENTRY glProgramUniform3i (GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
GL_APICALL void				GL_APIENTRY glProgramUniform4i (GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GL_APICALL void				GL_APIENTRY glProgramUniform1ui (GLuint program, GLint location, GLuint v0);
GL_APICALL void				GL_APIENTRY glProgramUniform2ui (GLuint program, GLint location, GLuint v0, GLuint v1);
GL_APICALL void				GL_APIENTRY glProgramUniform3ui (GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
GL_APICALL void				GL_APIENTRY glProgramUniform4ui (GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
GL_APICALL void				GL_APIENTRY glProgramUniform1f (GLuint program, GLint location, GLfloat v0);
GL_APICALL void				GL_APIENTRY glProgramUniform2f (GLuint program, GLint location, GLfloat v0, GLfloat v1);
GL_APICALL void				GL_APIENTRY glProgramUniform3f (GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GL_APICALL void				GL_APIENTRY glProgramUniform4f (GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GL_APICALL void				GL_APIENTRY glProgramUniform1iv (GLuint program, GLint location, GLsizei count, const GLint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform2iv (GLuint program, GLint location, GLsizei count, const GLint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform3iv (GLuint program, GLint location, GLsizei count, const GLint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform4iv (GLuint program, GLint location, GLsizei count, const GLint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform1uiv (GLuint program, GLint location, GLsizei count, const GLuint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform2uiv (GLuint program, GLint location, GLsizei count, const GLuint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform3uiv (GLuint program, GLint location, GLsizei count, const GLuint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform4uiv (GLuint program, GLint location, GLsizei count, const GLuint *value);
GL_APICALL void				GL_APIENTRY glProgramUniform1fv (GLuint program, GLint location, GLsizei count, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glProgramUniform2fv (GLuint program, GLint location, GLsizei count, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glProgramUniform3fv (GLuint program, GLint location, GLsizei count, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glProgramUniform4fv (GLuint program, GLint location, GLsizei count, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glProgramUniformMatrix2fv (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glProgramUniformMatrix3fv (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glProgramUniformMatrix4fv (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glUseProgramStages (GLuint pipeline, GLbitfield stages, GLuint program); 
GL_APICALL void				GL_APIENTRY glValidateProgramPipeline (GLuint pipeline); 


/*-------------------------------------------------------------------------
 * GL4 NV extensions
 *-----------------------------------------------------------------------*/

GL_APICALL GLuint			GL_APIENTRY glGenPathsNV (GLsizei range);
GL_APICALL void				GL_APIENTRY glDeletePathsNV (GLuint path, GLsizei range);
GL_APICALL GLboolean		GL_APIENTRY glIsPathNV (GLuint path);
GL_APICALL void				GL_APIENTRY glPathCommandsNV (GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
GL_APICALL void				GL_APIENTRY glPathCoordsNV (GLuint path, GLsizei numCoords, GLenum coordType, const void *coords);
GL_APICALL void				GL_APIENTRY glPathSubCommandsNV (GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
GL_APICALL void				GL_APIENTRY glPathSubCoordsNV (GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords);
GL_APICALL void				GL_APIENTRY glPathStringNV (GLuint path, GLenum format, GLsizei length, const void *pathString);
GL_APICALL void				GL_APIENTRY glPathGlyphsNV (GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GL_APICALL void				GL_APIENTRY glPathGlyphRangeNV (GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GL_APICALL void				GL_APIENTRY glWeightPathsNV (GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights);
GL_APICALL void				GL_APIENTRY glCopyPathNV (GLuint resultPath, GLuint srcPath);
GL_APICALL void				GL_APIENTRY glInterpolatePathsNV (GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight);
GL_APICALL void				GL_APIENTRY glTransformPathNV (GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues);
GL_APICALL void				GL_APIENTRY glPathParameterivNV (GLuint path, GLenum pname, const GLint *value);
GL_APICALL void				GL_APIENTRY glPathParameteriNV (GLuint path, GLenum pname, GLint value);
GL_APICALL void				GL_APIENTRY glPathParameterfvNV (GLuint path, GLenum pname, const GLfloat *value);
GL_APICALL void				GL_APIENTRY glPathParameterfNV (GLuint path, GLenum pname, GLfloat value);
GL_APICALL void				GL_APIENTRY glPathDashArrayNV (GLuint path, GLsizei dashCount, const GLfloat *dashArray);
GL_APICALL void				GL_APIENTRY glPathStencilFuncNV (GLenum func, GLint ref, GLuint mask);
GL_APICALL void				GL_APIENTRY glPathStencilDepthOffsetNV (GLfloat factor, GLfloat units);
GL_APICALL void				GL_APIENTRY glStencilFillPathNV (GLuint path, GLenum fillMode, GLuint mask);
GL_APICALL void				GL_APIENTRY glStencilStrokePathNV (GLuint path, GLint reference, GLuint mask);
GL_APICALL void				GL_APIENTRY glStencilFillPathInstancedNV (GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues);
GL_APICALL void				GL_APIENTRY glStencilStrokePathInstancedNV (GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues);
GL_APICALL void				GL_APIENTRY glPathCoverDepthFuncNV (GLenum func);
GL_APICALL void				GL_APIENTRY glPathColorGenNV (GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs);
GL_APICALL void				GL_APIENTRY glPathTexGenNV (GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs);
GL_APICALL void				GL_APIENTRY glPathFogGenNV (GLenum genMode);
GL_APICALL void				GL_APIENTRY glCoverFillPathNV (GLuint path, GLenum coverMode);
GL_APICALL void				GL_APIENTRY glCoverStrokePathNV (GLuint path, GLenum coverMode);
GL_APICALL void				GL_APIENTRY glCoverFillPathInstancedNV (GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
GL_APICALL void				GL_APIENTRY glCoverStrokePathInstancedNV (GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
GL_APICALL void				GL_APIENTRY glGetPathParameterivNV (GLuint path, GLenum pname, GLint *value);
GL_APICALL void				GL_APIENTRY glGetPathParameterfvNV (GLuint path, GLenum pname, GLfloat *value);
GL_APICALL void				GL_APIENTRY glGetPathCommandsNV (GLuint path, GLubyte *commands);
GL_APICALL void				GL_APIENTRY glGetPathCoordsNV (GLuint path, GLfloat *coords);
GL_APICALL void				GL_APIENTRY glGetPathDashArrayNV (GLuint path, GLfloat *dashArray);
GL_APICALL void				GL_APIENTRY glGetPathMetricsNV (GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics);
GL_APICALL void				GL_APIENTRY glGetPathMetricRangeNV (GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics);
GL_APICALL void				GL_APIENTRY glGetPathSpacingNV (GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing);
GL_APICALL void				GL_APIENTRY glGetPathColorGenivNV (GLenum color, GLenum pname, GLint *value);
GL_APICALL void				GL_APIENTRY glGetPathColorGenfvNV (GLenum color, GLenum pname, GLfloat *value);
GL_APICALL void				GL_APIENTRY glGetPathTexGenivNV (GLenum texCoordSet, GLenum pname, GLint *value);
GL_APICALL void				GL_APIENTRY glGetPathTexGenfvNV (GLenum texCoordSet, GLenum pname, GLfloat *value);
GL_APICALL GLboolean		GL_APIENTRY glIsPointInFillPathNV (GLuint path, GLuint mask, GLfloat x, GLfloat y);
GL_APICALL GLboolean		GL_APIENTRY glIsPointInStrokePathNV (GLuint path, GLfloat x, GLfloat y);
GL_APICALL GLfloat			GL_APIENTRY glGetPathLengthNV (GLuint path, GLsizei startSegment, GLsizei numSegments);
GL_APICALL GLboolean		GL_APIENTRY glPointAlongPathNV (GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY);
GL_APICALL void				GL_APIENTRY glMatrixLoad3x2fNV (GLenum matrixMode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glMatrixLoad3x3fNV (GLenum matrixMode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glMatrixLoadTranspose3x3fNV (GLenum matrixMode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glMatrixMult3x2fNV (GLenum matrixMode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glMatrixMult3x3fNV (GLenum matrixMode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glMatrixMultTranspose3x3fNV (GLenum matrixMode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glStencilThenCoverFillPathNV (GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode);
GL_APICALL void				GL_APIENTRY glStencilThenCoverStrokePathNV (GLuint path, GLint reference, GLuint mask, GLenum coverMode);
GL_APICALL void				GL_APIENTRY glStencilThenCoverFillPathInstancedNV (GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
GL_APICALL void				GL_APIENTRY glStencilThenCoverStrokePathInstancedNV (GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
GL_APICALL GLenum			GL_APIENTRY glPathGlyphIndexRangeNV (GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2]);
GL_APICALL GLenum			GL_APIENTRY glPathGlyphIndexArrayNV (GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GL_APICALL GLenum			GL_APIENTRY glPathMemoryGlyphIndexArrayNV (GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GL_APICALL void				GL_APIENTRY glProgramPathFragmentInputGenNV (GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs);
GL_APICALL void				GL_APIENTRY glGetProgramResourcefvNV (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params); 
GL_APICALL void				GL_APIENTRY glMatrixLoadf (GLenum mode, const GLfloat *m);
GL_APICALL void				GL_APIENTRY glBlendBarrierNV( void );
GL_APICALL void				GL_APIENTRY glBlendParameteriNV( GLenum pname, GLint value );


#ifdef __cplusplus
}
#endif

#endif /* __gl_func_def_h_ */
