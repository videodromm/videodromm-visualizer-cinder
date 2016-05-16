
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// window manager
//#include "WindowMngr.h"
// UserInterface
#include "CinderImGui.h"
// Warping
#include "Warp.h"
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
// fbo
//#include "VDFbo.h"
// UnionJack
#include "UnionJack.h"
// spout
#include "spout.h"
// hap codec movie
#include "MovieHap.h"
// Mix
#include "VDMix.h"

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

	void saveThumb();
	void fileDrop(FileDropEvent event) override;
	//void resize() override;
	void resizeWindow();

	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;

	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;

	void updateWindowTitle();
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
	// Mix
	VDMixList					mMixes;
	fs::path					mMixesFilepath;
	bool						mIsResizing;
	//void						createRenderWindow();
	//void						deleteRenderWindows();
	//vector<WindowMngr>			allRenderWindows;
	// imgui
	float						color[4];
	float						backcolor[4];
	int							playheadPositions[12];
	int							speeds[12];
	// mPreviewFboWidth 80 mPreviewFboHeight 60 margin 10 inBetween 15
	int							w;
	int							h;
	int							displayHeight;
	int							xPos;
	int							yPos;
	int							yPosRow1;
	int							yPosRow2;
	int							yPosRow3;
	int							largeW;
	int							largeH;
	int							largePreviewW;
	int							largePreviewH;
	int							margin;
	int							inBetween;

	float						f = 0.0f;
	char						buf[64];

	bool						mouseGlobal;
	// movie
	qtime::MovieGlHapRef		mMovie;
	void loadMovieFile(const fs::path &path);
	bool						mLoopVideo;
	// warping
	gl::TextureRef				mImage;
	WarpList					mWarps;
	string						fileWarpsName;
	//Area						mSrcArea;
	fs::path					mWarpSettings;
	// fbo
	void						renderSceneToFbo();
	gl::FboRef					mRenderFbo;
	void						renderUIToFbo();
	gl::FboRef					mUIFbo;
	bool						mFadeInDelay;
	// timeline
	Anim<float>					mSaveThumbTimer;
	bool						removeUI;
};
