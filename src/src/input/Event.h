#pragma once
#include <utility/Utility.h>
#include <queue>
#include <map>

namespace GEE
{
	class GameManager;

	enum class MouseButton
	{
		Left,
		Right,
		Middle,

		Mouse1,
		Mouse2,
		Mouse3,
		Mouse4,
		Mouse5,
		Mouse6,
		Mouse7,
		Mouse8,

		Last = Mouse8
	};

	enum class Key
	{
		Unknown = -1,
		Space = ' ',
		Apostrophe = '\'', /* ' */
		Comma = ';', /* , */
		Minus = '-', /* - */
		Period = '.', /* . */
		Slash = '/', /* / */
		Zero = '0',
		One,
		Two,
		Three,
		Four,
		Five,
		Six,
		Seven,
		Eight,
		Nine,
		Semicolon = ';', /* ; */
		Equal = '=', /* = */
		A = 'A',
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LeftBracket = '[', /* [ */
		Backslash = '\\', /* \ */
		RightBracket = ']', /* ] */
		GraveAccent = '`', /* ` */
		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */
		Escape = 256,
		Enter,
		Tab,
		Backspace,
		Insert,
		Delete,
		RightArrow,
		LeftArrow,
		DownArrow,
		UpArrow,
		PageUp,
		PageDown,
		Home,
		End,
		CapsLock,
		ScrollLock,
		NumLock,
		PrintScreen,
		Pause,
		F1 = 276,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		F25,
		Keypad0 = 320,
		Keypad1,
		Keypad2,
		Keypad3,
		Keypad4,
		Keypad5,
		Keypad6,
		Keypad7,
		Keypad8,
		Keypad9,
		KeypadDecimal = 330,
		KeypadDivide,
		KeypadMultiply,
		KeypadSubtract,
		KeypadAdd,
		KeypadEnter,
		KeypadEqual,
		LeftShift = 340,
		LeftControl,
		LeftAlt,
		LeftSuper,
		RightShift,
		RightControl,
		RightAlt,

		RightSup,
		Menu,
		Last = Menu
	};

	enum KeyModifierFlags
	{
		Shift = 1,
		Control = 2,
		Alt = 4,
		Super = 8,
		CapsLock = 16,
		NumLock = 32
	};

	enum class EventType
	{
		WindowClosed,
		WindowResized,
		KeyPressed,
		KeyRepeated,
		KeyReleased,
		CharacterEntered,
		MouseMoved,
		MouseScrolled,
		MousePressed,
		MouseReleased,
		FocusSwitched,

		CanvasViewChanged
	};


	class Event
	{
	public:
		/**
		 * @param type - an enum indicating the type of this Event. Should match the class of this Event.
		 * @param eventRoot - an optional Actor that will be the first to receive this Event, if it is in the active Scene. If it's not, or if eventRoot is nullptr, active scene's root will be used.
		*/
		Event(Actor* eventRoot = nullptr);
		virtual EventType GetType() const = 0;
		Actor* GetEventRoot() { return EventRoot; }
		const Actor* GetEventRoot() const { return EventRoot; }

		virtual ~Event() {}
	private:
		
		// The actor which will be used as the first recipient of this Event, and which will pass this Event to its children. Can be used for optimisation.
		Actor* EventRoot;
	};



	class CursorMoveEvent : public Event
	{
	public:
		CursorMoveEvent(Vec2f newPositionPx, Vec2u windowSizePx, Actor* eventRoot = nullptr);
		CursorMoveEvent(Vec2f newPositionNDC, Actor* eventRoot = nullptr);
		Vec2f GetNewPositionNDC() const;
		EventType GetType() const override { return EventType::MouseMoved; }

	private:
		Vec2f NewPositionNDC;
	};

	class MouseButtonEvent : public Event
	{
	public:
		MouseButtonEvent(MouseButton, int modifierBits, bool released = false, Actor* eventRoot = nullptr);
		MouseButton GetButton() const;
		int GetModifierBits() const;
		EventType GetType() const override { return bReleased ? EventType::MouseReleased : EventType::MousePressed; }

	private:
		const MouseButton Button;
		const int ModifierBits;
		const bool bReleased; // If true, then mouse button was released. Otherwise, it was pressed.
	};

	class MouseScrollEvent : public Event
	{
	public:
		MouseScrollEvent(Vec2f offset, Actor* eventRoot = nullptr);
		Vec2f GetOffset() const;
		EventType GetType() const override { return EventType::MouseScrolled; }

	private:
		Vec2f Offset;
	};

	class KeyEvent : public Event
	{
	public:
		enum KeyAction
		{
			Pressed,
			Released,
			Repeated
		};

		KeyEvent(Key, int modifierFlags, KeyAction action, Actor* eventRoot = nullptr);
		Key GetKeyCode() const;
		int GetModifierBits() const;
		EventType GetType() const override
		{
			switch (Action)
			{
			case Pressed: return EventType::KeyPressed;
			case Released: return EventType::KeyReleased;
			case Repeated: return EventType::KeyRepeated;
			}
		}

	private:
		Key KeyCode;
		KeyAction Action;
		int ModifierBits;
	};

	class CharEnteredEvent : public Event
	{
	public:
		CharEnteredEvent(unsigned int unicode, Actor* eventRoot = nullptr);
		std::string GetUTF8() const;
		EventType GetType() const override { return EventType::CharacterEntered; }

	private:
		unsigned int Unicode;
	};

	class WindowClosedEvent : public Event
	{
	public:
		using Event::Event;
		EventType GetType() const override { return EventType::WindowClosed; }
	};

	class WindowResizedEvent : public Event
	{
	public:
		using Event::Event;
		EventType GetType() const override { return EventType::WindowResized; }
	};

	class FocusSwitchedEvent : public Event
	{
	public:
		using Event::Event;
		EventType GetType() const override { return EventType::FocusSwitched; }
	};

	class CanvasViewChangedEvent : public Event
	{
	public:
		CanvasViewChangedEvent(Actor* eventRoot = nullptr, bool clampViewToElements = true) : Event(eventRoot),
			ClampViewToElements(clampViewToElements)
		{
		};
		bool ClampsViewToElements() const { return ClampViewToElements; }
		EventType GetType() const override { return EventType::CanvasViewChanged; }

	private:
		bool ClampViewToElements;
	};

	/**
	 * @brief General class for holding all events in a Game.
	*/
	class EventHolder
	{
	public:
		SharedPtr<Event> PollEvent(GameScene&);
		template <typename EventClass>
		void PushEvent(GameScene&, const EventClass& _event);
		template <typename EventClass>
		void PushEventInActiveScene(GameManager&, const EventClass& _event);

	private:
		std::map<GameScene*, std::queue<SharedPtr<Event>>> Events;
	};

	/**
	 * @brief Class for pushing Events by clients of the engine.
	*/
	class EventPusher
	{
	public:
		template <typename EventClass>
		void PushEvent(const EventClass& _event)
		{
			_EventHolder.PushEvent<EventClass>(Scene, _event);
		}
	private:
		EventPusher(GameScene& scene, EventHolder& evHolder) : Scene(scene), _EventHolder(evHolder) {}

		friend class Game;

		GameScene& Scene;
		EventHolder& _EventHolder;
	};

	template<typename EventClass>
	inline void EventHolder::PushEvent(GameScene& scene, const EventClass& _event)
	{
		Events[&scene].push(MakeShared<EventClass>(_event));
	}
	template<typename EventClass>
	inline void EventHolder::PushEventInActiveScene(GameManager& gameHandle, const EventClass& _event)
	{
		GEE_CORE_ASSERT(gameHandle.GetActiveScene());
		PushEvent<EventClass>(*gameHandle.GetActiveScene(), _event);
	}
}