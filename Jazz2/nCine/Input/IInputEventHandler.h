#pragma once

#include "IInputManager.h"

namespace nCine
{
	/// The interface class for handling input events from keyboard, screen touches, mouse, accelerometer and joystick
	class IInputEventHandler
	{
	public:
		IInputEventHandler() {
			IInputManager::setHandler(this);
		}
		inline virtual ~IInputEventHandler() {}

		/// Callback function called every time a key is pressed
		inline virtual void onKeyPressed(const KeyboardEvent& event) {}
		/// Callback function called every time a key is released
		inline virtual void onKeyReleased(const KeyboardEvent& event) {}
		/// Callback function called every time a text input is generated
		inline virtual void onTextInput(const TextInputEvent& event) {}
		/// Callback function called every time a touch event is made
		inline virtual void onTouchEvent(const TouchEvent& event) {}
#if defined(DEATH_TARGET_ANDROID)
		/// Callback function called at fixed time with the updated reading from the accelerometer sensor
		inline virtual void onAcceleration(const AccelerometerEvent& event) {}
#endif
		/// Callback function called every time a mouse button is pressed
		inline virtual void onMouseButtonPressed(const MouseEvent& event) {}
		/// Callback function called every time a mouse button is released
		inline virtual void onMouseButtonReleased(const MouseEvent& event) {}
		/// Callback function called every time the mouse is moved
		inline virtual void onMouseMoved(const MouseState& state) {}
		/// Callback function called every time a scroll input occurs (mouse wheel, touchpad gesture, etc.)
		inline virtual void onScrollInput(const ScrollEvent& event) {}

		/// Callback function called every time a joystick button is pressed
		inline virtual void onJoyButtonPressed(const JoyButtonEvent& event) {}
		/// Callback function called every time a joystick button is released
		inline virtual void onJoyButtonReleased(const JoyButtonEvent& event) {}
		/// Callback function called every time a joystick hat is moved
		inline virtual void onJoyHatMoved(const JoyHatEvent& event) {}
		/// Callback function called every time a joystick axis is moved
		inline virtual void onJoyAxisMoved(const JoyAxisEvent& event) {}

		/// Callback function called every time a button of a joystick with mapping is pressed
		inline virtual void onJoyMappedButtonPressed(const JoyMappedButtonEvent& event) {}
		/// Callback function called every time a button of a joystick with mapping is released
		inline virtual void onJoyMappedButtonReleased(const JoyMappedButtonEvent& event) {}
		/// Callback function called every time an axis of a joystick with mapping is moved
		inline virtual void onJoyMappedAxisMoved(const JoyMappedAxisEvent& event) {}

		/// Callback function called every time a joystick is connected
		inline virtual void onJoyConnected(const JoyConnectionEvent& event) {}
		/// Callback function called every time a joystick is disconnected
		inline virtual void onJoyDisconnected(const JoyConnectionEvent& event) {}

		/// Callback function called when the system sends a quit event, for example when the user clicks the window close button
		/*! \returns True if the application should quit */
		inline virtual bool onQuitRequest() {
			return true;
		}
	};

}
