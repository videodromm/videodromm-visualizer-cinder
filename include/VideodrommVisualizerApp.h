#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Animation
#include "VDAnimation.h"

#include "VDTexture.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

class VideodrommVisualizerApp : public App {

public:
	static void					prepare(Settings *settings);
	void						setup() override;
	void						mouseDown(MouseEvent event) override;
	void						mouseDrag(MouseEvent event) override;
	void						mouseMove(MouseEvent event) override;
	void						mouseUp(MouseEvent event) override;
	void						keyDown(KeyEvent event) override;
	void						keyUp(KeyEvent event) override;
	void						update() override;
	void						draw() override;
	void						fileDrop(FileDropEvent event) override;
	void						cleanup() override;
	void						resize() override;
	void						setUIVisibility(bool visible);
private:
	// Settings
	VDSettingsRef				mVDSettings;
	// Session
	VDSessionRef				mVDSession;
	// Log
	VDLogRef					mVDLog;
	// Animation
	VDAnimationRef				mVDAnimation;

	VDTextureList				mTexs;
	fs::path					mTexturesFilepath;
	int							i, x;
	ci::gl::FboRef				mFboA, mFboB, mFboMix;
	vector<ci::gl::FboRef>		mFboBlend;
	gl::GlslProgRef				mGlslA, mGlslB, mGlslMix, mGlslBlend;
	void						renderSceneA();
	void						renderSceneB();
	void						renderMix();
	void						renderBlend();
	int							mCurrentBlend;
};
