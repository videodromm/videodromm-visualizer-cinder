#include "VideodrommVisualizerApp.h"
/*
TODO
- 0204 blendmodes preview
- 0204 thread for loading image sequence
- 2802 list of shaders show/active on mouseover
- 2802 imgui contextual window depending on mouseover
- 2802 imgui vertical : textures
- warp select mix fbo texture
- flip horiz
- check flip H and V (spout also)
- sort fbo names and indexes (warps only 4 or 5 inputs)
- spout texture 10 create shader 10.glsl(ThemeFromBrazil) iChannel0
- warpwrapper handle texture mode 0 for spout (without fbo)
- put sliderInt instead of popups //warps next
- proper slitscan h and v //wip
- proper rotation

tempo 142
bpm = (60 * fps) / fpb

where bpm = beats per min
fps = frames per second
fpb = frames per beat

fpb = 4, bpm = 142
fps = 142 / 60 * 4 = 9.46
*/
void VideodrommVisualizerApp::prepare(Settings *settings) {
	settings->setWindowSize(1024, 768);
}
void VideodrommVisualizerApp::setup() {

	// Log
	mVDLog = VDLog::create();
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);
	// Utils
	mVDUtils = VDUtils::create(mVDSettings);
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDSession);
	// Message router
	mVDRouter = VDRouter::create(mVDSettings, mVDAnimation, mVDSession);
	// Mix

	mMixesFilepath = getAssetPath("") / mVDSettings->mAssetsPath / "mixes.xml";
	if (fs::exists(mMixesFilepath)) {
		// load textures from file if one exists
		mMixes = VDMix::readSettings(mVDSettings, mVDAnimation, mVDRouter, loadFile(mMixesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mMixes.push_back(VDMix::create(mVDSettings, mVDAnimation, mVDRouter));
	}
	mMixes[0]->setLeftFboIndex(2);
	mMixes[0]->setRightFboIndex(1);
	mVDAnimation->tapTempo();
	// UI
	mVDUI = VDUI::create(mVDSettings, mMixes[0], mVDRouter, mVDAnimation, mVDSession);
	setFrameRate(mVDSession->getTargetFps());
	// maximize fps
	disableFrameRate();
	gl::enableVerticalSync(false);

	mFadeInDelay = true;

#if (defined( CINDER_MSW )|| defined( CINDER_MAC ))
	mVDUtils->getWindowsResolution();
#endif

	// render fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mRenderFbo = gl::Fbo::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, format.colorTexture());
	mWarpFboIndex = 1;
	// UI fbo
	//mUIFbo = gl::Fbo::create(mVDSettings->mMainWindowWidth, mVDSettings->mMainWindowHeight, format.colorTexture());
	mUIFbo = gl::Fbo::create(1000, 800, format.colorTexture());

	// warping
	gl::enableDepthRead();
	gl::enableDepthWrite();
	// initialize warps
	mWarpSettings = getAssetPath("") / mVDSettings->mAssetsPath / "warps.xml";
	if (fs::exists(mWarpSettings)) {
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(mWarpSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}
	// load image
	try {
		mImage = gl::Texture::create(loadImage(loadAsset("help.jpg")),
			gl::Texture2d::Format().loadTopDown().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));

	}
	catch (const std::exception &e) {
		console() << e.what() << std::endl;
	}
	//Warp::setSize(mWarps, ivec2(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight));// create small new warps too
	Warp::setSize(mWarps, ivec2(mVDSettings->mFboWidth, mVDSettings->mFboHeight)); // create small new warps 
	Warp::handleResize(mWarps);

	// imgui
	margin = 3;
	inBetween = 3;
	// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15 mPreviewWidth = 160;mPreviewHeight = 120;
	w = mVDSettings->mPreviewFboWidth + margin;
	h = mVDSettings->mPreviewFboHeight * 2.3;
	largeW = (mVDSettings->mPreviewFboWidth + margin) * 4;
	largeH = (mVDSettings->mPreviewFboHeight + margin) * 5;
	largePreviewW = mVDSettings->mPreviewWidth + margin;
	largePreviewH = (mVDSettings->mPreviewHeight + margin) * 2.4;
	displayHeight = mVDSettings->mMainWindowHeight - 50;
	yPosRow1 = 100 + margin;
	yPosRow2 = yPosRow1 + largePreviewH + margin;
	yPosRow3 = yPosRow2 + h*1.4 + margin;

	mouseGlobal = false;
	if (mVDSettings->mStandalone) {
		mVDSettings->mCursorVisible = false;
	}
	else {
		mVDSettings->mCursorVisible = true;
	}
	// mouse cursor and ui
	setUIVisibility(mVDSettings->mCursorVisible);
}

void VideodrommVisualizerApp::cleanup() {
	CI_LOG_V("shutdown");
	ui::Shutdown();
	// save warp settings
	Warp::writeSettings(mWarps, writeFile(mWarpSettings));
	mVDSettings->save();
	mVDSession->save();
	quit();
}

void VideodrommVisualizerApp::resize() {
	CI_LOG_V("resizeWindow");
	mVDUI->resize();

	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);

	mVDSettings->iResolution.x = mVDSettings->mRenderWidth;
	mVDSettings->iResolution.y = mVDSettings->mRenderHeight;
}

void VideodrommVisualizerApp::mouseMove(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarps, event)) {
		// let your application perform its mouseMove handling here
	}
}

void VideodrommVisualizerApp::mouseDown(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarps, event)) {
		// let your application perform its mouseDown handling here
		mVDAnimation->controlValues[21] = event.getX() / getWindowWidth();
	}
}

void VideodrommVisualizerApp::mouseDrag(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarps, event)) {
		// let your application perform its mouseDrag handling here
	}
}

void VideodrommVisualizerApp::mouseUp(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarps, event)) {
		// let your application perform its mouseUp handling here
	}
}

void VideodrommVisualizerApp::keyDown(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarps, event)) {
		// warp editor did not handle the key, so handle it here
		if (!mVDAnimation->handleKeyDown(event)) {
			// Animation did not handle the key, so handle it here

			switch (event.getCode()) {
			case KeyEvent::KEY_ESCAPE:
				// quit the application
				quit();
				break;
			case KeyEvent::KEY_w:
				// toggle warp edit mode
				Warp::enableEditMode(!Warp::isEditModeEnabled());
				break;
			case KeyEvent::KEY_LEFT:
				//for (unsigned int i = 0; i < mVDImageSequences.size(); i++)
				//{
				//	(mVDTextures->getInputTexture(i)->pauseSequence();
				//	mVDSettings->iBeat--;
				//	// Seek to a new position in the sequence
				//	mImageSequencePosition = (mVDTextures->getInputTexture(i)->getPlayheadPosition();
				//	(mVDTextures->getInputTexture(i)->setPlayheadPosition(--mImageSequencePosition);
				//}
				break;
			case KeyEvent::KEY_RIGHT:
				//for (unsigned int i = 0; i < mVDImageSequences.size(); i++)
				//{
				//	(mVDTextures->getInputTexture(i)->pauseSequence();
				//	mVDSettings->iBeat++;
				//	// Seek to a new position in the sequence
				//	mImageSequencePosition = (mVDTextures->getInputTexture(i)->getPlayheadPosition();
				//	(mVDTextures->getInputTexture(i)->setPlayheadPosition(++mImageSequencePosition);
				//}
				break;
			case KeyEvent::KEY_0:
				mWarpFboIndex = 0;
				break;
			case KeyEvent::KEY_1:
				mWarpFboIndex = 1;
				break;
			case KeyEvent::KEY_2:
				mWarpFboIndex = 2;
				break;
			case KeyEvent::KEY_l:
				mVDAnimation->load();
				break;

			case KeyEvent::KEY_h:
				// mouse cursor
				mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
				setUIVisibility(mVDSettings->mCursorVisible);
				break;
			case KeyEvent::KEY_n:
				mWarps.push_back(WarpPerspectiveBilinear::create());
				break;
			case KeyEvent::KEY_a:
				fileWarpsName = "warps" + toString(getElapsedFrames()) + ".xml";
				mWarpSettings = getAssetPath("") / mVDSettings->mAssetsPath / fileWarpsName;
				Warp::writeSettings(mWarps, writeFile(mWarpSettings));
				mWarpSettings = getAssetPath("") / mVDSettings->mAssetsPath / "warps.xml";
				break;
			}
		}
	}
}

void VideodrommVisualizerApp::keyUp(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarps, event)) {
		// let your application perform its keyUp handling here
		if (!mVDAnimation->handleKeyUp(event)) {
			// Animation did not handle the key, so handle it here
		}
	}
}

void VideodrommVisualizerApp::update()
{
	mVDSettings->iFps = getAverageFps();
	mVDSettings->sFps = toString(floor(mVDSettings->iFps));
	mVDAnimation->update();
	mVDRouter->update();
	mMixes[0]->update();
	// check if a shader has been received from websockets
	if (mVDSettings->mShaderToLoad != "") {
		mMixes[0]->loadFboFragmentShader(mVDSettings->mShaderToLoad, 1);
	}

	updateWindowTitle();
}
void VideodrommVisualizerApp::fileDrop(FileDropEvent event)
{
	int index = (int)(event.getX() / (mVDSettings->uiElementWidth + mVDSettings->uiMargin));// +1;
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	if (mMixes[0]->loadFileFromAbsolutePath(mFile, index) > -1) {
		// load success
		// reset zoom
		mVDAnimation->controlValues[22] = 1.0f;
	}
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
// Render the scene into the FBO
void VideodrommVisualizerApp::renderSceneToFbo()
{
	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp(mRenderFbo);
	gl::clear(Color::black());
	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), mRenderFbo->getSize());
	// iterate over the warps and draw their content
	for (auto &warp : mWarps) {
		//warp->draw(mMixes[0]->getFboTexture(mWarpFboIndex), mMixes[0]->getFboTexture(mWarpFboIndex)->getBounds());
		warp->draw(mMixes[0]->getTexture(), mMixes[0]->getTexture()->getBounds());
	}
}

// Render the UI into the FBO
void VideodrommVisualizerApp::renderUIToFbo()
{
	if (mVDUI->isReady()) {
		// this will restore the old framebuffer binding when we leave this function
		// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
		// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
		gl::ScopedFramebuffer fbScp(mUIFbo);
		// setup the viewport to match the dimensions of the FBO
		gl::ScopedViewport scpVp(ivec2(0), ivec2(mVDSettings->mFboWidth * mVDSettings->mUIZoom, mVDSettings->mFboHeight * mVDSettings->mUIZoom));
		gl::clear();
		gl::color(Color::white());
#pragma region chain
		// left
		int t = 0;
		int fboIndex = mMixes[0]->getLeftFboIndex();

		ui::SetNextWindowSize(ImVec2(mVDSettings->uiLargePreviewW, mVDSettings->uiPreviewH));
		ui::SetNextWindowPos(ImVec2((t * (mVDSettings->uiLargePreviewW + mVDSettings->uiMargin)) + mVDSettings->uiMargin + mVDSettings->uiLargeW, mVDSettings->uiYPosRow2));
		ui::Begin("it a", NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
			ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
			ui::Image((void*)mMixes[0]->getInputTexture(mMixes[0]->getFboInputTextureIndex(fboIndex))->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
			ui::PopItemWidth();
		}
		ui::End();
		t++;
		ui::SetNextWindowSize(ImVec2(mVDSettings->uiLargePreviewW, mVDSettings->uiPreviewH));
		ui::SetNextWindowPos(ImVec2((t * (mVDSettings->uiLargePreviewW + mVDSettings->uiMargin)) + mVDSettings->uiMargin + mVDSettings->uiLargeW, mVDSettings->uiYPosRow2));
		ui::Begin(mMixes[0]->getFboLabel(fboIndex).c_str(), NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
			ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
			ui::Image((void*)mMixes[0]->getFboTexture(fboIndex)->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
			ui::PopItemWidth();
		}
		ui::End();
		t++;
		ui::SetNextWindowSize(ImVec2(mVDSettings->uiLargePreviewW, mVDSettings->uiPreviewH));
		ui::SetNextWindowPos(ImVec2((t * (mVDSettings->uiLargePreviewW + mVDSettings->uiMargin)) + mVDSettings->uiMargin + mVDSettings->uiLargeW, mVDSettings->uiYPosRow2));
		ui::Begin("f a", NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
			ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
			ui::Image((void*)mMixes[0]->getTexture(1)->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
			ui::PopItemWidth();
		}
		ui::End();


		//right
		t = 0;
		fboIndex = mMixes[0]->getRightFboIndex();

		ui::SetNextWindowSize(ImVec2(mVDSettings->uiLargePreviewW, mVDSettings->uiPreviewH));
		ui::SetNextWindowPos(ImVec2((t * (mVDSettings->uiLargePreviewW + mVDSettings->uiMargin)) + mVDSettings->uiMargin + mVDSettings->uiLargeW, mVDSettings->uiYPosRow2 + mVDSettings->uiPreviewH + mVDSettings->uiMargin));
		ui::Begin("it b", NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
			ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
			ui::Image((void*)mMixes[0]->getInputTexture(mMixes[0]->getFboInputTextureIndex(fboIndex))->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
			ui::PopItemWidth();
		}
		ui::End();
		t++;
		ui::SetNextWindowSize(ImVec2(mVDSettings->uiLargePreviewW, mVDSettings->uiPreviewH));
		ui::SetNextWindowPos(ImVec2((t * (mVDSettings->uiLargePreviewW + mVDSettings->uiMargin)) + mVDSettings->uiMargin + mVDSettings->uiLargeW, mVDSettings->uiYPosRow2 + mVDSettings->uiPreviewH + mVDSettings->uiMargin));
		ui::Begin(mMixes[0]->getFboLabel(fboIndex).c_str(), NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
			ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
			ui::Image((void*)mMixes[0]->getFboTexture(fboIndex)->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
			ui::PopItemWidth();
		}
		ui::End();
		t++;
		ui::SetNextWindowSize(ImVec2(mVDSettings->uiLargePreviewW, mVDSettings->uiPreviewH));
		ui::SetNextWindowPos(ImVec2((t * (mVDSettings->uiLargePreviewW + mVDSettings->uiMargin)) + mVDSettings->uiMargin + mVDSettings->uiLargeW, mVDSettings->uiYPosRow2 + mVDSettings->uiPreviewH + mVDSettings->uiMargin));
		ui::Begin("f b", NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
			ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
			ui::Image((void*)mMixes[0]->getTexture(2)->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
			ui::PopItemWidth();
		}
		ui::End();
#pragma endregion chain


		//gl::draw(mUIFbo->getColorTexture());
		/*gl::draw(mMixes[0]->getTexture(), Rectf(384, 128, 512, 256));
		gl::draw(mMixes[0]->getTexture(1), Rectf(512, 128, 640, 256));
		gl::draw(mMixes[0]->getTexture(2), Rectf(640, 128, 768, 256));*/
	}
	showVDUI((int)getAverageFps());
}
void VideodrommVisualizerApp::draw()
{

	/* TODO check for single screen*/
	if (mFadeInDelay) {
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			if (mVDSettings->mDisplayCount > 1) {
				int uiWidth = (int)mVDSettings->mMainWindowWidth / 2;
				setWindowSize(mVDSettings->mRenderWidth + uiWidth, mVDSettings->mRenderHeight + 30);
				setWindowPos(ivec2(mVDSettings->mRenderX - uiWidth, 0));

			}
			else {
				setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
				setWindowPos(ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY));

			}
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	renderSceneToFbo();
	gl::clear(Color::black());
	gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, false);
	gl::draw(mRenderFbo->getColorTexture());

	//imgui
	if (!mVDSettings->mCursorVisible || Warp::isEditModeEnabled())
	{
		return;
	}
	renderUIToFbo();
	gl::draw(mUIFbo->getColorTexture());

}

void VideodrommVisualizerApp::updateWindowTitle()
{
	getWindow()->setTitle("(" + mVDSettings->sFps + " fps) " + toString(mVDSettings->iBeat) + " Videodromm");
}
// UI
void VideodrommVisualizerApp::showVDUI(unsigned int fps) {
	mVDUI->Run("UI", fps);
}
// If you're deploying to iOS, set the Render antialiasing to 0 for a significant
// performance improvement. This value defaults to 4 (AA_MSAA_4) on iOS and 16 (AA_MSAA_16)
// on the Desktop.
#if defined( CINDER_COCOA_TOUCH )
CINDER_APP(VideodrommVisualizerApp, RendererGl(RendererGl::AA_NONE))
#else
CINDER_APP(VideodrommVisualizerApp, RendererGl(RendererGl::Options().msaa(8)), &VideodrommVisualizerApp::prepare)
#endif
