#include <scene/Actor.h>
#include <scene/BoneComponent.h>
#include <rendering/LightProbe.h>
#include <scene/RenderableComponent.h>
#include <scene/LightComponent.h>
#include <scene/LightProbeComponent.h>
#include <scene/SoundSourceComponent.h>
#include <scene/CameraComponent.h>
#include <game/GameScene.h>
#include <physics/CollisionObject.h>
#include <scene/hierarchy/HierarchyTree.h>
#include <UI/UICanvas.h>

#include <input/InputDevicesStateRetriever.h>
#include <UI/UICanvasActor.h>

namespace GEE
{
	GameScene::GameScene(GameManager& gameHandle, const std::string& name, bool isAnUIScene) :
		RenderData(std::make_unique<GameSceneRenderData>(gameHandle.GetRenderEngineHandle(), isAnUIScene)),
		PhysicsData(std::make_unique<Physics::GameScenePhysicsData>(gameHandle.GetPhysicsHandle())),
		AudioData(std::make_unique<Audio::GameSceneAudioData>(gameHandle.GetAudioEngineHandle())),
		ActiveCamera(nullptr),
		Name(name),
		GameHandle(&gameHandle),
		bKillingProcessStarted(false),
		CurrentBlockingCanvas(nullptr)
	{
		RootActor = std::make_unique<Actor>(*this, nullptr, "SceneRoot");
		BlockingCanvases;
	}

	GameScene::GameScene(GameScene&& scene) :
		RootActor(nullptr),
		RenderData(std::move(scene.RenderData)),
		PhysicsData(std::move(scene.PhysicsData)),
		AudioData(std::move(scene.AudioData)),
		ActiveCamera(scene.ActiveCamera),
		Name(scene.Name),
		GameHandle(scene.GameHandle),
		bKillingProcessStarted(scene.bKillingProcessStarted),
		CurrentBlockingCanvas(nullptr)
	{
		RootActor = std::make_unique<Actor>(*this, nullptr, "SceneRoot");
	}

	const std::string& GameScene::GetName() const
	{
		return Name;
	}

	Actor* GameScene::GetRootActor()
	{
		return RootActor.get();
	}

	const Actor* GameScene::GetRootActor() const
	{
		return RootActor.get();
	}

	CameraComponent* GameScene::GetActiveCamera()
	{
		return ActiveCamera;
	}

	GameSceneRenderData* GameScene::GetRenderData()
	{
		return RenderData.get();
	}

	Physics::GameScenePhysicsData* GameScene::GetPhysicsData()
	{
		return PhysicsData.get();
	}

	Audio::GameSceneAudioData* GameScene::GetAudioData()
	{
		return AudioData.get();
	}

	GameManager* GameScene::GetGameHandle()
	{
		return GameHandle;
	}

	bool GameScene::IsBeingKilled() const
	{
		return bKillingProcessStarted;
	}

	int GameScene::GetHierarchyTreeCount() const
	{
		return static_cast<int>(HierarchyTrees.size());
	}

	HierarchyTemplate::HierarchyTreeT* GameScene::GetHierarchyTree(int index)
	{
		if (index > GetHierarchyTreeCount() - 1 || index < 0)
			return nullptr;

		return HierarchyTrees[index].get();
	}

	void GameScene::AddPostLoadLambda(std::function<void()> postLoadLambda)
	{
		PostLoadLambdas.push_back(postLoadLambda);
	}

	void GameScene::AddBlockingCanvas(UICanvas& canvas)
	{
		BlockingCanvases.push_back(&canvas);
		std::sort(BlockingCanvases.begin(), BlockingCanvases.end(), [](UICanvas* lhs, UICanvas* rhs) { return lhs->GetCanvasDepth() < rhs->GetCanvasDepth(); });
	}

	void GameScene::EraseBlockingCanvas(UICanvas& canvas)
	{
		BlockingCanvases.erase(std::remove_if(BlockingCanvases.begin(), BlockingCanvases.end(), [this, &canvas](UICanvas* canvasVec) { bool matches = canvasVec == &canvas; if (matches && CurrentBlockingCanvas == canvasVec) CurrentBlockingCanvas = nullptr; return matches; }), BlockingCanvases.end());	//should still be sorted
	}

	UICanvas* GameScene::GetCurrentBlockingCanvas() const
	{
		return CurrentBlockingCanvas;
	}

	std::string GameScene::GetUniqueActorName(const std::string& name) const
	{
		std::vector <Actor*> actorsWithSimilarNames;
		bool sameNameExists = false;
		std::function<void(Actor&)> getActorsWithSimilarNamesFunc = [&actorsWithSimilarNames, name, &getActorsWithSimilarNamesFunc, &sameNameExists](Actor& currentActor) { if (currentActor.GetName().rfind(name, 0) == 0) /* if currentActor's name stars with name */ { actorsWithSimilarNames.push_back(&currentActor); if (currentActor.GetName() == name) sameNameExists = true; }	for (auto& it : currentActor.GetChildren()) getActorsWithSimilarNamesFunc(*it); };

		getActorsWithSimilarNamesFunc(*RootActor);
		if (!sameNameExists)
			return name;

		int addedIndex = 1;
		std::string currentNameCandidate = name + std::to_string(addedIndex);
		while (std::find_if(actorsWithSimilarNames.begin(), actorsWithSimilarNames.end(), [currentNameCandidate](Actor* actor) { return actor->GetName() == currentNameCandidate; }) != actorsWithSimilarNames.end())
			currentNameCandidate = name + std::to_string(++addedIndex);

		return currentNameCandidate;
	}

	Actor& GameScene::AddActorToRoot(std::unique_ptr<Actor> actor)
	{
		return RootActor->AddChild(std::move(actor));
	}

	HierarchyTemplate::HierarchyTreeT& GameScene::CreateHierarchyTree(const std::string& name)
	{
		HierarchyTrees.push_back(std::make_unique<HierarchyTemplate::HierarchyTreeT>(HierarchyTemplate::HierarchyTreeT(*this, name)));
		std::cout << "Utworzono drzewo z root " << &HierarchyTrees.back()->GetRoot() << " - " << HierarchyTrees.back()->GetName() << '\n';
		return *HierarchyTrees.back();
	}

	HierarchyTemplate::HierarchyTreeT* GameScene::FindHierarchyTree(const std::string& name, HierarchyTemplate::HierarchyTreeT* treeToIgnore)
	{
		auto found = std::find_if(HierarchyTrees.begin(), HierarchyTrees.end(), [name, treeToIgnore](const std::unique_ptr<HierarchyTemplate::HierarchyTreeT>& tree) { return tree->GetName() == name && tree.get() != treeToIgnore; });
		if (found != HierarchyTrees.end())
			return (*found).get();

		return nullptr;
	}

	void GameScene::HandleEventAll(const Event& ev)
	{
		const Vec2f mouseNDC = static_cast<Vec2f>(GameHandle->GetInputRetriever().GetMousePositionNDC());
		auto found = std::find_if(BlockingCanvases.rbegin(), BlockingCanvases.rend(), [&mouseNDC](UICanvas* canvas) { return canvas->ContainsMouseCheck(mouseNDC) && !(dynamic_cast<UICanvasActor*>(canvas)->IsBeingKilled()); });
		if (found != BlockingCanvases.rend())
			CurrentBlockingCanvas = *found;
		else
			CurrentBlockingCanvas = nullptr;
		/*std::cout << "Current blocking canvas: " << CurrentBlockingCanvas;
		if (CurrentBlockingCanvas)
			std::cout << dynamic_cast<UICanvasActor*>(CurrentBlockingCanvas)->GetName();
		std::cout << '\n';*/
		RootActor->HandleEventAll(ev);
	}

	void GameScene::Update(float deltaTime)
	{
		if (IsBeingKilled())
		{
			Delete();
			return;
		}
		if (ActiveCamera && ActiveCamera->IsBeingKilled())
			BindActiveCamera(nullptr);

		RootActor->UpdateAll(deltaTime);
		for (auto& it : RenderData->SkeletonBatches)
			it->VerifySkeletonsLives();	//verify if any SkeletonInfos are invalid and get rid of any garbage objects
	}

	void GameScene::BindActiveCamera(CameraComponent* cam)
	{
		ActiveCamera = cam;
	}

	Actor* GameScene::FindActor(std::string name)
	{
		return RootActor->FindActor(name);
	}

	GameScene::~GameScene()
	{

	}

	void GameScene::MarkAsKilled()
	{
		bKillingProcessStarted = true;
		if (RootActor)
			RootActor->MarkAsKilled();

		if (PhysicsData)	GameHandle->GetPhysicsHandle()->RemoveScenePhysicsDataPtr(*PhysicsData);
		if (RenderData)		GameHandle->GetRenderEngineHandle()->RemoveSceneRenderDataPtr(*RenderData);
	}

	void GameScene::Delete()
	{
		GameHandle->DeleteScene(*this);
	}

	GameSceneRenderData::GameSceneRenderData(RenderEngineManager* renderHandle, bool isAnUIScene) :
		RenderHandle(renderHandle),
		ProbeTexArrays(std::make_shared<LightProbeTextureArrays>(LightProbeTextureArrays())),
		LightBlockBindingSlot(-1),
		ProbesLoaded(false),
		bIsAnUIScene(isAnUIScene),
		bUIRenderableDepthsDirtyFlag(false)
	{
		if (LightProbes.empty() && RenderHandle->GetShadingModel() == ShadingModel::SHADING_PBR_COOK_TORRANCE)
		{
			//LightProbeLoader::LoadLightProbeTextureArrays(this);
			ProbeTexArrays->IrradianceMapArr.Bind(12);
			ProbeTexArrays->PrefilterMapArr.Bind(13);
			ProbeTexArrays->BRDFLut.Bind(14);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	RenderEngineManager* GameSceneRenderData::GetRenderHandle()
	{
		return RenderHandle;
	}

	LightProbeTextureArrays* GameSceneRenderData::GetProbeTexArrays()
	{
		return ProbeTexArrays.get();
	}

	int GameSceneRenderData::GetAvailableLightIndex()
	{
		return Lights.size();
	}

	bool GameSceneRenderData::ContainsLights() const
	{
		return !Lights.empty();
	}

	bool GameSceneRenderData::ContainsLightProbes() const
	{
		return !LightProbes.empty();
	}

	bool GameSceneRenderData::HasLightWithoutShadowMap() const
	{
		for (auto& it : Lights)
			if (!it.get().HasValidShadowMap())
				return true;

		return false;
	}


	void GameSceneRenderData::AddRenderable(Renderable& renderable)
	{
		//if (bIsAnUIScene)
			//insertSorted(Renderables, &renderable, [](Renderable* value, Renderable* vecElement) { return value->GetUIDepth() < vecElement->GetUIDepth(); });
		//else
		Renderables.push_back(&renderable);
		if (bIsAnUIScene)
			MarkUIRenderableDepthsDirty();
		/*	std::cout << "***POST-ADD RENDERABLES***\n";
			i = 0;
			for (auto it : Renderables)
				std::cout << ++i << ". " << it << "### Depth:" << it->GetUIDepth() << '\n';*/
				//std::cout << "Adding renderable " << renderable.GetName() << " " << &renderable << "\n";
	}

	void GameSceneRenderData::AddLight(LightComponent& light)
	{
		Lights.push_back(light);
		for (int i = 0; i < static_cast<int>(Lights.size()); i++)
			Lights[i].get().SetIndex(i);
		if (LightBlockBindingSlot != -1)	//If light block was already set up, do it again because there isn't enough space for the new light.
			SetupLights(LightBlockBindingSlot);
	}

	void GameSceneRenderData::AddLightProbe(LightProbeComponent& probe)
	{
		LightProbes.push_back(&probe);
		for (int i = 0; i < static_cast<int>(LightProbes.size()); i++)
			LightProbes[i]->SetProbeIndex(i);

		if (!LightsBuffer.HasBeenGenerated())
			SetupLights(LightBlockBindingSlot);
	}

	std::shared_ptr<SkeletonInfo> GameSceneRenderData::AddSkeletonInfo()
	{
		std::shared_ptr<SkeletonInfo> info = std::make_shared<SkeletonInfo>(SkeletonInfo());
		if (SkeletonBatches.empty() || !SkeletonBatches.back()->AddSkeleton(info))
		{
			SkeletonBatches.push_back(std::make_shared<SkeletonBatch>());
			if (!SkeletonBatches.back()->AddSkeleton(info))
				std::cerr << "CRITICAL ERROR! Too many bones in SkeletonInfo for a SkeletonBatch\n";
		}
		return info;
	}

	void GameSceneRenderData::EraseRenderable(Renderable& renderable)
	{
		Renderables.erase(std::remove_if(Renderables.begin(), Renderables.end(), [&renderable](Renderable* renderableVec) {return renderableVec == &renderable; }), Renderables.end());
		//std::cout << "Erasing renderable " << renderable.GetName() << " " << &renderable << "\n";
	}

	void GameSceneRenderData::EraseLight(LightComponent& light)
	{
		Lights.erase(std::remove_if(Lights.begin(), Lights.end(), [&light](std::reference_wrapper<LightComponent>& lightVec) {return &lightVec.get() == &light; }), Lights.end());
		for (int i = 0; i < static_cast<int>(Lights.size()); i++)
			Lights[i].get().SetIndex(i);
		if (LightBlockBindingSlot != -1)	//If light block was already set up, do it again because there isn't enough space for the new light.
			SetupLights(LightBlockBindingSlot);
	}

	void GameSceneRenderData::EraseLightProbe(LightProbeComponent& lightProbe)
	{
		LightProbes.erase(std::remove_if(LightProbes.begin(), LightProbes.end(), [&lightProbe](LightProbeComponent* lightProbeVec) {return lightProbeVec == &lightProbe; }), LightProbes.end());
	}

	void GameSceneRenderData::MarkUIRenderableDepthsDirty()
	{
		bUIRenderableDepthsDirtyFlag = true;
	}

	void GameSceneRenderData::SetupLights(unsigned int blockBindingSlot)
	{
		LightBlockBindingSlot = blockBindingSlot;
		std::cout << "Setupping lights for bbindingslot " << blockBindingSlot << '\n';

		LightsBuffer.Generate(blockBindingSlot, sizeof(glm::vec4) * 2 + Lights.size() * 192);
		LightsBuffer.SubData1i((int)Lights.size(), (size_t)0);

		for (auto& light : Lights)
		{
			light.get().InvalidateCache();
		}

		//UpdateLightUniforms();
	}

	void GameSceneRenderData::UpdateLightUniforms()
	{
		LightsBuffer.offsetCache = sizeof(glm::vec4) * 2;
		for (auto& light : Lights)
		{
			light.get().InvalidateCache();
			light.get().UpdateUBOData(&LightsBuffer);
		}
	}

	int GameSceneRenderData::GetBatchID(SkeletonBatch& batch) const
	{
		for (int i = 0; i < static_cast<int>(SkeletonBatches.size()); i++)
			if (SkeletonBatches[i].get() == &batch)
				return i;

		return -1;	//batch not found; return invalid ID
	}

	SkeletonBatch* GameSceneRenderData::GetBatch(int ID)
	{
		if (ID < 0 || ID > SkeletonBatches.size() - 1)
			return nullptr;

		return SkeletonBatches[ID].get();
	}

	GameSceneRenderData::~GameSceneRenderData()
	{
		LightsBuffer.Dispose();
	}

	void GameSceneRenderData::AssertThatUIRenderablesAreSorted()
	{
		if (!bUIRenderableDepthsDirtyFlag)
			return;

		std::stable_sort(Renderables.begin(), Renderables.end(), [](Renderable* lhs, Renderable* rhs) { return lhs->GetUIDepth() < rhs->GetUIDepth(); });
		bUIRenderableDepthsDirtyFlag = false;
	}

	/*
	RenderableComponent* GameSceneRenderData::FindRenderable(std::string name)
	{
		auto found = std::find_if(Renderables.begin(), Renderables.end(), [name](Renderable* comp) { return comp->GetName() == name; });
		if (found != Renderables.end())
			return *found;

		return nullptr;
	}*/

	namespace Physics
	{
		GameScenePhysicsData::GameScenePhysicsData(PhysicsEngineManager* physicsHandle) :
			PhysicsHandle(physicsHandle),
			WasSetup(false)
		{
		}

		void GameScenePhysicsData::AddCollisionObject(CollisionObject& object, Transform& t)	//tytaj!!!!!!!!!!!!!!!!!!!!!!!!!!!! TUTAJ TUTAJ TU
		{
			CollisionObjects.push_back(&object);
			object.ScenePhysicsData = this;
			object.TransformPtr = &t;

			if (WasSetup)
				PhysicsHandle->AddCollisionObjectToPxPipeline(*this, object);
		}

		void GameScenePhysicsData::EraseCollisionObject(CollisionObject& object)
		{
			auto found = std::find(CollisionObjects.begin(), CollisionObjects.end(), &object);
			if (found != CollisionObjects.end())
			{
				if ((*found)->ActorPtr)
					(*found)->ActorPtr->release();
				CollisionObjects.erase(found);
			}
		}

		PhysicsEngineManager* GameScenePhysicsData::GetPhysicsHandle()
		{
			return PhysicsHandle;
		}
	}

	namespace Audio
	{
		GameSceneAudioData::GameSceneAudioData(Audio::AudioEngineManager* audioHandle) :
			AudioHandle(audioHandle)
		{
		}

		void GameSceneAudioData::AddSource(SoundSourceComponent& sourceComp)
		{
			Sources.push_back(sourceComp);
		}

		SoundSourceComponent* GameSceneAudioData::FindSource(std::string name)
		{
			auto found = std::find_if(Sources.begin(), Sources.end(), [name](std::reference_wrapper<SoundSourceComponent>& sourceComp) { return sourceComp.get().GetName() == name; });
			if (found != Sources.end())
				return &found->get();

			return nullptr;
		}
	}

}