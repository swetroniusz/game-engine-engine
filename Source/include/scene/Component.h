#pragma once
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>
#include <game/GameScene.h>
#include <math/Transform.h>
#include <game/GameManager.h>
#include <editor/EditorManager.h>
#include <physics/CollisionObject.h>
#include <rendering/Material.h>

namespace GEE
{

	enum class EditorElementRepresentation
	{
		INPUT_BOX,
		TICK_BOX,
		SLIDER,
		PALETTE
	};

	struct EditorElementDesc
	{

	};

	class EditorDescriptionBuilder;

	class Killable
	{
	public:
		virtual bool IsBeingKilled();
		template <typename FieldType> void AddReference(FieldType&);
		template <typename FieldType> void RemoveReference(FieldType&);
	protected:
		virtual void MarkAsKilled();

	};

	class ComponentBase
	{
	public:
		virtual Component& AddComponent(std::unique_ptr<Component> component) = 0;
	};

	class Component : public ComponentBase
	{
	public:
		Component(Actor&, Component* parentComp, const std::string& name = "A Component", const Transform& t = Transform());

		Component(const Component&) = delete;
		Component(Component&&);// = delete;
		//Component(Actor&, Component&&);

	protected:
		template <typename CompClass> friend class HierarchyTemplate::HierarchyNode;
		Component& operator=(const Component&);

		template <typename FieldType> void AddToGarbageCollector(FieldType*& field);
	public:
		Component& operator=(Component&&);

		virtual void OnStart();
		virtual void OnStartAll();

		void DebugHierarchy(int nrTabs = 0)
		{
			std::string tabs;
			for (int i = 0; i < nrTabs; i++)	tabs += "	";
			std::cout << tabs + "-" + Name + "\n";

			for (int i = 0; i < static_cast<int>(Children.size()); i++)
				Children[i]->DebugHierarchy(nrTabs + 1);
		}

		Actor& GetActor() const;
		const std::string& GetName() const;
		Transform& GetTransform();
		const Transform& GetTransform() const;
		std::vector<Component*> GetChildren();
		GameScene& GetScene() const;
		Physics::CollisionObject* GetCollisionObj() const;

		bool IsBeingKilled() const;

		void SetName(std::string name);
		void SetTransform(Transform transform);
		Physics::CollisionObject* SetCollisionObject(std::unique_ptr<Physics::CollisionObject>);
		virtual Component& AddComponent(std::unique_ptr<Component> component) override;
		void AddComponents(std::vector<std::unique_ptr<Component>> components);

		template<typename ChildClass, typename... Args> ChildClass& CreateComponent(Args&&... args);


		virtual void Update(float);
		void UpdateAll(float dt);

		virtual void QueueAnimation(Animation*);
		void QueueAnimationAll(Animation*);

		virtual void QueueKeyFrame(AnimationChannel&);
		void QueueKeyFrameAll(AnimationChannel&);

		virtual void DebugRender(RenderInfo info, Shader* shader) const; //this method should only be called to render the component as something (usually a textured billboard) to debug the project.
		void DebugRenderAll(RenderInfo info, Shader* shader) const;

		virtual std::shared_ptr<AtlasMaterial> LoadDebugRenderMaterial(const std::string& materialName, const std::string& path);
		virtual MaterialInstance GetDebugMatInst(EditorIconState);

		Component* SearchForComponent(std::string name);
		template <class CompClass = Component> CompClass* GetComponent(const std::string& name)
		{
			if (Name == name)
				return dynamic_cast<CompClass*>(this);

			for (const auto& it : Children)
				if (auto found = it->GetComponent<CompClass>(name))
					return found;

			return nullptr;
		}
		template<class CompClass> void GetAllComponents(std::vector <CompClass*>* comps)	//this function returns every element further in the hierarchy tree (kids, kids' kids, ...) that is of CompClass type
		{
			if (!comps)
				return;
			for (unsigned int i = 0; i < Children.size(); i++)
			{
				CompClass* c = dynamic_cast<CompClass*>(Children[i].get());
				if (c)					//if dynamic cast was succesful - this children is of CompCLass type
					comps->push_back(c);
			}
			for (unsigned int i = 0; i < Children.size(); i++)
				Children[i]->GetAllComponents<CompClass>(comps);	//do it reccurently in every child
		}
		template<class CastToClass, class CheckClass> void GetAllComponents(std::vector <CastToClass*>* comps) //this function works like the previous method, but every element must also be of CheckClass type (for ex. CastToClass = CollisionComponent, CheckClass = BSphere)
		{																												  //it's useful when you want to get a vector of CastToClass type pointers to objects, that are also of CheckClass type
			if (!comps)
				return;
			for (unsigned int i = 0; i < Children.size(); i++)
			{
				CastToClass* c = dynamic_cast<CastToClass*>(dynamic_cast<CheckClass*>(Children[i].get()));	//we're casting to CheckClass to check if our child is of the required type; we then cast to CastToClass to get a pointer of this type
				if (c)
					comps->push_back(c);
			}
			for (unsigned int i = 0; i < Children.size(); i++)
				Children[i]->GetAllComponents<CastToClass, CheckClass>(comps);	//robimy to samo we wszystkich "dzieciach"
		}

		virtual void GetEditorDescription(EditorDescriptionBuilder);

		void MarkAsKilled();

		template <typename Archive> void Save(Archive& archive)	 const
		{
			archive(CEREAL_NVP(Name), CEREAL_NVP(ComponentTransform), CEREAL_NVP(CollisionObj));
			archive(CEREAL_NVP(Children));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			archive(CEREAL_NVP(Name), CEREAL_NVP(ComponentTransform), CEREAL_NVP(CollisionObj));
			if (CollisionObj)
				Scene.GetPhysicsData()->AddCollisionObject(*CollisionObj, ComponentTransform);

			std::cout << "Serializing comp " << Name << '\n';
			if (GameHandle->HasStarted())
				OnStartAll();

			/*
				This code is not very elegant; it's because Components and Actors are not default_constructible.
				We always need to pass at least one reference to either the GameScene (in the case of Actors) or to the Actor (of the constructed Component).
				Furthermore, if an Actor/Component is not at the root of hierarchy (of a scene/an actor) we should always pass the pointer to the parent to the constructor, since users might wrongly assume that an Actor/Component is the root if the parent pointer passed to the constructor is nullptr.
				This might lead to bugs, so we avoid that.
			*/

			cereal::LoadAndConstruct<Component>::ParentComp = this;	//Set the static parent pointer to this
			archive(CEREAL_NVP(Children));	//We want all children to be constructed using the pointer. IMPORTANT: The first child will be serialized (the Load method will be called) before the next child is constructed! So the ParentComp pointer will be changed to the address of the new child when we construct its children. We counteract that in the next line.
			cereal::LoadAndConstruct<Component>::ParentComp = ParentComponent;	//There are no more children of this to serialize, so we "move up" the hierarchy and change the parent pointer to the parent of this, to allow brothers (other children of parent of this) to be constructed.

			for (auto& it : Children)
				it->GetTransform().SetParentTransform(&ComponentTransform);
		}
		/*template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<Component>& construct)
		{
			if (!LoadAndConstruct<Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<Component>::ActorRef, LoadAndConstruct<Component>::ParentComp, "");
			construct->Load(ar);
		}
		*/
		virtual ~Component();

	public:
		friend class Actor;
		std::unique_ptr<Component> DetachChild(Component& soughtChild);	//Find child in hierarchy and detach it from its parent
		void MoveChildren(Component& moveTo);
		void Delete();

		std::string Name;

		Transform ComponentTransform;
		std::vector <std::unique_ptr<Component>> Children;
		std::unique_ptr<Physics::CollisionObject> CollisionObj;

		GameScene& Scene; //The scene that this Component is present in
		Actor& ActorRef;
		Component* ParentComponent;
		GameManager* GameHandle;

		bool bKillingProcessStarted;

		std::shared_ptr<AtlasMaterial> DebugRenderMat;
		std::shared_ptr<MaterialInstance> DebugRenderMatInst;
		mutable glm::mat4 DebugRenderLastFrameMVP;
	};

	/*template<typename ChildClass>
	inline ChildClass& Component::CreateComponent(ChildClass&& objectCRef)
	{
		std::unique_ptr<ChildClass> createdChild = std::make_unique<ChildClass>(std::move(objectCRef));
		ChildClass& childRef = *createdChild;
		AddComponent(std::move(createdChild));

		return (ChildClass&)childRef;
	}*/


	template<typename ChildClass, typename... Args>
	inline ChildClass& Component::CreateComponent(Args&&... args)
	{
		std::unique_ptr<ChildClass> createdChild = std::make_unique<ChildClass>(ActorRef, this, std::forward<Args>(args)...);
		ChildClass& childRef = *createdChild;
		AddComponent(std::move(createdChild));

		if (GameHandle->HasStarted())
			childRef.OnStartAll();

		return (ChildClass&)childRef;
	}
	void CollisionObjRendering(RenderInfo& info, GameManager& gameHandle, Physics::CollisionObject& obj, const Transform& t, const glm::vec3& color = glm::vec3(0.1f, 0.6f, 0.3f));
}

namespace cereal
{

	template <> struct LoadAndConstruct<GEE::Component>
	{
		static GEE::Actor* ActorRef;			//= dummy actor
		static GEE::Component* ParentComp;	//= always nullptr
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::Component>& construct)
		{
			std::cout << "trying to construct comp " << ActorRef << '\n';
			if (!ActorRef)
				return;

			construct(*ActorRef, ParentComp);
			//ar(base_class<Component>(construct.ptr()));
			construct->Load(ar);
			//ar(*construct.ptr());
		}
	};

	template <> struct LoadAndConstruct<GEE::CameraComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::CameraComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->Load(ar);
		}
	};

	template <> struct LoadAndConstruct<GEE::ModelComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::ModelComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->Load(ar);
		}
	};

	template <> struct LoadAndConstruct<GEE::TextComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::TextComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;

			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "", GEE::Transform(), "", "");
			construct->Load(ar);
		}
	};

	template <> struct LoadAndConstruct<GEE::BoneComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::BoneComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->Load(ar);
		}
	};

	template <> struct LoadAndConstruct<GEE::LightComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::LightComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->GEE::LightComponent::Load(ar);
		}
	};

	template <> struct LoadAndConstruct<GEE::Audio::SoundSourceComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::Audio::SoundSourceComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->GEE::Audio::SoundSourceComponent::Load(ar);
		}
	};
	template <> struct LoadAndConstruct<GEE::AnimationManagerComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::AnimationManagerComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->Load(ar);
		}
	};
	template <> struct LoadAndConstruct<GEE::LightProbeComponent>
	{
		template <class Archive>
		static void load_and_construct(Archive& ar, cereal::construct<GEE::LightProbeComponent>& construct)
		{
			if (!LoadAndConstruct<GEE::Component>::ActorRef)
				return;
			construct(*LoadAndConstruct<GEE::Component>::ActorRef, LoadAndConstruct<GEE::Component>::ParentComp, "");
			construct->Load(ar);
		}
	};
}