#include <scene/LightComponent.h>
#include <rendering/Material.h>
#include <scene/UIInputBoxActor.h>
#include <UI/UICanvasActor.h>

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>

namespace GEE
{
	LightComponent::LightComponent(Actor& actor, Component* parentComp, std::string name, LightType type, unsigned int index, unsigned int shadowNr, float far, glm::mat4 projection, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, glm::vec3 settings) :
		Component(actor, parentComp, name, Transform()),
		Type(type),
		Ambient(amb),
		Diffuse(diff),
		Specular(spec),
		Attenuation(settings.x),
		ShadowBias(0.0f),
		bShadowMapCullFronts(true),
		LightIndex(index),
		ShadowMapNr(shadowNr),
		Far(far),
		Projection(projection),
		DirtyFlag(true),
		bHasValidShadowMap(false)
	{
		CutOff = glm::cos(glm::radians(settings.y));
		OuterCutOff = glm::cos(glm::radians(settings.z));

		std::cout << "WAZNE! ASSIGNING LIGHT INDEX " << index << "\n";

		Scene.GetRenderData()->AddLight(*this);

		TransformDirtyFlagIndex = ComponentTransform.AddDirtyFlag();
	}

	LightComponent::LightComponent(LightComponent&& comp) :
		Component(std::move(comp)),
		Type(comp.Type),
		Ambient(comp.Ambient),
		Diffuse(comp.Diffuse),
		Specular(comp.Specular),
		Attenuation(comp.Attenuation),
		ShadowBias(comp.ShadowBias),
		bShadowMapCullFronts(comp.bShadowMapCullFronts),
		CutOff(comp.CutOff),
		OuterCutOff(comp.OuterCutOff),
		LightIndex(comp.LightIndex),
		Far(comp.Far),
		Projection(comp.Projection),
		DirtyFlag(comp.DirtyFlag),
		ShadowMapNr(comp.ShadowMapNr),
		bHasValidShadowMap(comp.bHasValidShadowMap)
	{
		TransformDirtyFlagIndex = ComponentTransform.AddDirtyFlag();
		Scene.GetRenderData()->AddLight(*this);
	}

	LightType LightComponent::GetType() const
	{
		return Type;
	}

	float LightComponent::GetFar() const
	{
		return Far;
	}

	unsigned int LightComponent::GetLightIndex() const
	{
		return LightIndex;
	}

	unsigned int LightComponent::GetShadowMapNr() const
	{
		return ShadowMapNr;
	}

	glm::mat4 LightComponent::GetProjection() const
	{
		return Projection;
	}

	EngineBasicShape LightComponent::GetVolumeType() const
	{
		switch (Type)
		{
		case SPOT:
			if (Ambient == glm::vec3(0.0f))
				return EngineBasicShape::CONE;
		case POINT:
			return EngineBasicShape::SPHERE;
		case DIRECTIONAL:
			return EngineBasicShape::QUAD;
		default:
			std::cerr << "ERROR! Unknown light type: " << (int)Type << ".\n";
			return EngineBasicShape::SPHERE;
		}
	}

	Shader* LightComponent::GetRenderShader(const RenderToolboxCollection& renderCol) const
	{
		return GameHandle->GetRenderEngineHandle()->GetLightShader(renderCol, Type);
	}

	bool LightComponent::HasValidShadowMap() const
	{
		return bHasValidShadowMap;
	}

	void LightComponent::MarkValidShadowMap()
	{
		bHasValidShadowMap = true;
	}

	bool LightComponent::ShouldCullFrontsForShadowMap() const
	{
		return bShadowMapCullFronts;
	}

	void LightComponent::InvalidateCache()
	{
		DirtyFlag = true;
		bHasValidShadowMap = false;
		ComponentTransform.FlagMyDirtiness();
	}

	void LightComponent::CalculateLightRadius()
	{
		if (Type == LightType::DIRECTIONAL)
			return;

		DirtyFlag = true;

		///////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////

		float constant = 1.0f;
		float linear = 0.0f;
		float quadratic = Attenuation;
		float lightMax = glm::max(glm::max(Diffuse.r, Diffuse.g), Diffuse.b);

		float radius = 10.0f;
		if (Attenuation > 0.0f)
			radius = (-linear + glm::sqrt(linear * linear - 4.0f * quadratic * (constant - lightMax * (256.0f / 5.0f)))) / (2.0f * quadratic);

		Far = radius;
		Projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, Far);

		ComponentTransform.SetScale(glm::vec3(radius));

		///////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////

		if (Type == LightType::SPOT && Ambient == glm::vec3(0.0f))	//If we are a spotlight and we don't emit any ambient light, we can set the light volume to a cone instead of a sphere - more sweet FPS
		{
			float scale = radius * glm::tan(glm::acos(OuterCutOff));	//radius == height in our code
			ComponentTransform.SetScale(glm::vec3(scale, scale, radius));
		}
	}

	void LightComponent::SetAdditionalData(glm::vec3 data)
	{
		float prevCutOff = CutOff, prevOuterCutOff = OuterCutOff;

		SetAttenuation(data.x);
		CutOff = glm::cos(glm::radians(data.y));
		OuterCutOff = glm::cos(glm::radians(data.z));

		DirtyFlag = true;
		if (prevCutOff != CutOff || prevOuterCutOff != OuterCutOff)
			bHasValidShadowMap = false;
	}

	void LightComponent::SetAttenuation(float attenuation)
	{
		if (Attenuation == attenuation)
			return;

		Attenuation = attenuation;
		DirtyFlag = true;
		CalculateLightRadius();
	}

	void LightComponent::SetCutOff(float cutoff)
	{
		CutOff = cutoff;
		DirtyFlag = true;
	}

	void LightComponent::SetOuterCutOff(float outercutoff)
	{
		OuterCutOff = outercutoff;
		CalculateLightRadius();
	}

	void LightComponent::SetShadowBias(float bias)
	{
		ShadowBias = bias;
		DirtyFlag = true;
	}

	void LightComponent::SetType(LightType type)
	{
		if (Type != type)
			bHasValidShadowMap = false;

		Type = type;
		DirtyFlag = true;
	}

	void LightComponent::SetIndex(unsigned int index)
	{
		if (ShadowMapNr != index)
			bHasValidShadowMap = false;
		LightIndex = index;
		ShadowMapNr = index;
		DirtyFlag = true;
	}

	void LightComponent::UpdateUBOData(UniformBuffer* lightsUBO, size_t offset)
	{
		if (offset != -1)
			lightsUBO->offsetCache = offset;

		const Transform& worldTransform = ComponentTransform.GetWorldTransform();
		bool transformDirtyFlag = ComponentTransform.GetDirtyFlag(TransformDirtyFlagIndex);

		if (transformDirtyFlag)
		{
			switch (Type)
			{
			case LightType::DIRECTIONAL:
				lightsUBO->offsetCache += sizeof(glm::vec4);
				lightsUBO->SubData4fv(worldTransform.GetFrontVec(), lightsUBO->offsetCache); break;
			case LightType::POINT:
				lightsUBO->SubData4fv(worldTransform.Pos(), lightsUBO->offsetCache);
				lightsUBO->offsetCache += sizeof(glm::vec4); break;
			case LightType::SPOT:
				lightsUBO->SubData4fv(worldTransform.Pos(), lightsUBO->offsetCache);
				lightsUBO->SubData4fv(worldTransform.GetFrontVec(), lightsUBO->offsetCache); break;
			}
		}
		else
			lightsUBO->offsetCache += sizeof(glm::vec4) * 2;

		if (DirtyFlag)
		{
			lightsUBO->SubData4fv(std::vector <glm::vec4> {Vec4f(Ambient, ShadowBias), Vec4f(Diffuse, 0.0f), Vec4f(Specular, 0.0f)}, lightsUBO->offsetCache);
			float additionalData[3] = { Attenuation, CutOff, OuterCutOff };
			lightsUBO->SubData(12, additionalData, lightsUBO->offsetCache);
			lightsUBO->SubData1f(Type, lightsUBO->offsetCache);
			lightsUBO->SubData1f(ShadowMapNr, lightsUBO->offsetCache);
			lightsUBO->SubData1f(Far, lightsUBO->offsetCache);

			DirtyFlag = false;
		}
		else
			lightsUBO->offsetCache += 72;

		if (transformDirtyFlag)
			lightsUBO->SubDataMatrix4fv(Projection * worldTransform.GetViewMatrix(), lightsUBO->offsetCache + 8);
		else
			lightsUBO->offsetCache += sizeof(glm::mat4);

		lightsUBO->PadOffset();
	}

	glm::vec3& LightComponent::operator[](unsigned int i)
	{
		DirtyFlag = true;
		switch (i)
		{
		case 0:
			return Ambient;
		case 1:
			return Diffuse;
		case 2:
			return Specular;
		}

		return Ambient;
	}

	MaterialInstance LightComponent::GetDebugMatInst(EditorIconState state)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_LightComponent", "EditorAssets/lightcomponent_icon.png");
		return Component::GetDebugMatInst(state);
	}

	void LightComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Component::GetEditorDescription(descBuilder);

		descBuilder.AddField("Ambient").GetTemplates().VecInput(Ambient);
		descBuilder.AddField("Diffuse").GetTemplates().VecInput(Diffuse);
		descBuilder.AddField("Specular").GetTemplates().VecInput(Specular);
		UIInputBoxActor& attenuationInputBox = descBuilder.AddField("Attenuation").CreateChild<UIInputBoxActor>("Attenuation");
		attenuationInputBox.SetOnInputFunc([this](float val) { SetAttenuation(val); }, [this]() { return Attenuation; }, true);
		UIInputBoxActor& cutOffInputBox = descBuilder.AddField("Cutoff angle").CreateChild<UIInputBoxActor>("CutoffAngle");
		cutOffInputBox.SetOnInputFunc([this](float val) { SetCutOff(glm::cos(glm::radians(val))); }, [this]() { return glm::degrees(glm::acos(CutOff)); }, true);
		UIInputBoxActor& outerCutOffInputBox = descBuilder.AddField("Outer cutoff angle").CreateChild<UIInputBoxActor>("OuterCutoffAngle");
		outerCutOffInputBox.SetOnInputFunc([this](float val) { SetOuterCutOff(glm::cos(glm::radians(val))); }, [this]() { return glm::degrees(glm::acos(OuterCutOff)); }, true);
		UIInputBoxActor& shadowBiasInputBox = descBuilder.AddField("Shadow bias").CreateChild<UIInputBoxActor>("ShadowBias");
		shadowBiasInputBox.SetOnInputFunc([this](float val) { SetShadowBias(val); }, [this]() -> float { return ShadowBias; });
		descBuilder.AddField("Cull fronts in shadow maps").GetTemplates().TickBox(bShadowMapCullFronts);


		UICanvasField& typeField = descBuilder.AddField("Type");
		const std::string types[] = { "Directional", "Point", "Spot" };
		for (int i = 0; i < 3; i++)
			typeField.CreateChild<UIButtonActor>(types[i] + "TypeButton", types[i], [this, i]() {SetType(static_cast<LightType>(i)); CalculateLightRadius(); }).GetTransform()->Move(glm::vec2(static_cast<float>(i) * 2.0f, 0.0f));
	}

	LightComponent::~LightComponent()
	{
		Scene.GetRenderData()->EraseLight(*this);
	}

	LightVolume::LightVolume(const LightComponent& lightCompPtr) :
		LightCompPtr(&lightCompPtr)
	{
	}

	EngineBasicShape LightVolume::GetShape() const
	{
		return LightCompPtr->GetVolumeType();
	}

	Transform LightVolume::GetRenderTransform() const
	{
		Transform lightTransform = LightCompPtr->GetTransform().GetWorldTransform();
		lightTransform.SetScale(LightCompPtr->GetTransform().Scale());

		return lightTransform;
	}

	Shader* LightVolume::GetRenderShader(const RenderToolboxCollection& renderCol) const
	{
		return LightCompPtr->GetRenderShader(renderCol);
	}

	void LightVolume::SetupRenderUniforms(const Shader& shader) const
	{
		shader.Uniform1i("lightIndex", LightCompPtr->GetLightIndex());
	}



	LightType toLightType(std::string str)
	{
		if (str == "direction")
			return LightType::DIRECTIONAL;
		if (str == "spot")
			return LightType::SPOT;
		if (str != "point")
			std::cerr << str << " is not a known light type.\n";
		return LightType::POINT;
	}

}