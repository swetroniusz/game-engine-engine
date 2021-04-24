#pragma once
#include <math/Box.h>

class UICanvas;
class Transform;


class UICanvasElement
{
public:
	UICanvasElement();
	UICanvasElement(const UICanvasElement&) = delete;
	UICanvasElement(UICanvasElement&&);
	virtual Boxf<Vec2f> GetBoundingBox(bool world = true);	//Returns a box at (0, 0) of size (0, 0). Canvas space
	UICanvas* GetCanvasPtr();

	virtual ~UICanvasElement();
protected:
	virtual void AttachToCanvas(UICanvas& canvas);
	friend class UICanvas;

	UICanvas* CanvasPtr;
};
/*
class UICanvasComponent : public UICanvasElement
{
public:
	virtual Transform GetCanvasSpaceTransform() override;
};

class UICanvasActor : public UICanvasElement
{
public:
	virtual Transform GetCanvasSpaceTransform() override;
};*/