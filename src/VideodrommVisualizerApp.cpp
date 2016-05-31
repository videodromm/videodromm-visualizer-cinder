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
	// maximize fps
	disableFrameRate();
	gl::enableVerticalSync(false);
	// Log
	mVDLog = VDLog::create();
	CI_LOG_V("Controller");
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
	mMixesFilepath = getAssetPath("") / "mixes.xml";
	if (fs::exists(mMixesFilepath)) {
		// load textures from file if one exists
		mMixes = VDMix::readSettings(mVDSettings, mVDAnimation, loadFile(mMixesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mMixes.push_back(VDMix::create(mVDSettings, mVDAnimation));
	}
	mMixes[0]->setLeftFboIndex(2);
	mMixes[0]->setRightFboIndex(1);
	mVDAnimation->tapTempo();

	// UI
	mVDUI = VDUI::create(mVDSettings, mMixes[0], mVDRouter, mVDAnimation, mVDSession);

	setFrameRate(mVDSession->getTargetFps());
	mFadeInDelay = true;
	mIsResizing = true;
	mVDUtils->getWindowsResolution();
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
	setWindowPos(ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY));

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
	//Warp::setSize(mWarps, ivec2(mVDSettings->mFboWidth, mVDSettings->mFboHeight));
	Warp::setSize(mWarps, ivec2(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight));
	Warp::handleResize(mWarps);
	mSaveThumbTimer = 0.0f;

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
	removeUI = true;
	static float f = 0.0f;
	// mouse cursor
	if (mVDSettings->mCursorVisible) {
		hideCursor();
	} else {
		showCursor();
	}

}

void VideodrommVisualizerApp::cleanup() {
	CI_LOG_V("shutdown");
	// save warp settings
	Warp::writeSettings(mWarps, writeFile(mWarpSettings));
	mVDSettings->save();
	mVDSession->save();
	ui::Shutdown();
	quit();
}

void VideodrommVisualizerApp::resizeWindow() {
	mIsResizing = true;
	// disconnect ui window and io events callbacks
	ui::disconnectWindow(getWindow());

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
void VideodrommVisualizerApp::update()
{
	mVDSettings->iFps = getAverageFps();
	mVDSettings->sFps = toString(floor(mVDSettings->iFps));
	mVDAnimation->update();
	mVDRouter->update();

	// check if a shader has been received from websockets
	if (mVDSettings->mShaderToLoad != "") {
		mMixes[0]->loadFboFragmentShader(mVDSettings->mShaderToLoad, 1);
	}

	updateWindowTitle();
}
void VideodrommVisualizerApp::fileDrop(FileDropEvent event)
{
	int index = (int)(event.getX() / (margin + w));
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
		mMixes[0]->loadImageFile(mFile, index, true);
	}
	else if (ext == "glsl")
	{
		if (index > mMixes[0]->getFboCount() - 1) index = mMixes[0]->getFboCount() - 1;
		int rtn = mMixes[0]->loadFboFragmentShader(mFile, index);		
		if (rtn > -1 )
		{
			// reset zoom
			mVDAnimation->controlValues[22] = 1.0f;
			// save thumb
			timeline().apply(&mSaveThumbTimer, 1.0f, 1.0f).finishFn([&]{ saveThumb(); });
		}
	}
	else if (ext == "xml")
	{
	}
	else if (ext == "mov")
	{
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
	// iterate over the warps and draw their content
	for (auto &warp : mWarps) {
		warp->draw(mMixes[0]->getFboTexture(mWarpFboIndex), mMixes[0]->getFboTexture(mWarpFboIndex)->getBounds());
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
	//gl::setMatricesWindow(500, 400);



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
					sprintf(buf, "SEL ##lsh%d", i);
				}
				else {
					sprintf(buf, "%d##lsh%d", mVDShaders->getShader(i).microseconds, i);
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
					sprintf(buf, "L##s%d", i);
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
					sprintf(buf, "R##s%d", i);
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
					sprintf(buf, "P##s%d", i);
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
					sprintf(buf, "1##s%d", i);
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
					sprintf(buf, "2##s%d", i);
					if (ui::Button(buf)) mVDSettings->mWarp2FragIndex = i;
					if (ui::IsItemHovered()) ui::SetTooltip("Set warp 2 shader");
					ui::PopStyleColor(3);

					// enable removing shaders
					if (i > 4)
					{
						ui::SameLine();
						sprintf(buf, "X##s%d", i);
						if (ui::Button(buf)) mVDShaders->removePixelFragmentShaderAtIndex(i);
						if (ui::IsItemHovered()) ui::SetTooltip("Remove shader");
					}

					ui::PopID();

				}
				ui::End();
			} // if filtered

		} // for
		*/
	xPos = margin;
	
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
				sprintf(buf, "%d", i);
				if (ui::SliderInt(buf, &mVDSettings->iChannels[i], 0, mVDSettings->MAX - 1)) {
				}
				ui::NextColumn();
				ui::PopStyleColor(3);
				//ui::Text("%s", mMixes[0]->getInputTextureName(0, mVDSettings->iChannels[i]).c_str());
				ui::NextColumn();
			}
			ui::Columns(1);
		}
		ui::End();


#pragma endregion channels


	showVDUI((int)getAverageFps());
	/*
	#pragma region warps
	if (mVDSettings->mMode == MODE_WARP)
	{
	for (int i = 0; i < mBatchass->getWarpsRef()->getWarpsCount(); i++)
	{
	sprintf(buf, "Warp %d", i);
	ui::SetNextWindowSize(ImVec2(w, h));
	ui::Begin(buf);
	{
	ui::SetWindowPos(ImVec2((i * (w + inBetween)) + margin, yPos));
	ui::PushID(i);
	ui::Image((void*)mBatchass->getTexturesRef()->getFboTextureId(mVDSettings->mWarpFbos[i].textureIndex), ivec2(mVDSettings->mPreviewFboWidth, mVDSettings->mPreviewFboHeight));
	ui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
	ui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
	ui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
	sprintf(buf, "%d", mVDSettings->mWarpFbos[i].textureIndex);
	if (ui::SliderInt(buf, &mVDSettings->mWarpFbos[i].textureIndex, 0, mVDSettings->MAX - 1)) {
	}
	sprintf(buf, "%s", warpInputs[mVDSettings->mWarpFbos[i].textureIndex]);
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
	gl::setMatricesWindow(mVDSettings->mMainWindowWidth, mVDSettings->mMainWindowHeight, false);

	gl::draw(mRenderFbo->getColorTexture());
	/* TODO check for single screen
	if (mFadeInDelay) {
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			int uiWidth = (int)mVDSettings->mMainWindowWidth / 2;
			setWindowSize(mVDSettings->mRenderWidth + uiWidth, mVDSettings->mRenderHeight + 30);
			setWindowPos(ivec2(mVDSettings->mRenderX - uiWidth, 0));
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}*/
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
