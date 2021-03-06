#pragma once
#include "Postprocess.h"
#include <game/GameManager.h>
#include "RenderToolbox.h"
namespace GEE
{
	class LightProbe;
	struct LightProbeTextureArrays;
	class LocalLightProbe;
	class RenderableVolume;
	class BoneComponent;
	class TextComponent;
	class SkeletonBatch;

	class RenderEngine : public RenderEngineManager
	{
	public:
		RenderEngine(GameManager*);
		void Init(glm::uvec2 resolution);

		virtual const ShadingModel& GetShadingModel() override;
		virtual Mesh& GetBasicShapeMesh(EngineBasicShape) override;
		virtual Shader* GetLightShader(const RenderToolboxCollection& renderCol, LightType type) override;
		virtual RenderToolboxCollection* GetCurrentTbCollection() override;

		virtual std::vector<Material*> GetMaterials() override;

		/**
		 * @brief Add a new RenderToolboxCollection to the render engine to enable updating shadow maps automatically. By default, we load every toolbox that will be needed according to RenderToolboxCollection::Settings
		 * @param tbCollection: a constructed RenderToolboxCollection object containing the name and settings of it
		 * @param setupToolboxesAccordingToSettings: pass false as the second argument to disable loading any toolboxes
		 * @return a reference to the added RenderToolboxCollection
		*/
		virtual RenderToolboxCollection& AddRenderTbCollection(const RenderToolboxCollection& tbCollection, bool setupToolboxesAccordingToSettings = true) override;
		virtual Material* AddMaterial(std::shared_ptr<Material> material) override;
		virtual std::shared_ptr<Shader> AddShader(std::shared_ptr<Shader> shader, bool bForwardShader = false) override;

		void BindSkeletonBatch(SkeletonBatch* batch);
		void BindSkeletonBatch(GameSceneRenderData* sceneRenderData, unsigned int index);

		virtual void EraseRenderTbCollection(RenderToolboxCollection& tbCollection) override;
		virtual void EraseMaterial(Material&) override;

		virtual std::shared_ptr<Material> FindMaterial(std::string) override;
		virtual Shader* FindShader(std::string) override;


		void RenderShadowMaps(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData, std::vector<std::reference_wrapper<LightComponent>>);
		void RenderVolume(const RenderInfo&, EngineBasicShape, Shader&, const Transform* = nullptr);
		void RenderVolume(const RenderInfo&, RenderableVolume*, Shader* boundShader, bool shadedRender);
		void RenderVolumes(const RenderInfo&, const GEE_FB::Framebuffer& framebuffer, const std::vector<std::unique_ptr<RenderableVolume>>&, bool bIBLPass);
		void RenderLightProbes(GameSceneRenderData* sceneRenderData);
		void RenderRawScene(const RenderInfo& info, GameSceneRenderData* sceneRenderData, Shader* shader = nullptr);
		void RenderRawSceneUI(const RenderInfo& info, GameSceneRenderData* sceneRenderData);

		void RenderBoundInDebug(RenderInfo&, GLenum mode, GLint first, GLint count, glm::vec3 color = glm::vec3(1.0f));
		void PreLoopPass();

		void PrepareScene(RenderToolboxCollection& tbCollection, GameSceneRenderData* sceneRenderData);	//Call this method once per frame for each scene that will be rendered in order to prepare stuff like shadow maps
		void FullSceneRender(RenderInfo& info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer* framebuffer = nullptr, Viewport = Viewport(glm::vec2(0.0f), glm::vec2(0.0f)), bool clearMainFB = true, bool modifyForwardsDepthForUI = false);	//This method renders a scene with lighting and some postprocessing that improve the visual quality (e.g. SSAO, if enabled).

		void PrepareFrame();
		void PostFrame();	//This method completes the postprocessing after everything has been rendered. Call it at the end of your frame rendering function to minimize overhead. Algorithms like anti-aliasing don't need to run multiple times.
		virtual void RenderFrame(RenderInfo& info);

		void TestRenderCubemap(RenderInfo& info, GameSceneRenderData* sceneRenderData);
		virtual void RenderCubemapFromTexture(Texture targetTex, Texture tex, glm::uvec2 size, Shader& shader, int* layer = nullptr, int mipLevel = 0) override;
		virtual void RenderCubemapFromScene(RenderInfo info, GameSceneRenderData* sceneRenderData, GEE_FB::Framebuffer target, GEE_FB::FramebufferAttachment targetTex, GLenum attachmentType, Shader* shader = nullptr, int* layer = nullptr, bool fullRender = false) override;
		virtual void RenderText(const RenderInfo& info, const Font& font, std::string content, Transform t = Transform(), glm::vec3 color = glm::vec3(1.0f), Shader* shader = nullptr, bool convertFromPx = false, const std::pair<TextAlignment, TextAlignment> & = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM)) override; //Pass a shader if you do not want the default shader to be used.
		virtual void RenderStaticMesh(const RenderInfo& info, const MeshInstance& mesh, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) override; //Note: this function does not call the Use method of passed Shader. Do it manually.
		virtual void RenderStaticMeshes(const RenderInfo& info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, glm::mat4* lastFrameMVP = nullptr, Material* overrideMaterial = nullptr, bool billboard = false) override; //Note: this function does not call the Use method of passed Shader. Do it manually.
		virtual void RenderSkeletalMeshes(const RenderInfo& info, const std::vector<std::unique_ptr<MeshInstance>>& meshes, const Transform& transform, Shader* shader, SkeletonInfo& skelInfo, Material* overrideMaterial = nullptr) override; //Note: this function does not call the Use method of passed Shader. Do it manually

		void Dispose();

	private:
		virtual void AddSceneRenderDataPtr(GameSceneRenderData&) override;
		friend class Game;
		virtual void RemoveSceneRenderDataPtr(GameSceneRenderData&) override;
		void GenerateEngineObjects();
		void LoadInternalShaders();
		void Resize(glm::uvec2 resolution);

		GameManager* GameHandle;
		glm::uvec2 Resolution;

		Postprocess Postprocessing;

		std::vector <GameSceneRenderData*> ScenesRenderData;
	public:
		std::vector <std::shared_ptr <Material>> Materials;

		std::vector <std::shared_ptr <Shader>> Shaders;
		std::vector <std::shared_ptr <Shader>> UserShaders;
		std::vector <std::shared_ptr <Shader>> ForwardShaders;		//We store forward shaders in a different vector to attempt rendering the scene at forward render stage. Putting non-forward-rendering shaders here will reduce performance.
		//std::vector <std::shared_ptr <Shader>> SettingIndependentShaders;		//TODO: Put setting independent shaders here. When the user asks for a shader, search in CurrentTbCollection first. Then check here.
		std::vector <Shader*> LightShaders;		//Same with light shaders
		std::shared_ptr <Texture> EmptyTexture;

		const Mesh* BoundMesh;
		const Material* BoundMaterial;
		SkeletonBatch* BoundSkeletonBatch;

		std::vector <std::unique_ptr <RenderToolboxCollection>> RenderTbCollections;
		RenderToolboxCollection* CurrentTbCollection;

		glm::mat4 PreviousFrameView;

		struct CubemapRenderData
		{
			GEE_FB::Framebuffer DefaultFramebuffer;
			glm::mat4 DefaultV[6];
			glm::mat4 DefaultVP[6];
		} CubemapData;
	};

	glm::vec3 GetCubemapFront(unsigned int);
	glm::vec3 GetCubemapUp(unsigned int);
}