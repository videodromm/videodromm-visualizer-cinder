#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/gl/Fbo.h"
#include "Resources.h"

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
// Image sequence
#include "VDImageSequence.h"
// UnionJack
#include "UnionJack.h"
// spout
#include "spout.h"
// hap codec movie
#include "MovieHap.h"

using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;
using namespace VideoDromm;

class VideodrommVisualizerApp : public App {
public:
	static void prepare(Settings *settings);

	void setup() override;
	void cleanup() override;
	void update() override;
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
	// Image sequence
	vector<VDImageSequenceRef>	mVDImageSequences;
	// UnionJack
	vector<UnionJack>			mDisplays;
	std::string					str;
	std::string					targetStr;
	int							strSize;
	void						shift_left(std::size_t offset, std::size_t X);
	Color						mBlack = Color::black();
	Color						mBlue = Color8u(66, 161, 235);
	Color						mDarkBlue = Color8u::hex(0x1A3E5A);
	Color						mRed = Color8u(240, 0, 0);
	bool						mHorizontalAnimation;
	map<int, bool>				mIndexes;
	// tempo 
	float						bpm;
	float						fpb;
	// fbo
	void						renderSceneToFbo();
	gl::FboRef					mFbo;
	static const int			FBO_WIDTH = 640, FBO_HEIGHT = 480;
	// lines
	void						buildMeshes();
	unsigned int				mPoints = 50;
	unsigned int				mLines = 50;
	bool						mShowHud;
	gl::BatchRef				mLineBatch;
	gl::BatchRef				mMaskBatch;

	gl::TextureRef				mTexture;
	gl::GlslProgRef				mShader;
	CameraPersp					mCamera;
	mat4						mTextureMatrix;

	// movie
	qtime::MovieGlHapRef		mMovie;
	void loadMovieFile(const fs::path &path);
	bool						mLoopVideo;
	// warping
	gl::TextureRef				mImage;
	WarpList					mWarps;
	Area						mSrcArea;
	fs::path					mSettings;

};
