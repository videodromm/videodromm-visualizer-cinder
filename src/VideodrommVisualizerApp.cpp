#include "VideodrommVisualizerApp.h"

void VideodrommVisualizerApp::prepare(Settings *settings)
{
	settings->setWindowSize(1024, 768);
	settings->setBorderless();
	settings->setWindowPos(0, 0);
}
void VideodrommVisualizerApp::setup()
{
	// maximize fps
	disableFrameRate();
	gl::enableVerticalSync(false);
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);

	// UI
	mVDSettings->mStandalone = true;
	mVDSettings->mCursorVisible = false;
	setUIVisibility(mVDSettings->mCursorVisible);
	mVDSession->getWindowsResolution();
	mVDUI = VDUI::create(mVDSettings, mVDSession);

	mouseGlobal = false;

	static float f = 0.0f;
	// mouse cursor and UI
	// render fbo
	gl::Fbo::Format fboFormat;
	mFbo = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFormat.colorTexture());
	// windows
	mIsShutDown = false;
	mIsResizing = true;
	mRenderWindowTimer = 0.0f;
	timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });

}
void VideodrommVisualizerApp::positionRenderWindow() {
	mVDSettings->iResolution.x = mVDSettings->mRenderWidth;
	mVDSettings->iResolution.y = mVDSettings->mRenderHeight;
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);//20141214 was 0
	setWindowPos(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);

}

void VideodrommVisualizerApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void VideodrommVisualizerApp::update()
{
	mVDSettings->iFps = getAverageFps();
	mVDSettings->sFps = toString(floor(mVDSettings->iFps));

}

void VideodrommVisualizerApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		ui::disconnectWindow(getWindow());
		ui::Shutdown();
		// save settings
		mVDSettings->save();
		mVDSession->save();
		quit();
	}
}

void VideodrommVisualizerApp::mouseMove(MouseEvent event)
{
	if (!mVDSession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}

void VideodrommVisualizerApp::mouseDown(MouseEvent event)
{
	if (!mVDSession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
	}
}

void VideodrommVisualizerApp::mouseDrag(MouseEvent event)
{
	if (!mVDSession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}
}

void VideodrommVisualizerApp::mouseUp(MouseEvent event)
{
	if (!mVDSession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void VideodrommVisualizerApp::keyDown(KeyEvent event)
{
	if (!mVDSession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_h:
			// mouse cursor and ui visibility
			mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
			setUIVisibility(mVDSettings->mCursorVisible);
			break;
		}
	}
}

void VideodrommVisualizerApp::keyUp(KeyEvent event)
{
	if (!mVDSession->handleKeyUp(event)) {
	}
}
void VideodrommVisualizerApp::resizeWindow()
{
	mVDUI->resize();
	mVDSession->resize();
}
void VideodrommVisualizerApp::fileDrop(FileDropEvent event)
{
	int index = (int)(event.getX() / (mVDSettings->uiElementWidth + mVDSettings->uiMargin));// +1;
	ci::fs::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	if (mVDSession->loadFileFromAbsolutePath(mFile, index) > -1) {

	}
}

void VideodrommVisualizerApp::draw()
{
	gl::clear(Color::black());
	//gl::setMatricesWindow(toPixels(getWindowSize()),false);
	gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, false);
	gl::draw(mVDSession->getMixTexture(), getWindowBounds());
	getWindow()->setTitle(mVDSettings->sFps + " fps Videodromm visualizer");
	// imgui
	if (!mVDSettings->mCursorVisible) return;

	mVDUI->Run("UI", (int)getAverageFps());
	if (mVDUI->isReady()) {
	}
}

CINDER_APP(VideodrommVisualizerApp, RendererGl, &VideodrommVisualizerApp::prepare)

