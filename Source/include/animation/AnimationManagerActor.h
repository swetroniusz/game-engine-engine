#pragma once
#include <animation/Animation.h>
#include <scene/Component.h>
#include <deque>

struct AnimationChannelInstance
{
	AnimationChannel& ChannelRef;
	Component& ChannelComp;

	bool IsValid;

	std::deque<std::unique_ptr<Interpolator<glm::vec3>>> PosKeysLeft, ScaleKeysLeft;
	std::deque<std::unique_ptr<Interpolator<glm::quat>>> RotKeysLeft;

	AnimationChannelInstance(AnimationChannel&, Component&);
	void Restart();
	void Stop();
	float GetTimeLeft()
	{
		float timeLeft = 0.0f;
		for (auto& it : PosKeysLeft)
			timeLeft += (1.0f - it->GetInterp()->GetT()) * it->GetInterp()->GetDuration();

		return timeLeft;
	}
	bool Update(float);	//Returns true if animation is taking place, false otherwise

};

class AnimationInstance
{
	Animation& Anim;
	Component& AnimRootComp;
	std::vector<std::unique_ptr<AnimationChannelInstance>> ChannelInstances;
	float TimePassed;

	bool IsValid;

public:
	AnimationInstance(Animation&, Component&);

	Animation& GetAnimation() const;
	bool HasFinished() const;
	void Update(float);
	void Stop();
	void Restart();
};

class AnimationManagerComponent : public Component
{
	std::vector<std::unique_ptr<AnimationInstance>> AnimInstances;
	AnimationInstance* CurrentAnim;

public:
	AnimationManagerComponent(Actor&, Component* parentComp, const std::string& name);

	AnimationInstance* GetAnimInstance(int index);
	int GetAnimInstancesCount() const;
	AnimationInstance* GetCurrentAnim();

	void AddAnimationInstance(AnimationInstance&&);

	virtual void Update(float) override;
	void SelectAnimation(AnimationInstance*);

	virtual void GetEditorDescription(EditorDescriptionBuilder) override;
};