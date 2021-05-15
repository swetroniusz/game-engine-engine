#pragma once
#include <scene/Actor.h>
#include <scene/RenderableComponent.h>
#include <UI/UICanvas.h>

class UIAutomaticListActor;
class UIListActor;
class UICanvasField;
class UIButtonActor;
class UICanvasFieldCategory;
class UIActorDefault;

class UICanvasActor : public Actor, public UICanvas
{
public:
	UICanvasActor(GameScene&, Actor* parentActor, const std::string& name);
	UICanvasActor(UICanvasActor&&);

	virtual void OnStart() override;

	UIActorDefault* GetScaleActor();
	virtual glm::mat4 GetView() const override;
	virtual NDCViewport GetViewport() const override;
	virtual const Transform* GetCanvasT() const override;
	virtual Transform ToCanvasSpace(const Transform& worldTransform) const override;

	bool IsInContext();

	void RefreshFieldsList();

	void HideScrollBars();
	virtual void ClampViewToElements() override;

	virtual Actor& AddChild(std::unique_ptr<Actor>) override;
	virtual UICanvasFieldCategory& AddCategory(const std::string& name) override;
	virtual UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getElementOffset = nullptr) override;

	virtual void HandleEvent(const Event& ev) override;
	virtual void HandleEventAll(const Event& ev) override;
	
	virtual RenderInfo BindForRender(const RenderInfo&, const glm::uvec2& res) override;
	virtual void UnbindForRender(const glm::uvec2& res) override;


protected:
	virtual void GetExternalButtons(std::vector<UIButtonActor*>&) const;
private:
	void CreateScrollBars();
	template <VecAxis barAxis> void UpdateScrollBarT();
public:
	UIActorDefault* ScaleActor;
	UIListActor* FieldsList;
	glm::vec3 FieldSize;
	UIScrollBarActor* ScrollBarX, *ScrollBarY, *BothScrollBarsButton;
	UIScrollBarActor *ResizeBarX, *ResizeBarY;

	UICanvasActor* ContextStealerParent;
	std::vector<UICanvasActor*> ContextStealers;
};

#include <UI/UIListActor.h>
class UICanvasFieldCategory : public UIListActor
{
public:
	UICanvasFieldCategory(GameScene& scene, Actor* parentActor, const std::string& name);
	UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getElementOffset = nullptr);
	virtual glm::vec3 GetListOffset() override;
	void SetOnExpansionFunc(std::function<void()> onExpansionFunc);
	virtual void HandleEventAll(const Event& ev) override;
private:
	std::function<void()> OnExpansionFunc;
	UIButtonActor* ExpandButton;
};

class EditorDescriptionBuilder
{
public:
	EditorDescriptionBuilder(EditorManager&, UIActorDefault&);
	EditorDescriptionBuilder(EditorManager&, Actor&, UICanvas&);
	GameScene& GetEditorScene();
	UICanvas& GetCanvas();
	Actor& GetDescriptionParent();
	EditorManager& GetEditorHandle();

	UICanvasField& AddField(const std::string& name, std::function<glm::vec3()> getFieldOffsetFunc = nullptr);	//equivalent to GetCanvasActor().AddField(...). I put it here for easier access.

	template <typename ChildClass, typename... Args> ChildClass& CreateActor(Args&&...);

	void SelectComponent(Component*);
	void SelectActor(Actor*);
	void RefreshScene();

	void DeleteDescription();
private:
	EditorManager& EditorHandle;
	GameScene& EditorScene;
	Actor& DescriptionParent;
	UICanvas& CanvasRef;
};

template <typename ChildClass, typename... Args>
inline ChildClass& EditorDescriptionBuilder::CreateActor(Args&&... args)
{
	return DescriptionParent.CreateChild<ChildClass>(std::forward<Args>(args)...);
}



UICanvasField& AddFieldToCanvas(const std::string& name, UICanvasElement& element, std::function<glm::vec3()> getFieldOffsetFunc = nullptr);