#include "PoolBallActor.h"
#include <math/Transform.h>

namespace GEE
{
	PoolBallActor::PoolBallActor(GEE::GameScene& scene, GEE::Actor* parentActor, const std::string& name):
		PoolBallActor(scene, parentActor, name, Transform())
	{
	}

	PoolBallActor::PoolBallActor(GEE::GameScene& scene, GEE::Actor* parentActor, const std::string& name, const GEE::Transform& t):
		Actor(scene, parentActor, name, t)
	{
	}

	void PoolBallActor::Update(Time time)
	{
		Actor::Update(time);


		
	}
}
