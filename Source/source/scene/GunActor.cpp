#include <scene/GunActor.h>
#include <scene/SoundSourceComponent.h>
#include <assetload/FileLoader.h>
#include <input/Event.h>

#include <UI/UICanvasActor.h>
#include <UI/UIActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIButtonActor.h>
#include <UI/UIListActor.h>

namespace GEE
{
	GunActor::GunActor(GameScene& scene, Actor* parentActor, std::string name) :
		Actor(scene, parentActor, name),
		GunModel(nullptr),
		ParticleMeshInst(nullptr),
		GunBlast(nullptr)
	{
		FireCooldown = 2.0f;
		CooldownLeft = 0.0f;
		FiredBullets = 0;
	}

	void GunActor::Setup()
	{
		if (!SetupStream)
		{
			std::cerr << "ERROR! Can't setup GunActor " << Name << " dla " << this << ".\n";
			return;
		}

		std::string str;
		(*SetupStream) >> str;

		if (str == "FireMaterial")
		{
			(*SetupStream) >> str;
			std::cout << "Firematerial = " << GameHandle->GetRenderEngineHandle()->FindMaterial(str) << ".\n";
		}

		ModelComponent* found = GetRoot()->GetComponent<ModelComponent>("FireParticle");
		std::cout << "Wyszukalem se: " << found << ".\n";
		dynamic_cast<ModelComponent*>(found->SearchForComponent("Quad"))->SetRenderAsBillboard(true);
		ParticleMeshInst = found->FindMeshInstance("Quad");
		ParticleMeshInst->GetMaterialInst()->SetInterp(&dynamic_cast<AtlasMaterial*>(&ParticleMeshInst->GetMaterialInst()->GetMaterialRef())->GetTextureIDInterpolatorTemplate(Interpolation(0.0f, 0.25f, InterpolationType::LINEAR), 0.0f, dynamic_cast<AtlasMaterial*>(&ParticleMeshInst->GetMaterialInst()->GetMaterialRef())->GetMaxTextureID()));
		ParticleMeshInst->GetMaterialInst()->SetDrawBeforeAnim(false);
		ParticleMeshInst->GetMaterialInst()->SetDrawAfterAnim(false);
		GunModel = GetRoot()->GetComponent<ModelComponent>("MyDoubleBarrel");

		/*
		GunModel->GetTransform()->AddInterpolator("position", 10.0f, 20.0f, glm::vec3(0.0f, 0.0f, -10.0f), InterpolationType::LINEAR);
		GunModel->GetTransform()->AddInterpolator("rotation", 10.0f, 5.0f, glm::vec3(360.0f, 0.0f, 0.0f), InterpolationType::QUADRATIC, false, AnimBehaviour::STOP, AnimBehaviour::EXTRAPOLATE);
		GunModel->GetTransform()->AddInterpolator("scale", 10.0f, 10.0f, glm::vec3(0.3f, 0.3f, 0.2f), InterpolationType::CONSTANT);*/

		GunBlast = Scene.GetAudioData()->FindSource("DoubleBarrelBlast");

		Actor::Setup();
	}

	void GunActor::Update(float deltaTime)
	{
		if (GunModel && GunModel->IsBeingKilled())
			GunModel = nullptr;
		if (GunBlast && GunBlast->IsBeingKilled())
			GunBlast = nullptr;

		if (GunModel)
			GunModel->GetTransform().Update(deltaTime);
		CooldownLeft -= deltaTime;
		Actor::Update(deltaTime);
	}

	void GunActor::HandleEvent(const Event& ev)
	{
	}

	void GunActor::FireWeapon()
	{
		if (CooldownLeft > 0.0f)
			return;

		if (ParticleMeshInst)
			ParticleMeshInst->GetMaterialInst()->ResetAnimation();
		if (GunBlast)
			GunBlast->Play();

		if (GunModel)
		{
			GunModel->GetTransform().AddInterpolator<glm::quat>("rotation", 0.0f, 0.25f, glm::quat(glm::vec3(0.0f)), toQuat(glm::vec3(30.0f, 0.0f, 0.0f)), InterpolationType::QUINTIC, true);
			GunModel->GetTransform().AddInterpolator<glm::quat>("rotation", 0.25f, 1.25f, glm::quat(glm::vec3(0.0f)), InterpolationType::QUADRATIC, true);
		}

		Actor& actor = Scene.CreateActorAtRoot<Actor>("Bullet" + std::to_string(FiredBullets));
		//TODO: Change it so the bullet is fired at the barrel, not at the center
		std::unique_ptr<ModelComponent> bulletModel = std::make_unique<ModelComponent>(ModelComponent(actor, nullptr, "BulletModel" + std::to_string(FiredBullets++), Transform(GetTransform()->GetWorldTransform().PositionRef, glm::vec3(0.0f), glm::vec3(0.2f))));
		bulletModel->OnStart();
		EngineDataLoader::LoadModel("hqSphere/hqSphere.obj", *bulletModel, MeshTreeInstancingType::ROOTTREE, GameHandle->GetRenderEngineHandle()->FindMaterial("RustedIron").get());

		std::unique_ptr<CollisionObject> dupa = std::make_unique<CollisionObject>(CollisionObject(false, CollisionShapeType::COLLISION_SPHERE));
		CollisionObject& col = *bulletModel->SetCollisionObject(std::move(dupa));
		if (GunModel)
			GameHandle->GetPhysicsHandle()->ApplyForce(col, GunModel->GetTransform().GetWorldTransform().GetFrontVec() * 0.25f);

		actor.ReplaceRoot(std::move(bulletModel));

		actor.DebugHierarchy();


		CooldownLeft = FireCooldown;
	}
	void GunActor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		UICanvasField& gunModelField = descBuilder.AddField("GunModel");
		gunModelField.GetTemplates().ComponentInput<ModelComponent>(*GetRoot(), GunModel);
		std::cout << "Second element pos: " << gunModelField.GetTransform()->PositionRef.y << '\n';

		UICanvasField& blastField = descBuilder.AddField("Blast sound");
		blastField.GetTemplates().ComponentInput<SoundSourceComponent>(*GetRoot(), GunBlast);

		UICanvasField& fireField = descBuilder.AddField("Fire");
		fireField.CreateChild<UIButtonActor>("FireButton", "Fire", [this]() { FireWeapon(); });

		//[this]() {}
	}

}