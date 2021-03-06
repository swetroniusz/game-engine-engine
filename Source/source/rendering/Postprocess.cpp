#include <rendering/Postprocess.h>
#include "../SMAA/AreaTex.h"
#include "../SMAA/SearchTex.h"
#include <rendering/Mesh.h>
#include <rendering/RenderInfo.h>
#include <rendering/RenderToolbox.h>
#include <random>

namespace GEE
{
	using namespace GEE_FB;

	Postprocess::Postprocess() :
		FrameIndex(0),
		RenderHandle(nullptr)
	{
	}

	void Postprocess::Init(GameManager* gameHandle, glm::uvec2 resolution)
	{
		RenderHandle = gameHandle->GetRenderEngineHandle();

		QuadShader = RenderHandle->AddShader(ShaderLoader::LoadShaders("Quad", "Shaders/quad.vs", "Shaders/quad.fs"));
		QuadShader->Use();
		QuadShader->Uniform1i("tex", 0);
	}

	unsigned int Postprocess::GetFrameIndex()
	{
		return FrameIndex;
	}

	glm::mat4 Postprocess::GetJitterMat(const GameSettings::VideoSettings& usedSettings, int optionalIndex)
	{
		glm::vec2 jitter(0.0f);

		switch (usedSettings.AAType)
		{
		case AA_NONE:
		case AA_SMAA1X:
		default:
			return glm::mat4(1.0f);
		case AA_SMAAT2X:
			glm::vec2 jitters[] = {
				glm::vec2(-0.25f, 0.25f),
				glm::vec2(0.25f, -0.25f)
			};

			/*glm::vec2 jitters[] = {
	glm::vec2(10.0f, -10.0f),
	glm::vec2(-10.0f, 10.0f)
	};*/
			jitter = jitters[((optionalIndex >= 0) ? (optionalIndex) : (FrameIndex))];
			break;
		}

		jitter /= static_cast<glm::vec2>(usedSettings.Resolution) * 0.5f;
		return glm::translate(glm::mat4(1.0f), glm::vec3(jitter, 0.0f));
	}

	const Texture* Postprocess::GaussianBlur(PPToolbox<GaussianBlurToolbox> ppTb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* tex, int passes, unsigned int writeColorBuffer) const	//Implemented using ping-pong framebuffers (one gaussian blur pass is separable into two lower-cost passes) and linear filtering for performance
	{
		if (passes == 0)
			return tex;

		GaussianBlurToolbox& tb = ppTb.GetTb();

		tb.GaussianBlurShader->Use();
		glActiveTexture(GL_TEXTURE0);

		bool horizontal = true;

		for (int i = 0; i < passes; i++)
		{
			if (i == passes - 1)	//If the calling function specified the framebuffer to write the final result to, bind the given framebuffer for the last pass.
			{
				writeFramebuffer.Bind();
				writeFramebuffer.SetDrawBuffer(writeColorBuffer);
			}
			else	//For any pass that doesn't match given criteria, just switch the ping pong fbos
			{
				tb.BlurFramebuffers[horizontal]->Bind();
				const Texture* bindTex = (i == 0) ? (tex) : (tb.BlurFramebuffers[!horizontal]->GetColorBuffer(0).get());
				bindTex->Bind();
			}
			tb.GaussianBlurShader->Uniform1i("horizontal", horizontal);
			ppTb.RenderFullscreenQuad(tb.GaussianBlurShader, false);

			horizontal = !horizontal;
		}

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		tb.BlurFramebuffers[!horizontal]->GetColorBuffer(0).get();
		return writeFramebuffer.GetColorBuffer(writeColorBuffer).get();
	}

	const Texture* Postprocess::SSAOPass(RenderInfo& info, const Texture* gPosition, const Texture* gNormal)
	{
		SSAOToolbox* tb;
		if (!(tb = info.TbCollection.GetTb<SSAOToolbox>()))
		{
			std::cerr << "ERROR! Attempted to render SSAO image without calling the setup\n";
			return 0;
		}

		tb->SSAOFb->Bind();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		gPosition->Bind(0);
		gNormal->Bind(1);
		tb->SSAONoiseTex->Bind(2);

		tb->SSAOShader->Use();
		tb->SSAOShader->UniformMatrix4fv("view", info.view);
		tb->SSAOShader->UniformMatrix4fv("projection", info.projection);

		RenderFullscreenQuad(info, tb->SSAOShader, false);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);

		GaussianBlur(GetPPToolbox<GaussianBlurToolbox>(info.TbCollection), *tb->SSAOFb, nullptr, tb->SSAOFb->GetColorBuffer(0).get(), 4);
		return tb->SSAOFb->GetColorBuffer(0).get();
	}

	const Texture* Postprocess::SMAAPass(PPToolbox<SMAAToolbox> ppTb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* depthTex, const Texture* previousColorTex, const Texture* velocityTex, unsigned int writeColorBuffer, bool bT2x) const
	{
		SMAAToolbox& tb = ppTb.GetTb();
		tb.SMAAFb->Bind();

		/////////////////1. Edge detection
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		tb.SMAAShaders[0]->Use();
		colorTex->Bind(0);
		depthTex->Bind(1);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[0]);

		///////////////2. Weight blending
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		tb.SMAAShaders[1]->Use();
		if (bT2x)
			tb.SMAAShaders[1]->Uniform4fv("ssIndices", (FrameIndex == 0) ? (glm::vec4(1, 1, 1, 0)) : (glm::vec4(2, 2, 2, 0)));

		tb.SMAAFb->GetColorBuffer(0)->Bind(0);	//bind edges texture to slot 0
		tb.SMAAAreaTex->Bind(1);
		tb.SMAASearchTex->Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[1]);

		///////////////3. Neighborhood blending
		if (bT2x)
		{
			glDrawBuffer(GL_COLOR_ATTACHMENT2);
		}
		else
		{
			writeFramebuffer.Bind(true, viewport);
			writeFramebuffer.SetDrawBuffer(writeColorBuffer);
		}

		//glEnable(GL_FRAMEBUFFER_SRGB);

		tb.SMAAShaders[2]->Use();

		colorTex->Bind(0);
		tb.SMAAFb->GetColorBuffer(1)->Bind(1); //bind weights texture to slot 0

		if (velocityTex)
		{
			velocityTex->Bind(2);
		}

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[2]);
		//glDisable(GL_FRAMEBUFFER_SRGB);

		if (!bT2x)
			return nullptr;
		if (!velocityTex)
			return nullptr;

		///////////////4. Temporal resolve (optional)
		writeFramebuffer.Bind(true);
		writeFramebuffer.SetDrawBuffer(writeColorBuffer);

		tb.SMAAShaders[3]->Use();

		tb.SMAAFb->GetColorBuffer(2)->Bind(0);
		previousColorTex->Bind(1);
		velocityTex->Bind(2);

		ppTb.RenderFullscreenQuad(tb.SMAAShaders[3]);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		return writeFramebuffer.GetColorBuffer(writeColorBuffer).get();
	}

	const Texture* Postprocess::TonemapGammaPass(PPToolbox<ComposedImageStorageToolbox> tb, const GEE_FB::Framebuffer& writeFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* blurTex) const
	{
		writeFramebuffer.Bind(true, viewport);
		writeFramebuffer.SetDrawBuffer(0);

		glDisable(GL_DEPTH_TEST);
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);

		tb.GetTb().TonemapGammaShader->Use();

		colorTex->Bind(0);
		if (blurTex)
			blurTex->Bind(1);
		tb.GetTb().TonemapGammaShader->Uniform1i("HDRbuffer", 0);

		tb.RenderFullscreenQuad(tb.GetTb().TonemapGammaShader);

		return writeFramebuffer.GetColorBuffer(0).get();
	}

	void Postprocess::Render(RenderToolboxCollection& tbCollection, const GEE_FB::Framebuffer& finalFramebuffer, const Viewport* viewport, const Texture* colorTex, const Texture* blurTex, const Texture* depthTex, const Texture* velocityTex) const
	{
		const GameSettings::VideoSettings& settings = tbCollection.GetSettings();

		if (settings.bBloom && blurTex != 0)
			blurTex = GaussianBlur(GetPPToolbox<GaussianBlurToolbox>(tbCollection), *tbCollection.GetTb<GaussianBlurToolbox>()->BlurFramebuffers[0], nullptr, blurTex, 10);

		if (settings.AAType == AntiAliasingType::AA_SMAA1X)
		{
			const Texture* compositedTex = TonemapGammaPass(GetPPToolbox<ComposedImageStorageToolbox>(tbCollection), *tbCollection.GetTb<ComposedImageStorageToolbox>()->ComposedImageFb, nullptr, colorTex, blurTex);
			SMAAPass(GetPPToolbox<SMAAToolbox>(tbCollection), finalFramebuffer, viewport, compositedTex, depthTex, nullptr, nullptr);
		}
		else if (settings.AAType == AntiAliasingType::AA_SMAAT2X && velocityTex != 0)
		{
			PrevFrameStorageToolbox* storageTb = tbCollection.GetTb<PrevFrameStorageToolbox>();
			const Texture* compositedTex = TonemapGammaPass(GetPPToolbox<ComposedImageStorageToolbox>(tbCollection), *tbCollection.GetTb<ComposedImageStorageToolbox>()->ComposedImageFb, nullptr, colorTex, blurTex);
			unsigned int previousFrameIndex = (static_cast<int>(FrameIndex) - 1) % storageTb->StorageFb->GetNumberOfColorBuffers();
			const Texture* SMAAresult = SMAAPass(GetPPToolbox<SMAAToolbox>(tbCollection), *storageTb->StorageFb, nullptr, compositedTex, depthTex, storageTb->StorageFb->GetColorBuffer(previousFrameIndex).get(), velocityTex, FrameIndex, true);

			finalFramebuffer.Bind(true, viewport);
			finalFramebuffer.SetDrawBuffer(0);
			glEnable(GL_FRAMEBUFFER_SRGB);
			QuadShader->Use();

			SMAAresult->Bind(0);

			RenderFullscreenQuad(tbCollection);
			glDisable(GL_FRAMEBUFFER_SRGB);

			FrameIndex = (FrameIndex + 1) % storageTb->StorageFb->GetNumberOfColorBuffers();
		}
		else
			TonemapGammaPass(GetPPToolbox<ComposedImageStorageToolbox>(tbCollection), finalFramebuffer, viewport, colorTex, blurTex);
	}

	void Postprocess::Dispose()
	{
		/*if (SSAOFramebuffer)
			;// SSAOFramebuffer->Dispose(true);
		if (StorageFramebuffer)
			StorageFramebuffer->Dispose(true);
		if (SMAAFramebuffer)
			SMAAFramebuffer->Dispose(true);*/
			//TonemapGammaFramebuffer.Dispose(true);
		for (int i = 0; i < 2; i++)
			;//RenderHandle->GetCurrentTbCollection()->GetTb<GaussianBlurToolbox>()->BlurFramebuffers[i]->Dispose(true);
	}
}