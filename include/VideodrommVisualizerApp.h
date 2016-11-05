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

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace VideoDromm;

#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

class VideodrommVisualizerApp : public App {

public:
	static void prepare(Settings *settings);

	void setup() override;
	void mouseDown(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
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

	// Mix
	VDMixList					mMixes;
	fs::path					mMixesFilepath;
	// handle resizing for imgui
	void						resizeWindow();
	bool						mIsResizing;
	void						updateWindowTitle();
	// imgui
	float						color[4];
	float						backcolor[4];
	int							playheadPositions[12];
	int							speeds[12];

	float						f = 0.0f;
	char						buf[64];
	unsigned int				i, j;

	bool						mouseGlobal;
	//bool						removeUI;
	// shader
	//gl::GlslProgRef				aShader;
	// boolean to update the editor text
	//bool						mShaderTextToLoad; 

	//! default vertex shader
	//std::string					mPassthruVextexShaderString;
	//! default fragment shader
	//std::string					mFboTextureFragmentShaderString;
	string						mError;
	// fbo
	gl::FboRef					mFbo;
	bool						mIsShutDown;
	Anim<float>					mRenderWindowTimer;
	void						positionRenderWindow();
};