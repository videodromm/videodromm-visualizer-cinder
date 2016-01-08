#include "VideoDrommVisualizerApp.h"

void VideoDrommVisualizerApp::prepareSettings(Settings *settings)
{
	
}
void VideoDrommVisualizerApp::setup()
{
	// parameters
	mVDSettings = VDSettings::create();
	mVDSettings->mLiveCode = true;
	mVDSettings->mRenderThumbs = false;
	loadShader(getAssetPath("default.fs"));
	// utils
	mVDUtils = VDUtils::create(mVDSettings);

	mVDSettings->iResolution.x = mVDSettings->mRenderWidth;
	mVDSettings->iResolution.y = mVDSettings->mRenderHeight;
	mVDSettings->mRenderResolution = ivec2(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
	CI_LOG_V("createRenderWindow, resolution:" + toString(mVDSettings->iResolution.x) + "x" + toString(mVDSettings->iResolution.y));
	mVDSettings->mRenderResoXY = vec2(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);

	// instanciate the console class
	mConsole = AppConsole::create(mVDSettings, mVDUtils);

	// MPE
	mServerFramesProcessed = 0;
	mClient = MPEClient::create(this, USE_THREADED);

	// imgui
	showConsole = true;
	ui::initialize(ui::Options().autoRender(false));
	updateWindowTitle();
	// initialize warps
	mSettings = getAssetPath("") / "warps.xml";
	if (fs::exists(mSettings)) {
		// load warp settings from file if one exists
		mWarps = Warp::readSettings(loadFile(mSettings));
	}
	else {
		// otherwise create a warp from scratch
		mWarps.push_back(WarpBilinear::create());
		mWarps.push_back(WarpPerspective::create());
		mWarps.push_back(WarpPerspectiveBilinear::create());
	}

	// load test image
	try {
		mImage = gl::Texture::create(loadImage(loadAsset("help.jpg")),
			gl::Texture2d::Format().loadTopDown().mipmap(true).minFilter(GL_LINEAR_MIPMAP_LINEAR));

		//mSrcArea = mImage->getBounds();

		// adjust the content size of the warps
		Warp::setSize(mWarps, mImage->getSize());
	}
	catch (const std::exception &e) {
		console() << e.what() << std::endl;
	}
	// needed?
	mMesh = gl::VboMesh::create(geom::Rect());
}

void VideoDrommVisualizerApp::mpeReset()
{
    // Reset the state of your app.
    // This will be called when any client connects.
	mServerFramesProcessed = 0;
}

void VideoDrommVisualizerApp::update()
{
    if (!mClient->isConnected() && (getElapsedFrames() % 60) == 0)
    {
        mClient->start();
    }
	mVDSettings->iFps = getAverageFps();
	mVDSettings->sFps = toString(floor(mVDSettings->iFps));
	if (mVDSettings->iGreyScale)
	{
		mVDSettings->controlValues[1] = mVDSettings->controlValues[2] = mVDSettings->controlValues[3];
		mVDSettings->controlValues[5] = mVDSettings->controlValues[6] = mVDSettings->controlValues[7];
	}

	mVDSettings->iChannelTime[0] = getElapsedSeconds();
	mVDSettings->iChannelTime[1] = getElapsedSeconds() - 1;
	mVDSettings->iChannelTime[3] = getElapsedSeconds() - 2;
	mVDSettings->iChannelTime[4] = getElapsedSeconds() - 3;
	//
	if (mVDSettings->mUseTimeWithTempo)
	{
		mVDSettings->iGlobalTime = mVDSettings->iTempoTime*mVDSettings->iTimeFactor;
	}
	else
	{
		mVDSettings->iGlobalTime = getElapsedSeconds();
	}
	mVDSettings->iGlobalTime *= mVDSettings->iSpeedMultiplier;
	mVDUtils->update();
	mProg->uniform("iGlobalTime", static_cast<float>(getElapsedSeconds()));
	//mProg->uniform("iMouse", mMouseCoord);
	//mProg->uniform("iChannel0", 0);

}

void VideoDrommVisualizerApp::mpeFrameUpdate(long serverFrameNumber)
{
	mServerFramesProcessed = serverFrameNumber;
}

void VideoDrommVisualizerApp::draw()
{
    mClient->draw();
	gl::enableAlphaBlending();
	//gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
	if (mImage) {
		// iterate over the warps and draw their content
		for (auto &warp : mWarps) {
			
			gl::draw(mImage);
				
		}
	}
	gl::disableAlphaBlending();
	//imgui
	ui::NewFrame();

	gl::setMatricesWindow(getWindowSize());
	xPos = margin;
	yPos = margin + 30;
	// console
	if (showConsole)
	{
		ui::SetNextWindowSize(ImVec2((w + margin) * mVDSettings->MAX, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPos), ImGuiSetCond_Once);
		ShowAppConsole(&showConsole);
		if (mVDSettings->newMsg)
		{
			mVDSettings->newMsg = false;
			mConsole->AddLog(mVDSettings->mMsg.c_str());
		}
	}
	gl::draw(mMesh);

}

void VideoDrommVisualizerApp::mpeFrameRender(bool isNewFrame)
{
    gl::clear(Color(0.5,0.5,0.5));
    // Your render code.
}
// From imgui by Omar Cornut
void VideoDrommVisualizerApp::ShowAppConsole(bool* opened)
{
	mConsole->Run("Console", opened);
}
void VideoDrommVisualizerApp::resize()
{
	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);
}

void VideoDrommVisualizerApp::mouseMove(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseMove(mWarps, event)) {
		// let your application perform its mouseMove handling here
	}
}

void VideoDrommVisualizerApp::mouseDown(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDown(mWarps, event)) {
		// let your application perform its mouseDown handling here
	}
}

void VideoDrommVisualizerApp::mouseDrag(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseDrag(mWarps, event)) {
		// let your application perform its mouseDrag handling here
	}
}

void VideoDrommVisualizerApp::mouseUp(MouseEvent event)
{
	// pass this mouse event to the warp editor first
	if (!Warp::handleMouseUp(mWarps, event)) {
		// let your application perform its mouseUp handling here
	}
}

void VideoDrommVisualizerApp::keyDown(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyDown(mWarps, event)) {
		// warp editor did not handle the key, so handle it here
		switch (event.getCode()) {
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_f:
			// toggle full screen
			setFullScreen(!isFullScreen());
			break;
		case KeyEvent::KEY_v:
			// toggle vertical sync
			gl::enableVerticalSync(!gl::isVerticalSyncEnabled());
			break;
		case KeyEvent::KEY_w:
			// toggle warp edit mode
			Warp::enableEditMode(!Warp::isEditModeEnabled());
			break;
		}
	}
}

void VideoDrommVisualizerApp::keyUp(KeyEvent event)
{
	// pass this key event to the warp editor first
	if (!Warp::handleKeyUp(mWarps, event)) {
		// let your application perform its keyUp handling here
	}
}

void VideoDrommVisualizerApp::updateWindowTitle()
{
	getWindow()->setTitle("(" + mVDSettings->sFps + " fps) " + toString(mServerFramesProcessed));
}


void VideoDrommVisualizerApp::cleanup()
{
	// save warp settings
	Warp::writeSettings(mWarps, writeFile(mSettings));
	CI_LOG_V("shutdown");

	// save warp settings
	//mBatchass->getWarpsRef()->save();
	// save params
	mVDSettings->save();
	ui::Shutdown();
}
void VideoDrommVisualizerApp::fileDrop(FileDropEvent event)
{
	int index;
	string ext = "";
	// use the last of the dropped files
	const fs::path &mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);
	index = (int)(event.getX() / (margin + mVDSettings->mPreviewFboWidth + inBetween));// +1;

	if (ext == "png" || ext == "jpg")
	{
		mImage = gl::Texture::create(loadImage(mFile));
	}

}
void VideoDrommVisualizerApp::loadShader(const fs::path &fragment_path)
{
	try
	{	// load and compile our shader
		mProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("default.vs"))
			.fragment(loadFile(fragment_path)));
		// no exceptions occurred, so store the shader's path for reloading on keypress
		//mCurrentShaderPath = fragment_path;
		mProg->uniform("iResolution", vec3(getWindowWidth(), getWindowHeight(), 0.0f));
		//const vector<Uniform> & unicorns = mProg->getActiveUniforms();
	}
	catch (ci::gl::GlslProgCompileExc &exc)
	{
		console() << "Error compiling shader: " << exc.what() << endl;
	}
	catch (ci::Exception &exc)
	{
		console() << "Error loading shader: " << exc.what() << endl;
	}
}
// If you're deploying to iOS, set the Render antialiasing to 0 for a significant
// performance improvement. This value defaults to 4 (AA_MSAA_4) on iOS and 16 (AA_MSAA_16)
// on the Desktop.
#if defined( CINDER_COCOA_TOUCH )
CINDER_APP( VideoDrommVisualizerApp, RendererGl(RendererGl::AA_NONE), VideoDrommVisualizerApp::prepareSettings )
#else
CINDER_APP(VideoDrommVisualizerApp, RendererGl, VideoDrommVisualizerApp::prepareSettings)
#endif
