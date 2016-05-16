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
void VideodrommVisualizerApp::prepare(Settings *settings)
{
	settings->setWindowSize(1024, 768);
}
void VideodrommVisualizerApp::setup()
{
	// Log
	mVDLog = VDLog::create();
	CI_LOG_V("Controller");
	// Settings
	mVDSettings = VDSettings::create();
	mVDSettings->mLiveCode = false;
	mVDSettings->mRenderThumbs = false;
	// Session
	mVDSession = VDSession::create(mVDSettings);
	// Utils
	mVDUtils = VDUtils::create(mVDSettings);
	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings, mVDSession);
	// Message router
	mVDRouter = VDRouter::create(mVDSettings, mVDAnimation, mVDSession);
	// Image sequence
	CI_LOG_V("Assets folder: " + mVDUtils->getPath("").string());
	string imgSeqPath = mVDSession->getImageSequencePath();

	// Mix
	mMixesFilepath = getAssetPath("") / "mixes.xml";
	if (fs::exists(mMixesFilepath)) {
		// load textures from file if one exists
		mMixes = VDMix::readSettings(loadFile(mMixesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mMixes.push_back(VDMix::create());
	}
	// Shaders
	//mVDShaders = VDShaders::create(mVDSettings);
	// Textures
	//mVDTextures = VDTextures::create(mVDSettings, mVDShaders, mVDAnimation);
	// try loading image sequence from dir
	//imgSeqFboIndex = mMixes[0]->loadImageSequence(1, mVDSession->getImageSequencePath());

	setFrameRate(mVDSession->getTargetFps());
	mFadeInDelay = true;
	mIsResizing = true;
	mVDUtils->getWindowsResolution();
	//setWindowSize(mVDSettings->mMainWindowWidth, mVDSettings->mMainWindowHeight);
	//mMainWindow = getWindow();
	//mMainWindow->setBorderless();
	//mMainWindow->setPos(0, 0);
	//getWindow()->getSignalResize().connect(std::bind(&VideodrommVisualizerApp::resizeWindow, this));
	//getWindow()->getSignalDraw().connect(std::bind(&VideodrommVisualizerApp::drawControlWindow, this));
	// render fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mRenderFbo = gl::Fbo::create(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, format.colorTexture());

	// UI fbo
	mUIFbo = gl::Fbo::create(mVDSettings->mMainWindowWidth, mVDSettings->mMainWindowHeight, format.colorTexture());

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
	Warp::setSize(mWarps, ivec2(mVDSettings->mFboWidth, mVDSettings->mFboHeight));

	mSaveThumbTimer = 0.0f;
	// movie
	mLoopVideo = false;

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
	removeUI = false;
	static float f = 0.0f;
	// mouse cursor
	if (mVDSettings->mCursorVisible)
	{
		hideCursor();
	}
	else
	{
		showCursor();
	}
	mVDAnimation->tapTempo();

	mVDUtils->getWindowsResolution();

	mVDSettings->iResolution.x = mVDSettings->mRenderWidth;
	mVDSettings->iResolution.y = mVDSettings->mRenderHeight;


}

void VideodrommVisualizerApp::cleanup()
{
	CI_LOG_V("shutdown");
	// save warp settings
	Warp::writeSettings(mWarps, writeFile(mWarpSettings));
	mVDSettings->save();
	mVDSession->save();
	ui::Shutdown();
	quit();
}

void VideodrommVisualizerApp::resizeWindow()
{
	mIsResizing = true;
	// disconnect ui window and io events callbacks
	ui::disconnectWindow(getWindow());

	// tell the warps our window has been resized, so they properly scale up or down
	Warp::handleResize(mWarps);

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
	fs::path moviePath;

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
			case KeyEvent::KEY_o:
				moviePath = getOpenFilePath();
				if (!moviePath.empty())
					loadMovieFile(moviePath);
				break;
			case KeyEvent::KEY_r:
				if (mMovie) mMovie.reset();
				break;
			case KeyEvent::KEY_p:
				if (mMovie) mMovie->play();
				/*for (unsigned int i = 0; i < mVDImageSequences.size(); i++)
				{
				(mVDTextures->getInputTexture(i)->playSequence();
				}*/
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
			case KeyEvent::KEY_SPACE:
				if (mMovie) { if (mMovie->isPlaying()) mMovie->stop(); else mMovie->play(); }
				break;
			case KeyEvent::KEY_l:
				mVDAnimation->load();
				mLoopVideo = !mLoopVideo;
				if (mMovie) mMovie->setLoop(mLoopVideo);
				break;
			case KeyEvent::KEY_c:
				// mouse cursor
				mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
				if (mVDSettings->mCursorVisible)
				{
					hideCursor();
				}
				else
				{
					showCursor();
				}
				break;
			case KeyEvent::KEY_h:
				removeUI = !removeUI;
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
void VideodrommVisualizerApp::loadMovieFile(const fs::path &moviePath)
{
	try {
		mMovie.reset();
		// load up the movie, set it to loop, and begin playing
		mMovie = qtime::MovieGlHap::create(moviePath);
		mLoopVideo = (mMovie->getDuration() < 30.0f);
		mMovie->setLoop(mLoopVideo);
		mMovie->play();
	}
	catch (ci::Exception &e)
	{
		console() << string(e.what()) << std::endl;
		console() << "Unable to load the movie." << std::endl;
	}

}
void VideodrommVisualizerApp::update()
{
	mVDSettings->iFps = getAverageFps();
	mVDSettings->sFps = toString(floor(mVDSettings->iFps));
	mVDAnimation->update();

	mVDRouter->update();
	updateWindowTitle();

	renderSceneToFbo();
}
void VideodrommVisualizerApp::fileDrop(FileDropEvent event)
{
	int index = 1;
	string ext = "";
	// use the last of the dropped files
	boost::filesystem::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);

	if (ext == "wav" || ext == "mp3")
	{
	}
	else if (ext == "png" || ext == "jpg")
	{
		if (index < 1) index = 1;
		if (index > 3) index = 3;
		//mTextures->loadImageFile(mParameterBag->currentSelectedIndex, mFile);
		mMixes[0]->loadImageFile(mFile, 0, 0, true);
	}
	else if (ext == "glsl")
	{
		int rtn = mMixes[0]->loadFboFragmentShader(mFile, true);//right = true
		
		if (rtn > -1 )
		{
			// reset zoom
			mVDSettings->controlValues[22] = 1.0f;

			// save thumb
			timeline().apply(&mSaveThumbTimer, 1.0f, 1.0f).finishFn([&]{ saveThumb(); });
		}
	}
	else if (ext == "xml")
	{
	}
	else if (ext == "mov")
	{
		loadMovieFile(mFile);
	}
	else if (ext == "txt")
	{
	}
	else if (ext == "")
	{
		// try loading image sequence from dir
		//imgSeqFboIndex = mMixes[0]->loadImageSequence(index, mFile);
	}

}
void VideodrommVisualizerApp::saveThumb()
{
	/* TODO
	string filename;
	try
	{
	filename = mVDShaders->getFragFileName() + ".jpg";
	writeImage(getAssetPath("") / "thumbs" / filename, mVDTextures)->getFboTexture(mParameterBag->mCurrentPreviewFboIndex));
	CI_LOG_V("saved:" + filename);

	}
	catch (const std::exception &e)
	{
	CI_LOG_V("unable to save:" + filename + string(e.what()));
	}*/
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
	if (mMovie) {
		if (mMovie->isPlaying()) mMovie->draw();
	}
	gl::draw(mMixes[0]->getRightFboTexture(), Rectf(0, 0, 128, 128));
	gl::draw(mMixes[0]->getLeftFboTexture(), Rectf(128, 0, 256, 128));
	gl::draw(mMixes[0]->getFboTexture(0), Rectf(256, 0, 384, 128));
	gl::draw(mMixes[0]->getFboTexture(1), Rectf(384, 0, 512, 128));
	gl::draw(mMixes[0]->getFboInputTexture(0, 0), Rectf(0, 128, 128, 256));
	gl::draw(mMixes[0]->getTexture(), Rectf(128, 128, 256, 256));


	int i = 0;
	// iterate over the warps and draw their content
	for (auto &warp : mWarps) {
		warp->draw(mMixes[0]->getFboTexture(i), mMixes[0]->getFboTexture(i)->getBounds());
	}
}

// Render the UI into the FBO
void VideodrommVisualizerApp::renderUIToFbo()
{
	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp(mUIFbo);
	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), ivec2(mVDSettings->mFboWidth * mVDSettings->mUIZoom, mVDSettings->mFboHeight * mVDSettings->mUIZoom));
	gl::clear();
	gl::color(Color::white());
	gl::setMatricesWindow(500, 400);
	// imgui
	static int currentWindowRow1 = 0;
	static int currentWindowRow2 = 0;
	static int currentWindowRow3 = 0;

	xPos = margin;
	const char* warpInputs[] = { "mix", "left", "right", "warp1", "warp2", "preview", "abp", "live", "w8", "w9", "w10", "w11", "w12", "w13", "w14", "w15" };

#pragma region Info

	ui::SetNextWindowSize(ImVec2(1000, 100), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, margin), ImGuiSetCond_Once);
	sprintf_s(buf, "Videodromm Fps %c %d###fps", "|/-\\"[(int)(ImGui::GetTime() / 0.25f) & 3], (int)mVDSettings->iFps);
	ui::Begin(buf);
	{
		ImGui::PushItemWidth(mVDSettings->mPreviewFboWidth);

		ImGui::RadioButton("Textures", &currentWindowRow2, 0); ImGui::SameLine();
		ImGui::RadioButton("Fbos", &currentWindowRow2, 1); ImGui::SameLine();
		ImGui::RadioButton("Shaders", &currentWindowRow2, 2); ImGui::SameLine();

		ImGui::RadioButton("Osc", &currentWindowRow3, 0); ImGui::SameLine();
		ImGui::RadioButton("Midi", &currentWindowRow3, 1); ImGui::SameLine();
		ImGui::RadioButton("Chn", &currentWindowRow3, 2); ui::SameLine();
		ImGui::RadioButton("Blend", &currentWindowRow3, 3); ui::SameLine();

		if (ui::Button("Save Params"))
		{
			// save warp settings
			Warp::writeSettings(mWarps, writeFile("warps1.xml"));
			// save params
			mVDSettings->save();
		}
		ui::SameLine();

		ui::Text("Msg: %s", mVDSettings->mMsg.c_str());
#pragma region Audio

		if (ui::Button("x##spdx")) { mVDSettings->iSpeedMultiplier = 1.0; }
		ui::SameLine();
		ui::SliderFloat("speed x", &mVDSettings->iSpeedMultiplier, 0.01f, 5.0f, "%.1f");
		ui::SameLine();
		ui::Text("Beat %d ", mVDSettings->iBeat);
		ui::SameLine();
		ui::Text("Beat Idx %d ", mVDAnimation->iBeatIndex);
		//ui::SameLine();
		//ui::Text("Bar %d ", mVDAnimation->iBar);
		ui::SameLine();

		if (ui::Button("x##bpbx")) { mVDSession->iBeatsPerBar = 1; }
		ui::SameLine();
		ui::SliderInt("beats per bar", &mVDSession->iBeatsPerBar, 1, 8);

		ui::SameLine();
		ui::Text("Time %.2f", mVDSettings->iGlobalTime);
		ui::SameLine();
		ui::Text("Trk %s %.2f", mVDSettings->mTrackName.c_str(), mVDSettings->liveMeter);
		//			ui::Checkbox("Playing", &mVDSettings->mIsPlaying);
		ui::SameLine();
		mVDSettings->iDebug ^= ui::Button("Debug");
		ui::SameLine();

		ui::Text("Tempo %.2f ", mVDSession->getBpm());
		ui::Text("Target FPS %.2f ", mVDSession->getTargetFps());
		ui::SameLine();
		if (ui::Button("Tap tempo")) { mVDAnimation->tapTempo(); }
		ui::SameLine();
		if (ui::Button("Time tempo")) { mVDAnimation->mUseTimeWithTempo = !mVDAnimation->mUseTimeWithTempo; }
		ui::SameLine();

		//void Batchass::setTimeFactor(const int &aTimeFactor)
		ui::SliderFloat("time x", &mVDAnimation->iTimeFactor, 0.0001f, 1.0f, "%.01f");
		ui::SameLine();

		static ImVector<float> timeValues; if (timeValues.empty()) { timeValues.resize(40); memset(&timeValues.front(), 0, timeValues.size()*sizeof(float)); }
		static int timeValues_offset = 0;
		// audio maxVolume
		static float tRefresh_time = -1.0f;
		if (ui::GetTime() > tRefresh_time + 1.0f / 20.0f)
		{
			tRefresh_time = ui::GetTime();
			timeValues[timeValues_offset] = mVDSettings->maxVolume;
			timeValues_offset = (timeValues_offset + 1) % timeValues.size();
		}

		ui::SliderFloat("mult x", &mVDSettings->controlValues[13], 0.01f, 10.0f);
		ui::SameLine();
		//ImGui::PlotHistogram("Histogram", mMixes[0]->getSmallSpectrum(), 7, 0, NULL, 0.0f, 255.0f, ImVec2(0, 30));
		//ui::SameLine();

		if (mVDSettings->maxVolume > 240.0) ui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		ui::PlotLines("Volume", &timeValues.front(), (int)timeValues.size(), timeValues_offset, toString(mVDUtils->formatFloat(mVDSettings->maxVolume)).c_str(), 0.0f, 255.0f, ImVec2(0, 30));
		if (mVDSettings->maxVolume > 240.0) ui::PopStyleColor();
		ui::SameLine();
		// fps
		static ImVector<float> values; if (values.empty()) { values.resize(100); memset(&values.front(), 0, values.size()*sizeof(float)); }
		static int values_offset = 0;
		static float refresh_time = -1.0f;
		if (ui::GetTime() > refresh_time + 1.0f / 6.0f)
		{
			refresh_time = ui::GetTime();
			values[values_offset] = mVDSettings->iFps;
			values_offset = (values_offset + 1) % values.size();
		}
		if (mVDSettings->iFps < 12.0) ui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		ui::PlotLines("FPS", &values.front(), (int)values.size(), values_offset, mVDSettings->sFps.c_str(), 0.0f, mVDSession->getTargetFps(), ImVec2(0, 30));
		if (mVDSettings->iFps < 12.0) ui::PopStyleColor();


#pragma endregion Audio

		ui::PopItemWidth();
	}
	ui::End();
	xPos = margin + 1000;

#pragma endregion Info

#pragma region Global

	ui::SetNextWindowSize(ImVec2(largeW, displayHeight), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, margin), ImGuiSetCond_Once);
	ui::Begin("Animation");
	{
		ImGui::PushItemWidth(mVDSettings->mPreviewFboWidth);

		if (ui::CollapsingHeader("Mouse", NULL, true, true))
		{
			ui::Text("Mouse Position: (%.1f,%.1f)", ui::GetIO().MousePos.x, ui::GetIO().MousePos.y); ui::SameLine();
			ui::Text("Clic %d", ui::GetIO().MouseDown[0]);
			mouseGlobal ^= ui::Button("mouse gbl");
			if (mouseGlobal)
			{
				mVDSettings->mRenderPosXY.x = ui::GetIO().MousePos.x; ui::SameLine();
				mVDSettings->mRenderPosXY.y = ui::GetIO().MousePos.y;
				mVDSettings->iMouse.z = ui::GetIO().MouseDown[0];
			}
			else
			{

				mVDSettings->iMouse.z = ui::Button("mouse click");
			}
			ui::SliderFloat("MouseX", &mVDSettings->mRenderPosXY.x, 0, mVDSettings->mFboWidth);
			ui::SliderFloat("MouseY", &mVDSettings->mRenderPosXY.y, 0, 2048);// mVDSettings->mFboHeight);

		}
		if (ui::CollapsingHeader("Effects", NULL, true, true))
		{
			int hue = 0;

			(mVDSettings->iRepeat) ? ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(1.0f, 0.1f, 0.1f));
			ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			mVDSettings->iRepeat ^= ui::Button("repeat");
			ui::PopStyleColor(3);
			hue++;
			ui::SameLine();

			(mVDSettings->controlValues[45]) ? ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(1.0f, 0.1f, 0.1f));
			ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			if (ui::Button("glitch")) { mVDSettings->controlValues[45] = !mVDSettings->controlValues[45]; }
			ui::PopStyleColor(3);
			hue++;
			ui::SameLine();

			(mVDSettings->controlValues[46]) ? ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(1.0f, 0.1f, 0.1f));
			ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			if (ui::Button("toggle")) { mVDSettings->controlValues[46] = !mVDSettings->controlValues[46]; }
			ui::PopStyleColor(3);
			hue++;
			ui::SameLine();

			(mVDSettings->controlValues[47]) ? ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(1.0f, 0.1f, 0.1f));
			ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			if (ui::Button("vignette")) { mVDSettings->controlValues[47] = !mVDSettings->controlValues[47]; }
			ui::PopStyleColor(3);
			hue++;

			(mVDSettings->controlValues[48]) ? ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(1.0f, 0.1f, 0.1f));
			ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			if (ui::Button("invert")) { mVDSettings->controlValues[48] = !mVDSettings->controlValues[48]; }
			ui::PopStyleColor(3);
			hue++;
			ui::SameLine();

			(mVDSettings->iGreyScale) ? ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue / 7.0f, 1.0f, 0.5f)) : ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(1.0f, 0.1f, 0.1f));
			ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue / 7.0f, 0.7f, 0.7f));
			ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue / 7.0f, 0.8f, 0.8f));
			mVDSettings->iGreyScale ^= ui::Button("greyscale");
			ui::PopStyleColor(3);
			hue++;
			ui::SameLine();

			if (ui::Button("blackout"))
			{
				mVDSettings->controlValues[1] = mVDSettings->controlValues[2] = mVDSettings->controlValues[3] = mVDSettings->controlValues[4] = 0.0;
				mVDSettings->controlValues[5] = mVDSettings->controlValues[6] = mVDSettings->controlValues[7] = mVDSettings->controlValues[8] = 0.0;
			}
		}
		if (ui::CollapsingHeader("Animation", NULL, true, true))
		{

			ui::SliderInt("UI Zoom", &mVDSettings->mUIZoom, 1, 8);
			int ctrl;
			stringstream aParams;
			aParams << "{\"params\" :[{\"name\" : 0,\"value\" : " << getElapsedFrames() << "}"; // TimeStamp

			// iChromatic
			ctrl = 10;
			if (ui::Button("a##chromatic")) { mVDAnimation->lockChromatic(); }
			ui::SameLine();
			if (ui::Button("t##chromatic")) { mVDAnimation->tempoChromatic(); }
			ui::SameLine();
			if (ui::Button("x##chromatic")) { mVDAnimation->resetChromatic(); }
			ui::SameLine();
			if (ui::SliderFloat("chromatic/min/max", &mVDSettings->controlValues[ctrl], mVDAnimation->minChromatic, mVDAnimation->maxChromatic))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}

			// ratio
			ctrl = 11;
			if (ui::Button("a##ratio")) { mVDAnimation->lockRatio(); }
			ui::SameLine();
			if (ui::Button("t##ratio")) { mVDAnimation->tempoRatio(); }
			ui::SameLine();
			if (ui::Button("x##ratio")) { mVDAnimation->resetRatio(); }
			ui::SameLine();
			if (ui::SliderFloat("ratio/min/max", &mVDSettings->controlValues[ctrl], mVDAnimation->minRatio, mVDAnimation->maxRatio))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}
			// exposure
			ctrl = 14;
			if (ui::Button("a##exposure")) { mVDAnimation->lockExposure(); }
			ui::SameLine();
			if (ui::Button("t##exposure")) { mVDAnimation->tempoExposure(); }
			ui::SameLine();
			if (ui::Button("x##exposure")) { mVDAnimation->resetExposure(); }
			ui::SameLine();
			if (ui::DragFloat("exposure", &mVDSettings->controlValues[ctrl], 0.1f, mVDAnimation->minExposure, mVDAnimation->maxExposure))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
				mVDAnimation->setExposure(mVDSettings->controlValues[ctrl]);
			}
			// zoom
			ctrl = 22;
			if (ui::Button("a##zoom"))
			{
				mVDAnimation->lockZoom();
			}
			ui::SameLine();
			if (ui::Button("t##zoom")) { mVDAnimation->tempoZoom(); }
			ui::SameLine();
			if (ui::Button("x##zoom")) { mVDAnimation->resetZoom(); }
			ui::SameLine();
			if (ui::DragFloat("zoom", &mVDSettings->controlValues[ctrl], 0.1f, mVDAnimation->minZoom, mVDAnimation->maxZoom))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}
			// z position
			ctrl = 9;
			if (ui::Button("a##zpos")) { mVDAnimation->lockZPos(); }
			ui::SameLine();
			if (ui::Button("t##zpos")) { mVDAnimation->tempoZPos(); }
			ui::SameLine();
			if (ui::Button("x##zpos")) { mVDAnimation->resetZPos(); }
			ui::SameLine();
			if (ui::SliderFloat("zPosition", &mVDSettings->controlValues[ctrl], mVDAnimation->minZPos, mVDAnimation->maxZPos))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}

			// rotation speed 
			ctrl = 19;
			if (ui::Button("a##rotationspeed")) { mVDAnimation->lockRotationSpeed(); }
			ui::SameLine();
			if (ui::Button("t##rotationspeed")) { mVDAnimation->tempoRotationSpeed(); }
			ui::SameLine();
			if (ui::Button("x##rotationspeed")) { mVDAnimation->resetRotationSpeed(); }
			ui::SameLine();
			if (ui::DragFloat("rotationSpeed", &mVDSettings->controlValues[ctrl], 0.01f, mVDAnimation->minRotationSpeed, mVDAnimation->maxRotationSpeed))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}
			// badTv
			/*if (ui::Button("x##badtv")) { mVDSettings->iBadTv = 0.0f; }
			ui::SameLine();
			if (ui::SliderFloat("badTv/min/max", &mVDSettings->iBadTv, 0.0f, 5.0f))
			{
			}*/
			// param1
			if (ui::Button("x##param1")) { mVDSettings->iParam1 = 1.0f; }
			ui::SameLine();
			if (ui::SliderFloat("param1/min/max", &mVDSettings->iParam1, 0.01f, 100.0f))
			{
			}
			// param2
			if (ui::Button("x##param2")) { mVDSettings->iParam2 = 1.0f; }
			ui::SameLine();
			if (ui::SliderFloat("param2/min/max", &mVDSettings->iParam2, 0.01f, 100.0f))
			{
			}
			sprintf_s(buf, "XorY");
			mVDSettings->iXorY ^= ui::Button(buf);
			// blend modes
			if (ui::Button("x##blendmode")) { mVDSettings->iBlendMode = 0.0f; }
			ui::SameLine();
			ui::SliderInt("blendmode", &mVDSettings->iBlendMode, 0, mVDAnimation->maxBlendMode);

			// steps
			ctrl = 20;
			if (ui::Button("x##steps")) { mVDSettings->controlValues[ctrl] = 16.0f; }
			ui::SameLine();
			if (ui::SliderFloat("steps", &mVDSettings->controlValues[ctrl], 1.0f, 128.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}
			// pixelate
			ctrl = 15;
			if (ui::Button("x##pixelate")) { mVDSettings->controlValues[ctrl] = 1.0f; }
			ui::SameLine();
			if (ui::SliderFloat("pixelate", &mVDSettings->controlValues[ctrl], 0.01f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}
			// trixels
			ctrl = 16;
			if (ui::Button("x##trixels")) { mVDSettings->controlValues[ctrl] = 0.0f; }
			ui::SameLine();
			if (ui::SliderFloat("trixels", &mVDSettings->controlValues[ctrl], 0.00f, 1.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}
			// grid
			ctrl = 17;
			if (ui::Button("x##grid")) { mVDSettings->controlValues[ctrl] = 0.0f; }
			ui::SameLine();
			if (ui::SliderFloat("grid", &mVDSettings->controlValues[ctrl], 0.00f, 60.0f))
			{
				aParams << ",{\"name\" : " << ctrl << ",\"value\" : " << mVDSettings->controlValues[ctrl] << "}";
			}

			aParams << "]}";
			string strAParams = aParams.str();
			if (strAParams.length() > 60)
			{
				mVDRouter->sendJSON(strAParams);

			}
		}
		ui::PopItemWidth();
		if (ui::CollapsingHeader("Colors", NULL, true, true))
		{
			stringstream sParams;
			bool colorChanged = false;
			sParams << "{\"params\" :[{\"name\" : 0,\"value\" : " << getElapsedFrames() << "}"; // TimeStamp
			// foreground color
			color[0] = mVDSettings->controlValues[1];
			color[1] = mVDSettings->controlValues[2];
			color[2] = mVDSettings->controlValues[3];
			color[3] = mVDSettings->controlValues[4];
			ui::ColorEdit4("f", color);

			for (int i = 0; i < 4; i++)
			{
				if (mVDSettings->controlValues[i + 1] != color[i])
				{
					sParams << ",{\"name\" : " << i + 1 << ",\"value\" : " << color[i] << "}";
					mVDSettings->controlValues[i + 1] = color[i];
					colorChanged = true;
				}
			}
			if (colorChanged) mVDRouter->colorWrite(); //lights4events

			// background color
			backcolor[0] = mVDSettings->controlValues[5];
			backcolor[1] = mVDSettings->controlValues[6];
			backcolor[2] = mVDSettings->controlValues[7];
			backcolor[3] = mVDSettings->controlValues[8];
			ui::ColorEdit4("g", backcolor);
			for (int i = 0; i < 4; i++)
			{
				if (mVDSettings->controlValues[i + 5] != backcolor[i])
				{
					sParams << ",{\"name\" : " << i + 5 << ",\"value\" : " << backcolor[i] << "}";
					mVDSettings->controlValues[i + 5] = backcolor[i];
				}

			}
			// color multipliers
			if (ui::Button("x##RedX")) { mVDSettings->iRedMultiplier = 1.0f; }
			ui::SameLine();
			if (ui::SliderFloat("RedX", &mVDSettings->iRedMultiplier, 0.0f, 3.0f))
			{
			}
			if (ui::Button("x##GreenX")) { mVDSettings->iGreenMultiplier = 1.0f; }
			ui::SameLine();
			if (ui::SliderFloat("GreenX", &mVDSettings->iGreenMultiplier, 0.0f, 3.0f))
			{
			}
			if (ui::Button("x##BlueX")) { mVDSettings->iBlueMultiplier = 1.0f; }
			ui::SameLine();
			if (ui::SliderFloat("BlueX", &mVDSettings->iBlueMultiplier, 0.0f, 3.0f))
			{
			}

			sParams << "]}";
			string strParams = sParams.str();
			if (strParams.length() > 60)
			{
				mVDRouter->sendJSON(strParams);
			}

		}

		if (ui::CollapsingHeader("Camera", NULL, true, true))
		{
			ui::SliderFloat("Pos.x", &mVDSettings->mRenderPosXY.x, 0.0f, mVDSettings->mRenderWidth);
			ui::SliderFloat("Pos.y", &mVDSettings->mRenderPosXY.y, 0.0f, mVDSettings->mRenderHeight);
			float eyeZ = mVDSettings->mCamera.getEyePoint().z;
			if (ui::SliderFloat("Eye.z", &eyeZ, -500.0f, 1.0f))
			{
				vec3 eye = mVDSettings->mCamera.getEyePoint();
				eye.z = eyeZ;
				mVDSettings->mCamera.setEyePoint(eye);
			}
			ui::SliderFloat("ABP Bend", &mVDSettings->mBend, -20.0f, 20.0f);

		}

	}
	ui::End();

#pragma endregion Global
	xPos = margin;

#pragma region left
	// push color for this chain, must be popped at the end
	ui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 1.0f, 0.0f, 1.00f));

	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPosRow1), ImGuiSetCond_Once);
	ui::Begin("Source");
	{
		ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##left%d", 40);

		ui::Image((void*)mMixes[0]->getLeftFboTexture()->getId(), ivec2(mVDSettings->mPreviewWidth, mVDSettings->mPreviewHeight));

		ui::PopStyleColor(3);
		ui::PopItemWidth();
	}
	ui::End();
	xPos += largePreviewW + margin;

	// pop color for this chain
	ui::PopStyleColor(1);

	// next line


#pragma endregion left

#pragma region mix
	// left/warp1 fbo
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPosRow1), ImGuiSetCond_Once);
	ui::Begin("Left/warp1 fbo");
	{
		ui::PushItemWidth(mVDSettings->mPreviewFboWidth);
		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##mix%d", 40);
		ui::Image((void*)mMixes[0]->getTexture()->getId(), ivec2(mVDSettings->mPreviewWidth, mVDSettings->mPreviewHeight));

		ui::PopStyleColor(3);
		ui::PopItemWidth();
	}
	ui::End();
	xPos += largePreviewW + margin;

	// right/warp2 fbo
	ui::SetNextWindowSize(ImVec2(largePreviewW, largePreviewH), ImGuiSetCond_Once);
	ui::SetNextWindowPos(ImVec2(xPos, yPosRow1), ImGuiSetCond_Once);
	ui::Begin("Right/warp2 fbo");
	{
		ui::PushItemWidth(mVDSettings->mPreviewFboWidth);

		ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.1f, 0.6f, 0.6f));
		ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.1f, 0.7f, 0.7f));
		ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.1f, 0.8f, 0.8f));

		sprintf_s(buf, "FV##f%d", 41);

		ui::Image((void*)mImage->getId(), ivec2(mVDSettings->mPreviewWidth, mVDSettings->mPreviewHeight));

		ui::PopStyleColor(3);
		ui::PopItemWidth();
	}
	ui::End();
	xPos += largePreviewW + margin;

#pragma endregion mix
	xPos = margin;
	switch (currentWindowRow2) {
	case 0:
		// textures
		mVDSettings->mRenderThumbs = false;

#pragma region textures

		for (int i = 0; i < mMixes[0]->getInputTexturesCount(0); i++)
		{
			ui::SetNextWindowSize(ImVec2(w, h*1.4));
			ui::SetNextWindowPos(ImVec2((i * (w + inBetween)) + margin, yPosRow2));
			//ui::Begin(textureNames[i], NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			ui::Begin(mMixes[0]->getInputTextureName(0,i).c_str(), NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			{
				//BEGIN
				/*sprintf_s(buf, "WS##s%d", i);
				if (ui::Button(buf))
				{
				sprintf_s(buf, "IMG=%d.jpg", i);
				//mBatchass->wsWrite(buf);
				}
				if (ui::IsItemHovered()) ui::SetTooltip("Send texture file name via WebSockets");
				ui::SameLine();
				sprintf(buf, "FV##s%d", i);
				if (ui::Button(buf))
				{
				mVDTextures->flipTexture(i);
				}*/
				ui::PushID(i);
				ui::Image((void*)mMixes[0]->getFboInputTexture(0,i)->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
				ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));

				//if (ui::Button("Stop Load")) mVDImageSequences[0]->stopLoading();
				//ui::SameLine();

				/*if (mVDTextures->inputTextureIsSequence(i)) {
					if (!(mVDTextures->inputTextureIsLoadingFromDisk(i))) {
						ui::SameLine();
						sprintf_s(buf, "l##s%d", i);
						if (ui::Button(buf))
						{
							mVDTextures->inputTextureToggleLoadingFromDisk(i);
						}
						if (ui::IsItemHovered()) ui::SetTooltip("Pause loading from disk");
					}
					ui::SameLine();
					sprintf_s(buf, "p##s%d", i);
					if (ui::Button(buf))
					{
						mVDTextures->inputTexturePlayPauseSequence(i);
					}
					if (ui::IsItemHovered()) ui::SetTooltip("Play/Pause");
					ui::SameLine();
					sprintf_s(buf, "b##s%d", i);
					if (ui::Button(buf))
					{
						mVDTextures->inputTextureSyncToBeatSequence(i);
					}
					if (ui::IsItemHovered()) ui::SetTooltip("Sync to beat");
					ui::SameLine();
					sprintf_s(buf, "r##s%d", i);
					if (ui::Button(buf))
					{
						mVDTextures->inputTextureReverseSequence(i);
					}
					if (ui::IsItemHovered()) ui::SetTooltip("Reverse");
					playheadPositions[i] = mVDTextures->inputTextureGetPlayheadPosition(i);
					sprintf_s(buf, "p%d##s%d", playheadPositions[i], i);
					if (ui::Button(buf))
					{
					mVDTextures->inputTextureSetPlayheadPosition(i, playheadPositions[i]);
					}

					if (ui::SliderInt("scrub", &playheadPositions[i], 0, mVDTextures->inputTextureGetMaxFrame(i)))
					{
						mVDTextures->inputTextureSetPlayheadPosition(i, playheadPositions[i]);
					}
					speeds[i] = mVDTextures->inputTextureGetSpeed(i);
					if (ui::SliderInt("speed", &speeds[i], 0.0f, 6.0f))
					{
						mVDTextures->inputTextureSetSpeed(i, speeds[i]);
					}

				}*/

				//END
				ui::PopStyleColor(3);
				ui::PopID();
			}
			ui::End();
		}


#pragma endregion textures
		break;
	case 1:
		// Fbos

#pragma region fbos
		mVDSettings->mRenderThumbs = true;

		for (int i = 0; i < mMixes[0]->getFboCount(); i++)
		{
			ui::SetNextWindowSize(ImVec2(w, h));
			ui::SetNextWindowPos(ImVec2((i * (w + inBetween)) + margin, yPosRow2));
			ui::Begin(mMixes[0]->getFboName(i).c_str(), NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
			{

				//ui::PushID(i);
				ui::Image((void*)mMixes[0]->getFboTexture(i)->getId(), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
				/*ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));

				sprintf_s(buf, "FV##fbo%d", i);
				if (ui::Button(buf)) mVDTextures->flipFboV(i);
				if (ui::IsItemHovered()) ui::SetTooltip("Flip vertically");

				ui::PopStyleColor(3);
				ui::PopID();*/
			}
			ui::End();
		}


#pragma endregion fbos
		break;
	case 2:
		// Shaders

#pragma region library
		/*mVDSettings->mRenderThumbs = true;

		static ImGuiTextFilter filter;
		ui::SetNextWindowSize(ImVec2(w, h));
		ui::SetNextWindowPos(ImVec2(800, 240));
		ui::Begin("Filter", NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
		{
		ui::Text("Filter usage:\n"
		"  \"\"         display all lines\n"
		"  \"xxx\"      display lines containing \"xxx\"\n"
		"  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
		"  \"-xxx\"     hide lines containing \"xxx\"");
		filter.Draw();


		for (int i = 0; i < mVDShaders->getCount(); i++)
		{
		if (filter.PassFilter(mVDShaders->getShader(i).name.c_str()))
		ui::BulletText("%s", mVDShaders->getShader(i).name.c_str());
		}
		}
		ui::End();
		xPos = margin;
		yPos = yPosRow2;
		for (int i = 0; i < mVDShaders->getCount(); i++)
		{
			//if (filter.PassFilter(mVDShaders->getShader(i).name.c_str()) && mVDShaders->getShader(i).active)
			if (mVDShaders->getShader(i).active)
			{
				if (mVDSettings->iTrack == i) {
					sprintf_s(buf, "SEL ##lsh%d", i);
				}
				else {
					sprintf_s(buf, "%d##lsh%d", mVDShaders->getShader(i).microseconds, i);
				}

				ui::SetNextWindowSize(ImVec2(w, h));
				ui::SetNextWindowPos(ImVec2(xPos + margin, yPos));
				ui::Begin(buf, NULL, ImVec2(0, 0), ui::GetStyle().Alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
				{
					xPos += w + inBetween;
					if (xPos > mVDSettings->MAX * w * 1.0)
					{
						xPos = margin;
						yPos += h + margin;
					}
					ui::PushID(i);
					ui::Image((void*)mVDTextures->getFboTextureId(i), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
					if (ui::IsItemHovered()) ui::SetTooltip(mVDShaders->getShader(i).name.c_str());

					//ui::Columns(2, "lr", false);
					// left
					if (mVDSettings->mLeftFragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));

					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.0f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.0f, 0.8f, 0.8f));
					sprintf_s(buf, "L##s%d", i);
					if (ui::Button(buf)) mVDSettings->mLeftFragIndex = i;// TODO send via OSC? selectShader(true, i);
					if (ui::IsItemHovered()) ui::SetTooltip("Set shader to left");
					ui::PopStyleColor(3);
					//ui::NextColumn();
					ui::SameLine();
					// right
					if (mVDSettings->mRightFragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.3f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.3f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.3f, 0.8f, 0.8f));
					sprintf_s(buf, "R##s%d", i);
					if (ui::Button(buf)) mVDSettings->mRightFragIndex = i;// TODO send via OSC? selectShader(false, i);
					if (ui::IsItemHovered()) ui::SetTooltip("Set shader to right");
					ui::PopStyleColor(3);
					//ui::NextColumn();
					ui::SameLine();
					// preview
					if (mVDSettings->mPreviewFragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.6f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.6f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.6f, 0.8f, 0.8f));
					sprintf_s(buf, "P##s%d", i);
					if (ui::Button(buf)) mVDSettings->mPreviewFragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Preview shader");
					ui::PopStyleColor(3);

					// warp1
					if (mVDSettings->mWarp1FragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.16f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.16f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.16f, 0.8f, 0.8f));
					sprintf_s(buf, "1##s%d", i);
					if (ui::Button(buf)) mVDSettings->mWarp1FragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set warp 1 shader");
					ui::PopStyleColor(3);
					ui::SameLine();

					// warp2
					if (mVDSettings->mWarp2FragIndex == i)
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.77f, 1.0f, 0.5f));
					}
					else
					{
						ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.0f, 0.1f, 0.1f));
					}
					ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.77f, 0.7f, 0.7f));
					ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(0.77f, 0.8f, 0.8f));
					sprintf_s(buf, "2##s%d", i);
					if (ui::Button(buf)) mVDSettings->mWarp2FragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set warp 2 shader");
					ui::PopStyleColor(3);

					// enable removing shaders
					if (i > 4)
					{
						ui::SameLine();
						sprintf_s(buf, "X##s%d", i);
						if (ui::Button(buf)) mVDShaders->removePixelFragmentShaderAtIndex(i);
						if (ui::IsItemHovered()) ui::SetTooltip("Remove shader");
					}

					ui::PopID();

				}
				ui::End();
			} // if filtered

		} // for
		*/
#pragma endregion library
		break;
	}
	xPos = margin;
	switch (currentWindowRow3) {
	case 0:
		// Osc
		mVDSettings->mRenderThumbs = false;
		break;
	case 1:
		// Midi
		mVDSettings->mRenderThumbs = false;

#pragma region MIDI

		ui::SetNextWindowSize(ImVec2(largePreviewW + 20, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPosRow3), ImGuiSetCond_Once);
		ui::Begin("MIDI");
		{
			sprintf_s(buf, "Enable");
			if (ui::Button(buf)) mVDRouter->midiSetup();
			if (ui::CollapsingHeader("MidiIn", "20", true, true))
			{
				ui::Columns(2, "data", true);
				ui::Text("Name"); ui::NextColumn();
				ui::Text("Connect"); ui::NextColumn();
				ui::Separator();

				for (int i = 0; i < mVDRouter->getMidiInPortsCount(); i++)
				{
					ui::Text(mVDRouter->getMidiInPortName(i).c_str()); ui::NextColumn();

					if (mVDRouter->isMidiInConnected(i))
					{
						sprintf_s(buf, "Disconnect %d", i);
					}
					else
					{
						sprintf_s(buf, "Connect %d", i);
					}

					if (ui::Button(buf))
					{
						if (mVDRouter->isMidiInConnected(i))
						{
							mVDRouter->closeMidiInPort(i);
						}
						else
						{
							mVDRouter->openMidiInPort(i);
						}
					}
					ui::NextColumn();
					ui::Separator();
				}
				ui::Columns(1);
			}
		}
		ui::End();



#pragma endregion MIDI
		break;
	case 2:
		// Channels
		mVDSettings->mRenderThumbs = false;
#pragma region channels

		ui::SetNextWindowSize(ImVec2(w * 2, largePreviewH), ImGuiSetCond_Once);
		ui::SetNextWindowPos(ImVec2(xPos, yPosRow3), ImGuiSetCond_Once);

		ui::Begin("Channels");
		{
			ui::Columns(3);
			ui::SetColumnOffset(0, 4.0f);// int column_index, float offset)
			ui::SetColumnOffset(1, 20.0f);// int column_index, float offset)
			//ui::SetColumnOffset(2, 24.0f);// int column_index, float offset)
			ui::Text("Chn"); ui::NextColumn();
			ui::Text("Tex"); ui::NextColumn();
			ui::Text("Name"); ui::NextColumn();
			ui::Separator();
			for (int i = 0; i < mVDSettings->MAX - 1; i++)
			{
				ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
				ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
				ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
				ui::Text("c%d", i);
				ui::NextColumn();
				sprintf_s(buf, "%d", i);
				if (ui::SliderInt(buf, &mVDSettings->iChannels[i], 0, mVDSettings->MAX - 1)) {
				}
				ui::NextColumn();
				ui::PopStyleColor(3);
				ui::Text("%s", mMixes[0]->getInputTextureName(0, mVDSettings->iChannels[i]));
				ui::NextColumn();
			}
			ui::Columns(1);
		}
		ui::End();


#pragma endregion channels
		break;
	case 3:
		// Blendmodes

		break;

	}

	// next line
	xPos = margin;

	/*
	#pragma region warps
	if (mVDSettings->mMode == MODE_WARP)
	{
	for (int i = 0; i < mBatchass->getWarpsRef()->getWarpsCount(); i++)
	{
	sprintf_s(buf, "Warp %d", i);
	ui::SetNextWindowSize(ImVec2(w, h));
	ui::Begin(buf);
	{
	ui::SetWindowPos(ImVec2((i * (w + inBetween)) + margin, yPos));
	ui::PushID(i);
	ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mVDSettings->mWarpFbos[i].textureIndex), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
	ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
	ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
	ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
	sprintf_s(buf, "%d", mVDSettings->mWarpFbos[i].textureIndex);
	if (ui::SliderInt(buf, &mVDSettings->mWarpFbos[i].textureIndex, 0, mVDSettings->MAX - 1)) {
	}
	sprintf_s(buf, "%s", warpInputs[mVDSettings->mWarpFbos[i].textureIndex]);
	ui::Text(buf);

	ui::PopStyleColor(3);
	ui::PopID();
	}
	ui::End();
	}
	yPos += h + margin;
	}
	#pragma endregion warps
	*/

	gl::draw(mUIFbo->getColorTexture());



}
void VideodrommVisualizerApp::draw()
{

	if (mIsResizing) {
		mIsResizing = false;

		// set ui window and io events callbacks 
		ui::connectWindow(getWindow());

		ui::initialize();

#pragma region style
		// our theme variables
		ImGuiStyle& style = ui::GetStyle();
		style.WindowRounding = 4;
		style.WindowPadding = ImVec2(3, 3);
		style.FramePadding = ImVec2(2, 2);
		style.ItemSpacing = ImVec2(3, 3);
		style.ItemInnerSpacing = ImVec2(3, 3);
		style.WindowMinSize = ImVec2(w, mVDSettings->mPreviewFboHeight);
		style.Alpha = 0.6f;
		style.Colors[ImGuiCol_Text] = ImVec4(0.89f, 0.92f, 0.94f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.38f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.4f, 0.0f, 0.21f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.97f, 0.0f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
		style.Colors[ImGuiCol_ComboBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.99f, 0.22f, 0.22f, 0.50f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_Column] = ImVec4(0.04f, 0.04f, 0.04f, 0.22f);
		style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.9f, 0.45f, 0.45f, 1.00f);
		style.Colors[ImGuiCol_CloseButton] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.8f, 0.35f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		style.Colors[ImGuiCol_TooltipBg] = ImVec4(0.65f, 0.25f, 0.25f, 1.00f);
#pragma endregion style

	}

	renderSceneToFbo();
	gl::clear(Color::black());


	//gl::clear(Color::black());
	gl::draw(mRenderFbo->getColorTexture());
	if (mFadeInDelay) {
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			int uiWidth = (int)mVDSettings->mMainWindowWidth / 2;
			setWindowSize(mVDSettings->mRenderWidth + uiWidth, mVDSettings->mRenderHeight + 30);
			setWindowPos(ivec2(mVDSettings->mRenderX - uiWidth, 0));
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	//imgui
	if (removeUI || Warp::isEditModeEnabled())
	{
		return;
	}
	renderUIToFbo();
	gl::draw(mUIFbo->getColorTexture()); // TODO CHECK, Rectf(128, 0, 256, 128)
}

void VideodrommVisualizerApp::updateWindowTitle()
{
	getWindow()->setTitle("(" + mVDSettings->sFps + " fps) " + toString(mVDSettings->iBeat) + " Videodromm");

}
// If you're deploying to iOS, set the Render antialiasing to 0 for a significant
// performance improvement. This value defaults to 4 (AA_MSAA_4) on iOS and 16 (AA_MSAA_16)
// on the Desktop.
#if defined( CINDER_COCOA_TOUCH )
CINDER_APP(VideodrommVisualizerApp, RendererGl(RendererGl::AA_NONE))
#else
CINDER_APP(VideodrommVisualizerApp, RendererGl(RendererGl::Options().msaa(8)), &VideodrommVisualizerApp::prepare)
#endif
