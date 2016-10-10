
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// UserInterface
#include "CinderImGui.h"
// Settings
#include "VDSettings.h"
// Session
#include "VDSession.h"
// Log
#include "VDLog.h"
// Utils
#include "VDUtils.h"
// Message router
#include "VDRouter.h"
// Animation
#include "VDAnimation.h"
// Mix
#include "VDMix.h"
// UI
#include "VDUI.h"
// Warping
#include "Warp.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace VideoDromm;

#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

class VideodrommVisualizerApp : public App
{
public:
	static void prepare(Settings *settings);

	void setup() override;
	void cleanup() override;
	void update();
	void draw() override;

	void fileDrop(FileDropEvent event) override;
	void resize() override;

	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;

	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;

	void updateWindowTitle();
	void setUIVisibility(bool visible);
private:
	// Settings
	VDSettingsRef				mVDSettings;
	// Session
	VDSessionRef				mVDSession;
	// Log
	VDLogRef					mVDLog;
	// Utils
	VDUtilsRef					mVDUtils;
	// Message router
	VDRouterRef					mVDRouter;
	// Animation
	VDAnimationRef				mVDAnimation;
	// UI
	VDUIRef						mVDUI;
	//void						showVDUI(unsigned int fps);
	// Mix
	VDMixList					mMixes;
	fs::path					mMixesFilepath;
	// warping
	gl::TextureRef				mImage;
	WarpList					mWarps;
	string						fileWarpsName;
	fs::path					mWarpSettings;
	// fbo
	void						renderSceneToFbo();
	gl::FboRef					mRenderFbo;
	void						renderUIToFbo();
	gl::FboRef					mUIFbo;
	unsigned int				mWarpFboIndex;
	bool						mFadeInDelay;

};
