#include "VideodrommVisualizerApp.h"
/*
TODO
 0 video playback spout hap codec gst?
 1 post processing shader separated from mix shader
 2 video player spout test
 3 imgseq spout test
 4 midimix
 5 do like IFPS for all uniforms
 6 spout in don't show
 7 warp size fbo size lower only livecoding prj
 add mixall.frag (mix 8 textures from fbo or not, with weights)
 texture0 = audio in
 create mVDSession->getShader(0 to 8) for midimix
 shader 0 = spout receiver (videoplayer: add spout sender)
 shader 1 = texture 1 (no fbo) jpg or imgseq
 shader 2 = texture 2 (no fbo) jpg or imgseq
 shader 3 = texture 3 (no fbo) movie gstreamer
 shader 4 to 7 = user shader
 midi: get currentvalue and start from there
 integrate gstreamer
 displacement sphere
 beautiful chaos
*/
void VideodrommVisualizerApp::setup()
{
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);
	mVDSettings->mStandalone = true;
	mVDSettings->mCursorVisible = false;
	setUIVisibility(mVDSettings->mCursorVisible);

	// make window size the maximum
	mVDSession->getWindowsResolution();
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);//20141214 was 0
	setWindowPos(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);

	// Animation
	mVDAnimation = VDAnimation::create(mVDSettings);
	// initialize
	mTexturesFilepath = getAssetPath("") / "defaulttextures.xml";
	if (fs::exists(mTexturesFilepath)) {
		// load textures from file if one exists
		mTexs = VDTexture::readSettings(mVDAnimation, loadFile(mTexturesFilepath));
	}
	else {
		// otherwise create a texture from scratch
		mTexs.push_back(TextureAudio::create(mVDAnimation));
	}
	gl::Texture::Format fmt;
	fmt.setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
	fmt.setBorderColor(Color::black());

	gl::Fbo::Format fboFmt;
	fboFmt.setColorTextureFormat(fmt);
	mFboA = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
	mFboB = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
	mFboMix = gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt);
	for (size_t i = 0; i < 27; i++)
	{
		mFboBlend.push_back(gl::Fbo::create(mVDSettings->mFboWidth, mVDSettings->mFboHeight, fboFmt));

	}
	mCurrentBlend = 0;
	mGlslA = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadAsset("simple330a.frag"));
	mGlslB = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadAsset("simple330b.frag"));
	mGlslMix = gl::GlslProg::create(loadAsset("passthru.vert"), loadAsset("mix.frag"));
	mGlslBlend = gl::GlslProg::create(loadAsset("passthru.vert"), loadAsset("mix.frag"));
	gl::enableDepthRead();
	gl::enableDepthWrite();
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
void VideodrommVisualizerApp::fileDrop(FileDropEvent event)
{
	int index = 1;
	string ext = "";
	// use the last of the dropped files
	ci::fs::path mPath = event.getFile(event.getNumFiles() - 1);
	string mFile = mPath.string();
	int dotIndex = mFile.find_last_of(".");
	int slashIndex = mFile.find_last_of("\\");
	bool found = false;

	if (dotIndex != std::string::npos && dotIndex > slashIndex) ext = mFile.substr(mFile.find_last_of(".") + 1);

	if (ext == "wav" || ext == "mp3")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::AUDIO) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "png" || ext == "jpg")
	{
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::IMAGE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}
	else if (ext == "frag")
	{
		if (event.getX() < getWindowWidth() / 2) {
			mGlslA = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadFile(mFile));
		}
		else {
			mGlslB = gl::GlslProg::create(loadAsset("shaders/passthrough330.vert"), loadFile(mFile));

		}
	}
	else if (ext == "")
	{
		// try loading image sequence from dir
		for (auto tex : mTexs)
		{
			if (!found) {
				if (tex->getType() == VDTexture::SEQUENCE) {
					tex->loadFromFullPath(mFile);
					found = true;
				}
			}
		}
	}

}
void VideodrommVisualizerApp::resize()
{

}

void VideodrommVisualizerApp::cleanup()
{
	// save textures
	VDTexture::writeSettings(mTexs, writeFile(mTexturesFilepath));

	quit();
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
			// mouse cursor
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
void VideodrommVisualizerApp::update()
{
	mVDSession->setFloatUniformValueByIndex(mVDSettings->IFPS, getAverageFps());
	mVDSession->update();
	mGlslA->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlslA->uniform("iChannel0", 0);
	mGlslB->uniform("iGlobalTime", (float)(getElapsedSeconds()/2));
	mGlslB->uniform("iChannel0", 0);

	//mVDSettings->iBlendMode = getElapsedFrames()/48 % 27;
	mGlslMix->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlslMix->uniform("iResolution", vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0));
	mGlslMix->uniform("iMouse", mVDAnimation->getVec4UniformValueByName("iMouse"));
	mGlslMix->uniform("iChannel0", 0);
	mGlslMix->uniform("iChannel1", 1);
	mGlslMix->uniform("iAudio0", 0);
	mGlslMix->uniform("iFreq0", mVDAnimation->iFreqs[0]);
	mGlslMix->uniform("iFreq1", mVDAnimation->iFreqs[1]);
	mGlslMix->uniform("iFreq2", mVDAnimation->iFreqs[2]);
	mGlslMix->uniform("iFreq3", mVDAnimation->iFreqs[3]);
	mGlslMix->uniform("iChannelTime", mVDSettings->iChannelTime, 4);
	mGlslMix->uniform("iColor", vec3(mVDAnimation->getFloatUniformValueByIndex(1), mVDAnimation->getFloatUniformValueByIndex(2), mVDAnimation->getFloatUniformValueByIndex(3)));
	mGlslMix->uniform("iBackgroundColor", mVDAnimation->getVec3UniformValueByName("iBackgroundColor"));
	mGlslMix->uniform("iSteps", (int)mVDAnimation->getFloatUniformValueByIndex(10));
	mGlslMix->uniform("iRatio", mVDAnimation->getFloatUniformValueByIndex(11));
	mGlslMix->uniform("width", 1);
	mGlslMix->uniform("height", 1);
	mGlslMix->uniform("iRenderXY", mVDSettings->mRenderXY);
	mGlslMix->uniform("iZoom", mVDAnimation->getFloatUniformValueByIndex(12));
	mGlslMix->uniform("iAlpha", mVDAnimation->getFloatUniformValueByIndex(4) * mVDSettings->iAlpha);
	mGlslMix->uniform("iBlendmode", mCurrentBlend); // should be mVDSettings->iBlendMode);
	mGlslMix->uniform("iChromatic", mVDAnimation->getFloatUniformValueByIndex(17));
	mGlslMix->uniform("iRotationSpeed", mVDAnimation->getFloatUniformValueByIndex(9));
	mGlslMix->uniform("iCrossfade", 0.5f);// mVDAnimation->controlValues[18]);
	mGlslMix->uniform("iPixelate", mVDAnimation->getFloatUniformValueByIndex(15));
	mGlslMix->uniform("iExposure", mVDAnimation->getFloatUniformValueByIndex(14));
	mGlslMix->uniform("iDeltaTime", mVDAnimation->iDeltaTime);
	mGlslMix->uniform("iFade", (int)mVDSettings->iFade);
	mGlslMix->uniform("iToggle", (int)mVDAnimation->getBoolUniformValueByIndex(46));
	mGlslMix->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
	mGlslMix->uniform("iTransition", mVDSettings->iTransition);
	mGlslMix->uniform("iAnim", mVDSettings->iAnim.value());
	mGlslMix->uniform("iRepeat", (int)mVDSettings->iRepeat);
	mGlslMix->uniform("iVignette", (int)mVDAnimation->getBoolUniformValueByIndex(47));
	mGlslMix->uniform("iInvert", (int)mVDAnimation->getBoolUniformValueByIndex(48));
	mGlslMix->uniform("iDebug", (int)mVDSettings->iDebug);
	mGlslMix->uniform("iShowFps", (int)mVDSettings->iShowFps);
	mGlslMix->uniform("iFps", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IFPS));
	mGlslMix->uniform("iTempoTime", mVDAnimation->getFloatUniformValueByName("iTempoTime"));
	mGlslMix->uniform("iGlitch", (int)mVDAnimation->getBoolUniformValueByIndex(45));
	mGlslMix->uniform("iTrixels", mVDAnimation->getFloatUniformValueByIndex(16));
	mGlslMix->uniform("iBeat", mVDSettings->iBeat);
	mGlslMix->uniform("iSeed", mVDSettings->iSeed);
	mGlslMix->uniform("iRedMultiplier", mVDAnimation->getFloatUniformValueByName("iRedMultiplier"));
	mGlslMix->uniform("iGreenMultiplier", mVDAnimation->getFloatUniformValueByName("iGreenMultiplier"));
	mGlslMix->uniform("iBlueMultiplier", mVDAnimation->getFloatUniformValueByName("iBlueMultiplier"));
	mGlslMix->uniform("iFlipH", 0);
	mGlslMix->uniform("iFlipV", 0);
	mGlslMix->uniform("iParam1", mVDSettings->iParam1);
	mGlslMix->uniform("iParam2", mVDSettings->iParam2);
	mGlslMix->uniform("iXorY", mVDSettings->iXorY);
	mGlslMix->uniform("iBadTv", mVDAnimation->getFloatUniformValueByName("iBadTv"));

	mGlslBlend->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlslBlend->uniform("iResolution", vec3(mVDSettings->mFboWidth, mVDSettings->mFboHeight, 1.0));
	mGlslBlend->uniform("iMouse", mVDAnimation->getVec4UniformValueByName("iMouse"));
	mGlslBlend->uniform("iChannel0", 0);
	mGlslBlend->uniform("iChannel1", 1);
	mGlslBlend->uniform("iAudio0", 0);
	mGlslBlend->uniform("iFreq0", mVDAnimation->iFreqs[0]);
	mGlslBlend->uniform("iFreq1", mVDAnimation->iFreqs[1]);
	mGlslBlend->uniform("iFreq2", mVDAnimation->iFreqs[2]);
	mGlslBlend->uniform("iFreq3", mVDAnimation->iFreqs[3]);
	mGlslBlend->uniform("iChannelTime", mVDSettings->iChannelTime, 4);
	mGlslBlend->uniform("iColor", vec3(mVDAnimation->getFloatUniformValueByIndex(1), mVDAnimation->getFloatUniformValueByIndex(2), mVDAnimation->getFloatUniformValueByIndex(3)));
	mGlslBlend->uniform("iBackgroundColor", mVDAnimation->getVec3UniformValueByName("iBackgroundColor"));
	mGlslBlend->uniform("iSteps", (int)mVDAnimation->getFloatUniformValueByIndex(10));
	mGlslBlend->uniform("iRatio", mVDAnimation->getFloatUniformValueByIndex(11));
	mGlslBlend->uniform("width", 1);
	mGlslBlend->uniform("height", 1);
	mGlslBlend->uniform("iRenderXY", mVDSettings->mRenderXY);
	mGlslBlend->uniform("iZoom", mVDAnimation->getFloatUniformValueByIndex(12));
	mGlslBlend->uniform("iAlpha", mVDAnimation->getFloatUniformValueByIndex(4) * mVDSettings->iAlpha);
	mGlslBlend->uniform("iBlendmode", mCurrentBlend);
	mGlslBlend->uniform("iChromatic", mVDAnimation->getFloatUniformValueByIndex(17));
	mGlslBlend->uniform("iRotationSpeed", mVDAnimation->getFloatUniformValueByIndex(9));
	mGlslBlend->uniform("iCrossfade", 0.5f);// mVDAnimation->controlValues[18]);
	mGlslBlend->uniform("iPixelate", mVDAnimation->getFloatUniformValueByIndex(15));
	mGlslBlend->uniform("iExposure", mVDAnimation->getFloatUniformValueByIndex(14));
	mGlslBlend->uniform("iDeltaTime", mVDAnimation->iDeltaTime);
	mGlslBlend->uniform("iFade", (int)mVDSettings->iFade);
	mGlslBlend->uniform("iToggle", (int)mVDAnimation->getBoolUniformValueByIndex(46));
	mGlslBlend->uniform("iGreyScale", (int)mVDSettings->iGreyScale);
	mGlslBlend->uniform("iTransition", mVDSettings->iTransition);
	mGlslBlend->uniform("iAnim", mVDSettings->iAnim.value());
	mGlslBlend->uniform("iRepeat", (int)mVDSettings->iRepeat);
	mGlslBlend->uniform("iVignette", (int)mVDAnimation->getBoolUniformValueByIndex(47));
	mGlslBlend->uniform("iInvert", (int)mVDAnimation->getBoolUniformValueByIndex(48));
	mGlslBlend->uniform("iDebug", (int)mVDSettings->iDebug);
	mGlslBlend->uniform("iShowFps", (int)mVDSettings->iShowFps);
	mGlslBlend->uniform("iFps", mVDAnimation->getFloatUniformValueByIndex(mVDSettings->IFPS));
	mGlslBlend->uniform("iTempoTime", mVDAnimation->getFloatUniformValueByName("iTempoTime"));
	mGlslBlend->uniform("iGlitch", (int)mVDAnimation->getBoolUniformValueByIndex(45));
	mGlslBlend->uniform("iTrixels", mVDAnimation->getFloatUniformValueByIndex(16));
	mGlslBlend->uniform("iBeat", mVDSettings->iBeat);
	mGlslBlend->uniform("iSeed", mVDSettings->iSeed);
	mGlslBlend->uniform("iRedMultiplier", mVDAnimation->getFloatUniformValueByName("iRedMultiplier"));
	mGlslBlend->uniform("iGreenMultiplier", mVDAnimation->getFloatUniformValueByName("iGreenMultiplier"));
	mGlslBlend->uniform("iBlueMultiplier", mVDAnimation->getFloatUniformValueByName("iBlueMultiplier"));
	mGlslBlend->uniform("iFlipH", 0);
	mGlslBlend->uniform("iFlipV", 0);
	mGlslBlend->uniform("iParam1", mVDSettings->iParam1);
	mGlslBlend->uniform("iParam2", mVDSettings->iParam2);
	mGlslBlend->uniform("iXorY", mVDSettings->iXorY);
	mGlslBlend->uniform("iBadTv", mVDAnimation->getFloatUniformValueByName("iBadTv"));
	renderSceneA();
	renderSceneB();
	renderMix();
	renderBlend();
}
void VideodrommVisualizerApp::renderSceneA()
{
	gl::ScopedFramebuffer scopedFbo(mFboA);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslA);
	mTexs[1]->getTexture()->bind(0);

	gl::drawSolidRect(Rectf(0, 0, mFboA->getWidth(), mFboA->getHeight()));
}
void VideodrommVisualizerApp::renderSceneB()
{
	gl::ScopedFramebuffer scopedFbo(mFboB);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslB);
	mTexs[2]->getTexture()->bind(0);

	gl::drawSolidRect(Rectf(0, 0, mFboB->getWidth(), mFboB->getHeight()));
}
void VideodrommVisualizerApp::renderMix()
{
	gl::ScopedFramebuffer scopedFbo(mFboMix);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslMix);
	mFboA->getColorTexture()->bind(0);
	mFboB->getColorTexture()->bind(1);
	gl::drawSolidRect(Rectf(0, 0, mFboMix->getWidth(), mFboMix->getHeight()));
}
void VideodrommVisualizerApp::renderBlend()
{
	mCurrentBlend++;
	if (mCurrentBlend>mFboBlend.size() - 1) mCurrentBlend = 0;
	gl::ScopedFramebuffer scopedFbo(mFboBlend[mCurrentBlend]);
	gl::clear(Color::black());

	gl::ScopedGlslProg glslScope(mGlslBlend);
	mFboA->getColorTexture()->bind(0);
	mFboB->getColorTexture()->bind(1);
	gl::drawSolidRect(Rectf(0, 0, mFboBlend[mCurrentBlend]->getWidth(), mFboBlend[mCurrentBlend]->getHeight()));
}
void VideodrommVisualizerApp::draw()
{
	gl::clear(Color::black());
	i = 0;
	for (auto tex : mTexs)
	{
		int x = 128 * i;
		gl::draw(tex->getTexture(), Rectf(0 + x, 0, 128 + x, 128));
		i++;
	}
	for (size_t b = 0; b < mFboBlend.size() - 1; b++)
	{
		int x = 64 * b;
		gl::draw(mFboBlend[b]->getColorTexture(), Rectf(0 + x, 256, 64 + x, 320));
	}
	gl::draw(mFboA->getColorTexture(), Rectf(0, 128, 128, 256));
	gl::draw(mFboMix->getColorTexture(), Rectf(128, 128, 256, 256));
	gl::draw(mFboB->getColorTexture(), Rectf(384, 128, 512, 256));
	getWindow()->setTitle(mVDSettings->sFps + " fps Videodromm visualizer");

}

void VideodrommVisualizerApp::prepare(Settings *settings)
{
	//settings->setWindowSize(40, 10);
	settings->setBorderless();
#ifdef _DEBUG
	settings->setConsoleWindowEnabled();
#else
#endif  // _DEBUG
}
CINDER_APP(VideodrommVisualizerApp, RendererGl, &VideodrommVisualizerApp::prepare)
