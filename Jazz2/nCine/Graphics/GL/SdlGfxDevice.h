#pragma once

#if defined(WITH_SDL)

#if defined(_MSC_VER) && defined(__has_include)
#	if __has_include("../../../../Libs/SDL2/SDL.h")
#		define __HAS_LOCAL_SDL
#	endif
#endif
#ifdef __HAS_LOCAL_SDL
#	include "../../../../Libs/SDL2/SDL.h"
#else
#	include <SDL.h>
#endif

#include "../IGfxDevice.h"
#include "../../Primitives/Vector2.h"
#include "../DisplayMode.h"

namespace nCine {

	/// The SDL based graphics device
	class SdlGfxDevice : public IGfxDevice
	{
	public:
		SdlGfxDevice(const WindowMode& windowMode, const GLContextInfo& glContextInfo, const DisplayMode& displayMode);
		~SdlGfxDevice() override;

		void setSwapInterval(int interval) override;

		void setResolution(bool fullscreen, int width = 0, int height = 0) override;

		inline void update() override {
			SDL_GL_SwapWindow(windowHandle_);
		}

		inline void setWindowPosition(int x, int y) override {
			SDL_SetWindowPosition(windowHandle_, x, y);
		}
		inline void setWindowTitle(const StringView& windowTitle) override {
			SDL_SetWindowTitle(windowHandle_, String::nullTerminatedView(windowTitle).data());
		}
		void setWindowIcon(const StringView& windowIconFilename) override;

		int windowPositionX() const override;
		int windowPositionY() const override;
		const Vector2i windowPosition() const override;

		void flashWindow() const override;

		int windowMonitorIndex() const override;

		const VideoMode& currentVideoMode(unsigned int monitorIndex) const override;
		bool setVideoMode(unsigned int modeIndex) override;

		static inline SDL_Window* windowHandle() {
			return windowHandle_;
		}

	protected:
		void setResolutionInternal(int width, int height) override;

	private:
		/// SDL2 window handle
		static SDL_Window* windowHandle_;
		/// SDL2 OpenGL context handle
		SDL_GLContext glContextHandle_;

		/// Deleted copy constructor
		SdlGfxDevice(const SdlGfxDevice&) = delete;
		/// Deleted assignment operator
		SdlGfxDevice& operator=(const SdlGfxDevice&) = delete;

		/// Initilizes the video subsystem (SDL)
		void initGraphics();
		/// Initilizes the OpenGL graphic context
		void initDevice(bool isFullscreen, bool isResizable);

		void updateMonitors() override;

		void convertVideoModeInfo(const SDL_DisplayMode& sdlVideoMode, IGfxDevice::VideoMode& videoMode) const;

		friend class SdlInputManager;
	};
}

#endif