#include "PawnActor.h"
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/BoneComponent.h>
#include <animation/AnimationManagerActor.h>
#include <UI/UIListActor.h>
#include <scene/UIButtonActor.h>

PawnActor::PawnActor(GameScene& scene, const std::string& name):
	Actor(scene, name),
	PreAnimBonePos(glm::vec3(0.0f)),
	//RootBone(nullptr),
	AnimManager(nullptr),
	UpdateAnimsListInEditor(nullptr),
	AnimIndex(0),
	SpeedPerSec(1.0f),
	CurrentTargetPos(glm::vec3(0.0f)),
	PathIndex(0),
	Gun(nullptr)
{
}

void PawnActor::OnStart()
{
	Actor::OnStart();
	GetTransform()->SetPosition(glm::vec3(-5.0f, -0.51f, -20.0f));
}

void PawnActor::Update(float deltaTime)
{
	Actor::Update(deltaTime);

	if (IsBeingKilled() || !AnimManager)
		return;


	/*		glm::vec3 velocity = RootBone->GetTransform().GetWorldTransform().RotationRef * glm::abs(RootBone->GetTransform().PositionRef - PreAnimBonePos);
		velocity.y = 0.0f;
		//velocity.x = 0.0f;*/

	if (CurrentTargetPos != glm::vec3(0.0f))
	{
		if (glm::distance(CurrentTargetPos, GetTransform()->GetWorldTransform().PositionRef) < 0.05f)
		{
			CurrentTargetPos = glm::vec3(0.0f);
			if (AnimManager->GetCurrentAnim())
				AnimManager->GetCurrentAnim()->Stop();
			PathIndex = (++PathIndex) % 4;
			return;
		}

		if (!AnimManager->GetCurrentAnim())
			AnimManager->SelectAnimation(AnimManager->GetAnimInstance(AnimIndex));

		glm::vec3 posDir = CurrentTargetPos - GetTransform()->GetWorldTransform().PositionRef;
		posDir.y = 0.0f;
		posDir = glm::normalize(posDir);

		glm::vec3 playerDir = glm::normalize(Scene.FindActor("CameraActor")->GetTransform()->GetWorldTransform().PositionRef - GetTransform()->GetWorldTransform().PositionRef);
		float distance = glm::distance(Scene.FindActor("CameraActor")->GetTransform()->GetWorldTransform().PositionRef, GetTransform()->GetWorldTransform().PositionRef);
		std::cout << "Dot: " << glm::dot(playerDir, GetTransform()->RotationRef * glm::vec3(0.0f, 0.0f, -1.0f)) << '\n';
		if (Gun && glm::dot(playerDir, GetTransform()->RotationRef * glm::vec3(0.0f, 0.0f, -1.0f)) > glm::cos(glm::radians(30.0f)) && distance < 3.0f)
		{ 
			Gun->FireWeapon();
			Gun->GetTransform()->SetRotationWorld(quatFromDirectionVec(-playerDir));
			posDir = glm::normalize(glm::vec3(playerDir.x, 0.0f, playerDir.z));

		}

		GetTransform()->SetRotation(quatFromDirectionVec(-posDir));
		
		glm::vec3 velocity = GetTransform()->GetWorldTransform().RotationRef * glm::vec3(0.0f, 0.0f, -1.0f * SpeedPerSec * deltaTime);

		GetTransform()->Move(velocity);
	}

	MoveAlongPath();
}
#include <scene/UIInputBoxActor.h>
void PawnActor::GetEditorDescription(UIActor& actor, GameScene& editorScene)
{
	Actor::GetEditorDescription(actor, editorScene);

	AddFieldToCanvas("AnimManager", actor).GetTemplates().ComponentInput<AnimationManagerComponent>(*GetRoot(), [this](AnimationManagerComponent* animManager) {AnimManager = animManager; UpdateAnimsListInEditor(animManager); });
	AddFieldToCanvas("Target", actor).GetTemplates().VecInput(CurrentTargetPos);
	dynamic_cast<UIInputBoxActor*>(actor.FindActor("VecBox0"))->SetRetrieveContentEachFrame(true);
	dynamic_cast<UIInputBoxActor*>(actor.FindActor("VecBox1"))->SetRetrieveContentEachFrame(true);
	dynamic_cast<UIInputBoxActor*>(actor.FindActor("VecBox2"))->SetRetrieveContentEachFrame(true);

	AddFieldToCanvas("Gun", actor).GetTemplates().ObjectInput<Actor, GunActor>(*this, Gun);

	UIInputBoxActor& speedInputBox = AddFieldToCanvas("Speed", actor).CreateChild(UIInputBoxActor(editorScene, "SpeedInputBox"));
	speedInputBox.SetOnInputFunc([this](float val) { SpeedPerSec = val; }, [this]()->float { return SpeedPerSec; });
	speedInputBox.GetTransform()->Move(glm::vec2(1.0f, 0.0f));

	UICanvasField& animsField = AddFieldToCanvas("AnimList", actor);
	UIAutomaticListActor& animsList = animsField.CreateChild(UIAutomaticListActor(editorScene, "Animations list", glm::vec3(3.0f, 0.0f, 0.0f)));
	animsList.GetTransform()->Move(glm::vec2(1.0f, 0.0f));

	UpdateAnimsListInEditor = [this, &actor, &animsList, &editorScene](AnimationManagerComponent* animManager) {
		for (int i = 0; i < animManager->GetAnimInstancesCount(); i++)
			actor.GetCanvasPtr()->AddUIElement(animsList.CreateChild(UIButtonActor(editorScene, "Anim button", animManager->GetAnimInstance(i)->GetAnimation().Name, [this, i]() {AnimIndex = i; })));
		animsList.Refresh();
	};

	if (AnimManager)
		UpdateAnimsListInEditor(AnimManager);
}

void PawnActor::MoveToPosition(const glm::vec3& worldPos)
{
	CurrentTargetPos = worldPos;
}

void PawnActor::MoveAlongPath()
{
	switch (PathIndex)
	{
	case 0: MoveToPosition(glm::vec3(-5.0f, -0.51f, 0.0f)); break;
	case 1: MoveToPosition(glm::vec3(-15.0f, -0.51f, 0.0f)); break;
	case 2: MoveToPosition(glm::vec3(-15.0f, -0.51f, -20.0f)); break;
	case 3: MoveToPosition(glm::vec3(-5.0f, -0.51f, -20.0f)); break;
	}
}
