#pragma once
#include "Texture.h"
#include <glm/glm.hpp>
#include <vector>
#include <string> 
#include <memory>

//TODO: Remont kapitalny calego systemu framebufferow
//****	Wyjeb te 3 klasy i rozbij je na funkcje
//****	Wyjeb funkcje Load z Framebuffer i rozbij to na: SetColorBuffers(vec<Texture>), AttachColorBuffer(Texture), SetDepthBuffer(Texture*), SetAttachments(vec<Texture>, Texture*) (pamietaj o glDrawBuffers zalezne od FBO!!!), argumenty tych funkcji to Texture zamiast tych zjebanych spaghetti attachment ja pierdole

namespace GEE_FB
{
	class FramebufferAttachment : public NamedTexture
	{
	protected:
		GLenum AttachmentEnum;

	public:
		FramebufferAttachment(NamedTexture tex = NamedTexture(Texture()), GLenum attachmentEnum = GL_ZERO);
		bool WasCreated();
		GLenum GetAttachmentEnum();
		void SetAttachmentEnum(GLenum);
		void BindToFramebuffer(GLenum target, GLenum attachment);
	};

	class Framebuffer
	{
		unsigned int FBO;
		glm::uvec2 RenderSize;
	public:
		std::vector <std::shared_ptr<FramebufferAttachment>> ColorBuffers;
		std::shared_ptr<FramebufferAttachment> DepthBuffer;
	public:
		Framebuffer();

		bool IsLoaded() const;
		unsigned int GetFBO() const;
		glm::uvec2 GetSize() const;
		std::shared_ptr<FramebufferAttachment> GetColorBuffer(unsigned int index) const;
		std::shared_ptr<FramebufferAttachment> GetColorBuffer(std::string name) const;
		unsigned int GetNumberOfColorBuffers() const;
		void SetAttachments(glm::uvec2 size = glm::uvec2(0), std::shared_ptr<FramebufferAttachment> colorBuffer = nullptr, std::shared_ptr<FramebufferAttachment> depthBuffer = nullptr, unsigned int samples = 0);
		void SetAttachments(glm::uvec2 size, std::vector<std::shared_ptr<FramebufferAttachment>> colorBuffers, std::shared_ptr<FramebufferAttachment> depthBuffer = nullptr, unsigned int samples = 0);
		void SetAttachments(glm::uvec2 size, const FramebufferAttachment& colorBuffer, FramebufferAttachment* depthBuffer = nullptr, unsigned int samples = 0);
		void SetAttachments(glm::uvec2 size, const std::vector<FramebufferAttachment>& colorBuffers, FramebufferAttachment* depthBuffer = nullptr, unsigned int samples = 0);
		void BlitToFBO(unsigned int, int = 1);
		void Bind(bool changeViewportSize = true) const;
		void SetDrawBuffers() const;
		void Dispose(bool disposeBuffers = false);
	};

	bool ContainsAlphaChannel(GLenum internalformat);
	bool ContainsStencil(GLenum internalformat);

	std::shared_ptr<FramebufferAttachment> reserveColorBuffer(glm::uvec2 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedColorBuffer");
	std::shared_ptr<FramebufferAttachment> reserveColorBuffer(glm::uvec3 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedColorBuffer");
	std::shared_ptr<FramebufferAttachment> reserveDepthBuffer(glm::uvec2 size, GLenum internalformat = GL_RGB, GLenum type = GL_UNSIGNED_BYTE, GLenum magFilter = GL_NEAREST, GLenum minFilter = GL_NEAREST, GLenum texType = GL_TEXTURE_2D, unsigned int samples = 0, std::string texName = "undefinedDepthBuffer");
}
std::string debugFramebuffer();