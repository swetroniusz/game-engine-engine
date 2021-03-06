#include <game/GameManager.h>
#include <scene/hierarchy/HierarchyTree.h>

namespace GEE
{
	GameScene* GameManager::DefaultScene = nullptr;
	GameManager* GameManager::GamePtr = nullptr;

	template <> void EditorManager::Select<Component>(Component* obj, GameScene& editorScene) { SelectComponent(obj, editorScene); }
	template <> void EditorManager::Select<Actor>(Actor* obj, GameScene& editorScene) { SelectActor(obj, editorScene); }
	template <> void EditorManager::Select<GameScene>(GameScene* obj, GameScene& editorScene) { SelectScene(obj, editorScene); }

	bool HTreeObjectLoc::IsValidTreeElement() const
	{
		return TreePtr != nullptr;
	}

	std::string HTreeObjectLoc::GetTreeName() const
	{
		if (!IsValidTreeElement())
			return std::string();

		return TreePtr->GetName();
	}
	GameManager& GameManager::Get()
	{
		return *GamePtr;
	}
}