#pragma once

namespace nCine
{
	class AppConfiguration;
	class Viewport;

	/// The interface class for handling nCine application events
	class IAppEventHandler
	{
	public:
		/// Pure virtual destructor in order to make the class abstract
		virtual ~IAppEventHandler() = 0;

		/// Called once before initialization to setup the application
		/*! \warning At this stage it is only safe to modify the `config` object.
		 *  No other engine API calls should be made. */
		virtual void onPreInit(AppConfiguration& config) { }
		/// Called once on application initialization
		virtual void onInit() { }
		/// Called at the start of each frame
		virtual void onFrameStart() { }
		/// Called every time the scenegraph has been traversed and all nodes have been transformed
		virtual void onPostUpdate() { }
		/// Called every time a viewport is going to be drawn
		virtual void onDrawViewport(Viewport& viewport) { }
		/// Called at the end of each frame, just before swapping buffers
		virtual void onFrameEnd() { }
		/// Called every time the window is resized (by the system or the user)
		virtual void onResizeWindow(int width, int height) { }
		/// Called every time fullscreen state of the window is changed
		virtual void onFullscreenChanged(bool isFullscreen) { }
		/// Called once on application shutdown
		virtual void onShutdown() { }
		/// Called every time the application needs to be suspended
		virtual void onSuspend() { }
		/// Called every time the application resumes from suspension
		virtual void onResume() { }
	};

	inline IAppEventHandler::~IAppEventHandler() { }

}
