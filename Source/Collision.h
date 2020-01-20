#pragma once
#include "Component.h"


enum CollisionType
{
	NONE,
	INTERSECT,
	CONTAIN
};

/*  ===========================================  */

class CollisionComponent : public Component
{
public:
	bool bFlipCollisionSide;
	CollisionComponent(std::string name, bool flip = false) : Component(name, Transform()) { bFlipCollisionSide = flip; }
};

////////////////////////

class BSphere: public CollisionComponent
{
public:
	BSphere(std::string name, bool flip = false) : CollisionComponent(name, flip) {}
};

////////////////////////

class BBox : public CollisionComponent
{
public:
	BBox(std::string name, bool flip = false) : CollisionComponent(name, flip) {}
};

/*  ===========================================  */

class Collision
{
	/*		SAT		*/
	static void GetCubeMinMax(glm::vec3, BBox, float*, float*);
	static CollisionType CheckAxis(glm::vec3, BBox, BBox, bool&);
	////////SAT///////

public:
	static CollisionType CubeCube(BBox, BBox, std::vector<glm::vec3>*); //SAT
	static CollisionType CubeSphere(BBox, BSphere, std::vector<glm::vec3>*);
	static bool SpherePoint(BSphere, glm::vec3);
	static CollisionType SphereSphere(BSphere, BSphere);

	static CollisionType CheckForCollision(CollisionComponent*, CollisionComponent*, std::vector <glm::vec3>*); //ta funkcja jest obrzydliwa; zdaje sobie z tego sprawe, ale nie mam lepszego pomyslu jak bardziej elegancko wywolywac odpowiednie sprawdzanie kolizji (na podstawie obiektu skonwertowanego w gore)
};

/*  ===========================================  */

float length2(glm::vec3);
glm::vec3 clampToNearestFace(BBox, glm::vec3);
std::vector <glm::vec3> getBounceNormals(BBox, BSphere);