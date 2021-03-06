#include <scene/ModelComponent.h>
#include <game/GameScene.h>
#include <rendering/RenderInfo.h>
#include <UI/UICanvasActor.h>
#include <scene/UIInputBoxActor.h>
#include <rendering/Texture.h>
#include <scene/TextComponent.h>
#include <UI/UICanvasField.h>

#include <UI/UIListActor.h>
#include <scene/UIWindowActor.h>
#include <scene/LightProbeComponent.h>
#include <rendering/RenderToolbox.h>
#include <scene/CameraComponent.h>
#include <scene/Controller.h>

namespace GEE
{
	using namespace MeshSystem;

	ModelComponent::ModelComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, SkeletonInfo* info, Material* overrideMat) :
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp),
		LastFrameMVP(glm::mat4(1.0f)),
		SkelInfo(info),
		RenderAsBillboard(false)
	{
	}

	ModelComponent::ModelComponent(ModelComponent&& model) :
		RenderableComponent(std::move(model)),
		UIComponent(std::move(model)),
		MeshInstances(std::move(model.MeshInstances)),
		SkelInfo(model.SkelInfo),
		RenderAsBillboard(model.RenderAsBillboard),
		LastFrameMVP(model.LastFrameMVP)
	{
	}

	ModelComponent& ModelComponent::operator=(const ModelComponent& compT)
	{
		Component::operator=(compT);

		for (int i = 0; i < compT.GetMeshInstanceCount(); i++)
			AddMeshInst(MeshInstance(const_cast<Mesh&>(compT.GetMeshInstance(i).GetMesh()), const_cast<Material*>(compT.GetMeshInstance(i).GetMaterialPtr())));

		/*if (overrideMaterial)
			OverrideInstancesMaterial(overrideMaterial);
		else if (compT.GetOverrideMaterial())
			OverrideInstancesMaterial(meshNodeCast->GetOverrideMaterial());*/
		return *this;
	}

	void ModelComponent::OverrideInstancesMaterial(Material* overrideMat)
	{
		for (auto& it : MeshInstances)
			it->SetMaterial(overrideMat);
	}

	void ModelComponent::OverrideInstancesMaterialInstances(std::shared_ptr<MaterialInstance> overrideMatInst)
	{
		for (auto& it : MeshInstances)
			it->SetMaterialInst(overrideMatInst);
	}

	void ModelComponent::SetLastFrameMVP(const glm::mat4& lastMVP) const
	{
		LastFrameMVP = lastMVP;
	}

	void ModelComponent::SetSkeletonInfo(SkeletonInfo* info)
	{
		SkelInfo = info;
	}

	void ModelComponent::SetRenderAsBillboard(bool billboard)
	{
		RenderAsBillboard = billboard;
	}

	void ModelComponent::DRAWBATCH() const
	{
		if (SkelInfo)
		{
			;// SkelInfo->DRAWBATCH();
		}
	}

	int ModelComponent::GetMeshInstanceCount() const
	{
		return MeshInstances.size();
	}

	const MeshInstance& ModelComponent::GetMeshInstance(int index) const
	{
		return *MeshInstances[index];
	}

	const glm::mat4& ModelComponent::GetLastFrameMVP() const
	{
		return LastFrameMVP;
	}

	SkeletonInfo* ModelComponent::GetSkeletonInfo() const
	{
		return SkelInfo;
	}

	MeshInstance* ModelComponent::FindMeshInstance(const std::string& nodeName, const std::string& specificMeshName)
	{
		/*for (int i = 0; i < 2; i++)	//Search 2 times; first look at the specific names and then at node names.
		{
			std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return !str1.empty() && str1 == str2; };
			for (std::unique_ptr<MeshInstance>& it : MeshInstances)
				if ((i == 0 && (checkEqual(specificMeshName, it->GetMesh().GetLocalization().SpecificName)) || checkEqual(specificMeshName, it->GetMesh().GetLocalization().NodeName) ||
					(i == 1 && (checkEqual(nodeName, it->GetMesh().GetLocalization().SpecificName)) || checkEqual(nodeName, it->GetMesh().GetLocalization().NodeName))))
					return it.get();
		}*/

		if (nodeName.empty() && specificMeshName.empty())
			return nullptr;

		std::function<bool(const std::string&, const std::string&)> checkEqual = [](const std::string& str1, const std::string& str2) -> bool { return str1.empty() || str1 == str2; };
		for (std::unique_ptr<MeshInstance>& it : MeshInstances)
			if ((checkEqual(nodeName, it->GetMesh().GetLocalization().NodeName)) && checkEqual(specificMeshName, it->GetMesh().GetLocalization().SpecificName))
				return it.get();

		for (auto& it : Children)
		{
			ModelComponent* modelCast = dynamic_cast<ModelComponent*>(it.get());
			MeshInstance* meshInst = modelCast->FindMeshInstance(nodeName, specificMeshName);
			if (meshInst)
				return meshInst;
		}


		return nullptr;
	}

	void ModelComponent::AddMeshInst(const MeshInstance& meshInst)
	{
		MeshInstances.push_back(std::make_unique<MeshInstance>(meshInst));
	}

	void ModelComponent::Update(float deltaTime)
	{
		for (auto& it : MeshInstances)
			if (it->GetMaterialInst())
				it->GetMaterialInst()->Update(deltaTime);

		if (Name == "MeshPreviewModel")
			ComponentTransform.SetRotation((glm::mat3)glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)));
	}

	void ModelComponent::Render(const RenderInfo& info, Shader* shader)
	{
		if (GetHide())
			return;

		std::shared_ptr<AtlasMaterial> found = std::dynamic_pointer_cast<AtlasMaterial>(GameHandle->GetRenderEngineHandle()->FindMaterial("Kopec"));
		if (Name == "KOPEC")
		{
			OverrideInstancesMaterialInstances(std::make_shared<MaterialInstance>(*found, found->GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.2f, InterpolationType::LINEAR, true, AnimBehaviour::STOP, AnimBehaviour::REPEAT), 0.0f, 1.0f)));
			Name = "Kopec";
		}

		if (SkelInfo && SkelInfo->GetBoneCount() > 0)
			GameHandle->GetRenderEngineHandle()->RenderSkeletalMeshes(info, MeshInstances, GetTransform().GetWorldTransform(), shader, *SkelInfo);
		else
		{
			GameHandle->GetRenderEngineHandle()->RenderStaticMeshes((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), MeshInstances, GetTransform().GetWorldTransform(), shader, &LastFrameMVP, nullptr, RenderAsBillboard);
			if (CanvasPtr)
				CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
		}
	}

	void ModelComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		RenderableComponent::GetEditorDescription(descBuilder);

		Material* tickMaterial = new Material("TickMaterial", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
		tickMaterial->AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/tick_icon.png", GL_SRGB_ALPHA, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true), "albedo1"));

		descBuilder.AddField("Render as billboard").GetTemplates().TickBox(RenderAsBillboard);
		descBuilder.AddField("Hide").GetTemplates().TickBox(Hide);

		UICanvasFieldCategory& cat = descBuilder.GetCanvas().AddCategory("Mesh instances");
		cat.GetExpandButton()->CreateComponent<TextConstantSizeComponent>("NrMeshInstancesText", Transform(), std::to_string(MeshInstances.size()), "", std::pair<TextAlignment, TextAlignment>(TextAlignment::CENTER, TextAlignment::CENTER));
		cat.GetTemplates().ListSelection<std::unique_ptr<MeshInstance>>(MeshInstances.begin(), MeshInstances.end(), [this, descBuilder](UIAutomaticListActor& listActor, std::unique_ptr<MeshInstance>& meshInst)
			{
				std::string name = meshInst->GetMesh().GetLocalization().NodeName + " (" + meshInst->GetMesh().GetLocalization().SpecificName + ")";
				listActor.CreateChild<UIButtonActor>(name + "Button", name, [this, descBuilder, &meshInst]() mutable {
					UIWindowActor& window = dynamic_cast<UICanvasActor*>(&descBuilder.GetCanvas())->CreateChildCanvas<UIWindowActor>("MeshViewport");
					window.SetTransform(Transform(Vec2f(0.0f), Vec2f(0.5f)));

					GameScene& meshPreviewScene = GameHandle->CreateScene("GEE_Mesh_Preview_Scene");

					LightProbeComponent& probe = meshPreviewScene.GetRootActor()->CreateComponent<LightProbeComponent>("PreviewLightProbe");
					LightProbeLoader::LoadLightProbeFromFile(probe, "EditorAssets/winter_lake_01_4k.hdr");

					ModelComponent& model = meshPreviewScene.CreateActorAtRoot<Actor>("MeshPreviewActor").CreateComponent<ModelComponent>("MeshPreviewModel");
					model.AddMeshInst(*meshInst);

					Actor& camActor = meshPreviewScene.CreateActorAtRoot<Actor>("MeshPreviewCameraActor");
					CameraComponent& cam = camActor.CreateComponent<CameraComponent>("MeshPreviewCamera", glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f));
					camActor.GetTransform()->Move(Vec3f(0.0f, 0.0f, 10.0f));
					meshPreviewScene.BindActiveCamera(&cam);

					Controller& camController = camActor.CreateChild<Controller>("MeshPreviewCameraController");
					camController.SetPossessedActor(&camActor);

					UIButtonActor& viewportButton = window.CreateChild<UIButtonActor>("MeshPreviewViewportActor", [this, &meshPreviewScene, &camController]() { std::cout << "VIEWPORT WCISIETY\n"; GameHandle->SetActiveScene(&meshPreviewScene); GameHandle->PassMouseControl(&camController); });

					RenderEngineManager& renderHandle = *GameHandle->GetRenderEngineHandle();

					GameSettings* settings = new GameSettings(*GameHandle->GetGameSettings());

					settings->WindowSize = glm::uvec2(1024);
					RenderToolboxCollection& renderTbCollection = renderHandle.AddRenderTbCollection(RenderToolboxCollection("GEE_E_Mesh_Preview_Toolbox_Collection", settings->Video));

					std::shared_ptr<Material> viewportMaterial = std::make_shared<Material>("GEE_E_Mesh_Preview_Viewport", 0.0f, renderHandle.FindShader("Forward_NoLight"));
					renderHandle.AddMaterial(viewportMaterial);
					viewportMaterial->AddTexture(std::make_shared<NamedTexture>(*renderTbCollection.GetTb<FinalRenderTargetToolbox>()->GetFinalFramebuffer().GetColorBuffer(0), "albedo1"));
					viewportButton.SetMatIdle(*viewportMaterial);
					viewportButton.SetMatClick(*viewportMaterial);
					viewportButton.SetMatHover(*viewportMaterial);

					window.SetOnCloseFunc([&meshPreviewScene, &renderHandle, viewportMaterial, &renderTbCollection]() { meshPreviewScene.MarkAsKilled();  renderHandle.EraseMaterial(*viewportMaterial); renderHandle.EraseRenderTbCollection(renderTbCollection); });
					});
			});

		descBuilder.AddField("Override materials").GetTemplates().ObjectInput<Material, Material>([this]() { return GameHandle->GetRenderEngineHandle()->GetMaterials(); }, [this](Material* material) { OverrideInstancesMaterial(material); });
	}

	unsigned int ModelComponent::GetUIDepth() const
	{
		return GetElementDepth();
	}

}