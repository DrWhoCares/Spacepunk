// Framebuffer.cpp

#include "Framebuffer.hpp"
#include "Engine.hpp"

Framebuffer::Framebuffer(const char* _name) : Asset(_name) {
	loaded = true;
}

Framebuffer::~Framebuffer() {
	term();
}

void Framebuffer::init(Uint32 _width, Uint32 _height) {
	if (fbo) {
		return;
	}

	width = _width;
	height = _height;

	glActiveTexture(GL_TEXTURE0);

	// Create the color texture
	glGenTextures(1, &color);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA32F, width, height, false);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	// Create the depth texture
	glGenTextures(1, &depth);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depth);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_DEPTH32F_STENCIL8, width, height, false);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	// Create the FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, color, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depth, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// check fbo status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		mainEngine->fmsg(Engine::MSG_ERROR, "failed to create framebuffer");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::term() {
	if (fbo) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (color) {
		glDeleteTextures(1, &color);
		color = 0;
	}
	if (depth) {
		glDeleteTextures(1, &depth);
		depth = 0;
	}
	if (stencil) {
		glDeleteTextures(1, &stencil);
		stencil = 0;
	}
}

void Framebuffer::bindForWriting() {
	if (!fbo)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, color, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depth, 0);
	glViewport(0, 0, width, height);
}

void Framebuffer::bindForReading(GLenum textureUnit, GLenum attachment) const {
	glActiveTexture(textureUnit);
	if (color && attachment == GL_COLOR_ATTACHMENT0) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color);
	}
	else if (depth && attachment == GL_DEPTH_ATTACHMENT) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depth);
	}
}