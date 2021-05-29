#pragma once

namespace GEE
{
	enum class EditorIconState
	{
		IDLE,
		HOVER,
		BEING_CLICKED_INSIDE,
		BEING_CLICKED_OUTSIDE,
		ACTIVATED
	};

	class UICanvasActor;
}