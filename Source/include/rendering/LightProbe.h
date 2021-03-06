#pragma once
#include "Texture.h"
#include <game/GameManager.h>
#include <math/Transform.h>
#include "RenderableVolume.h"
namespace GEE
{
	struct LightProbeTextureArrays
	{
		Texture IrradianceMapArr;
		Texture PrefilterMapArr;
		Texture BRDFLut;

		unsigned int NextLightProbeNr;

		LightProbeTextureArrays();
	};

	/*class LightProbe		//DEPRECATED
	{
	protected:
		GameSceneRenderData* SceneRenderData;
		RenderEngineManager* RenderHandle;
	public:
		unsigned int ProbeNr;
		Texture EnvironmentMap;

		LightProbe(GameSceneRenderData*);
		virtual EngineBasicShape GetShape() const;
		virtual Transform GetTransform() const;
		virtual Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const;
	};

	class LocalLightProbe : public LightProbe
	{
	public:
		EngineBasicShape Shape;
		Transform ProbeTransform;

		LocalLightProbe(GameSceneRenderData* sceneRenderData, Transform = Transform(), EngineBasicShape = EngineBasicShape::CUBE);
		virtual EngineBasicShape GetShape() const override;
		virtual Transform GetTransform() const override;
	};*/

	struct LightProbeLoader
	{
		static void LoadLightProbeFromFile(LightProbeComponent& probe, std::string path);
		static void ConvoluteLightProbe(const LightProbeComponent&, Texture* envMap = nullptr);	//Note: this light probe should have an environment map located at LightProbe::ProbeNr of texture array LightProbeTextureArrays::EnvironmentMapArr
		static void LoadLightProbeTextureArrays(GameSceneRenderData* sceneRenderData);	//Important: Call this function before loading any light probes!
	};

	class LightProbeVolume : public RenderableVolume
	{
		const LightProbeComponent* ProbePtr;
	public:
		LightProbeVolume(const LightProbeComponent& probe);
		virtual EngineBasicShape GetShape() const override;
		virtual Transform GetRenderTransform() const override;
		virtual Shader* GetRenderShader(const RenderToolboxCollection& renderCol) const override;
		virtual void SetupRenderUniforms(const Shader& shader) const override;
	};
}