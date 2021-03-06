#include "PawnActor.h"
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/BoneComponent.h>
#include <animation/AnimationManagerActor.h>
#include <UI/UIListActor.h>
#include <scene/UIButtonActor.h>

#include <scene/UIInputBoxActor.h>

namespace GEE
{
	PawnActor::PawnActor(GameScene& scene, Actor* parentActor, const std::string& name) :
		Actor(scene, parentActor, name),
		PreAnimBonePos(glm::vec3(0.0f)),
		//RootBone(nullptr),
		AnimManager(nullptr),
		UpdateAnimsListInEditor(nullptr),
		AnimIndex(0),
		SpeedPerSec(1.0f),
		CurrentTargetPos(glm::vec3(0.0f)),
		PathIndex(0),
		Gun(nullptr),
		PlayerTarget(nullptr)
	{
	}

	void PawnActor::OnStart()
	{
		Actor::OnStart();
	}

	void PawnActor::Update(float deltaTime)
	{
		Actor::Update(deltaTime);

		if (IsBeingKilled() || !AnimManager)
			return;

		if (AnimManager->IsBeingKilled())
		{
			AnimManager = nullptr;
			return;
		}
		if (Gun && Gun->IsBeingKilled())
			Gun = nullptr;


		/*		glm::vec3 velocity = RootBone->GetTransform().GetWorldTransform().RotationRef * glm::abs(RootBone->GetTransform().PositionRef - PreAnimBonePos);
			velocity.y = 0.0f;
			//velocity.x = 0.0f;*/

		if (CurrentTargetPos != glm::vec3(0.0f))
		{
			if (glm::distance(CurrentTargetPos, GetTransform()->GetWorldTransform().Pos()) < 0.05f)
			{
				CurrentTargetPos = glm::vec3(0.0f);
				if (AnimManager->GetCurrentAnim())
					AnimManager->GetCurrentAnim()->Stop();
				PathIndex = (++PathIndex) % 4;
				return;
			}

			if (!AnimManager->GetCurrentAnim())
				AnimManager->SelectAnimation(AnimManager->GetAnimInstance(AnimIndex));

			glm::vec3 posDir = CurrentTargetPos - GetTransform()->GetWorldTransform().Pos();
			posDir.y = 0.0f;
			posDir = glm::normalize(posDir);
			if (PlayerTarget)
			{
				glm::vec3 playerDir = glm::normalize(PlayerTarget->GetTransform()->GetWorldTransform().Pos() - GetTransform()->GetWorldTransform().Pos());
				float distance = glm::distance((glm::vec3)PlayerTarget->GetTransform()->GetWorldTransform().Pos(), GetTransform()->GetWorldTransform().Pos());
	
				if (Gun && glm::dot(playerDir, GetTransform()->Rot() * glm::vec3(0.0f, 0.0f, -1.0f)) > glm::cos(glm::radians(30.0f)) && distance < 3.0f)
				{
					Gun->FireWeapon();
					//Gun->GetTransform()->SetRotationWorld(quatFromDirectionVec(-playerDir));
					posDir = glm::normalize(glm::vec3(playerDir.x, 0.0f, playerDir.z));

				}
			}

			GetTransform()->SetRotation(quatFromDirectionVec(-posDir));

			glm::vec3 velocity = GetTransform()->GetWorldTransform().Rot() * glm::vec3(0.0f, 0.0f, -1.0f * SpeedPerSec * deltaTime);

			GetTransform()->Move(velocity);
		}

		MoveAlongPath();
	}

	void PawnActor::GetEditorDescription(EditorDescriptionBuilder descBuilder)
	{
		Actor::GetEditorDescription(descBuilder);

		descBuilder.AddField("Anim Manager").GetTemplates().ComponentInput<AnimationManagerComponent>(*GetRoot(), [this](AnimationManagerComponent* animManager) {AnimManager = animManager; UpdateAnimsListInEditor(animManager); });
		descBuilder.AddField("Target position").GetTemplates().VecInput(CurrentTargetPos);

		dynamic_cast<UIInputBoxActor*>(descBuilder.GetDescriptionParent().FindActor("VecBox0"))->SetRetrieveContentEachFrame(true);
		dynamic_cast<UIInputBoxActor*>(descBuilder.GetDescriptionParent().FindActor("VecBox1"))->SetRetrieveContentEachFrame(true);
		dynamic_cast<UIInputBoxActor*>(descBuilder.GetDescriptionParent().FindActor("VecBox2"))->SetRetrieveContentEachFrame(true);

		descBuilder.AddField("Gun").GetTemplates().ObjectInput<Actor, GunActor>(*this, Gun);
		descBuilder.AddField("Player target").GetTemplates().ObjectInput<Actor, Actor>(const_cast<Actor&>(*Scene.GetRootActor()), PlayerTarget);

		UIInputBoxActor& speedInputBox = descBuilder.AddField("Speed").CreateChild<UIInputBoxActor>("SpeedInputBox");
		speedInputBox.SetOnInputFunc([this](float val) { SpeedPerSec = val; }, [this]()->float { return SpeedPerSec; });

		UICanvasField& animsField = descBuilder.AddField("Anim List");
		UIAutomaticListActor& animsList = animsField.CreateChild<UIAutomaticListActor>("Animations list", glm::vec3(3.0f, 0.0f, 0.0f));

		UpdateAnimsListInEditor = [this, &animsList, descBuilder](AnimationManagerComponent* animManager) mutable {
			for (auto& it : animsList.GetChildren())
				it->MarkAsKilled();

			if (animManager)
				for (int i = 0; i < animManager->GetAnimInstancesCount(); i++)
					animsList.CreateChild<UIButtonActor>("Anim button", animManager->GetAnimInstance(i)->GetAnimation().Localization.Name, [this, i]() {AnimIndex = i; });
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

}