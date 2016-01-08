#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"

#include "MPEApp.hpp"
#include "MPEClient.h"

#include "Warp.h"

// UserInterface
#include "CinderImGui.h"
// Settings
#include "VDSettings.h"
// Utils
#include "VDUtils.h"
// Console
#include "AppConsole.h"

// Choose the threading mode.
// Generally Threaded is the way to go, but if find your app crashing
// because you're making GL calls on a different thread, use the non-threaded client.
// Threaded is used by default. This switch is for demo purposes.
#define USE_THREADED   0


using namespace ci;
using namespace ci::app;
using namespace ph::warping;
using namespace std;
using namespace mpe;
using namespace VideoDromm;

// NEXT STEP: Create multiple targets
// https://github.com/wdlindmeier/Most-Pixels-Ever-Cinder/wiki/MPE-Setup-Tutorial-for-Cinder#3-create-multiple-targets

class VideoDrommVisualizerApp : public App, public MPEApp
{
public:
	static void					prepareSettings(Settings *settings);
	void						setup() override;
	void 						cleanup() override;
	void 						resize() override;
	void 						mouseMove(MouseEvent event) override;
	void 						mouseDown(MouseEvent event) override;
	void 						mouseDrag(MouseEvent event) override;
	void 						mouseUp(MouseEvent event) override;

	void 						keyDown(KeyEvent event) override;
	void 						keyUp(KeyEvent event) override;
	void 						fileDrop(FileDropEvent event) override;

	void 						updateWindowTitle();
	void						loadShader(const fs::path &fragment_path);

	void						mpeReset() override;

	void						update() override;
	void						mpeFrameUpdate(long serverFrameNumber);

	void						draw();
	void						mpeFrameRender(bool isNewFrame);

	MPEClientRef				mClient;
	long						mServerFramesProcessed;
	// Settings
	VDSettingsRef				mVDSettings;
	// Utils
	VDUtilsRef					mVDUtils;
	// imgui
	int							w;
	int							h;
	int							displayHeight;
	int							xPos;
	int							yPos;
	int							largeW;
	int							largeH;
	int							largePreviewW;
	int							largePreviewH;
	int							margin;
	int							inBetween;
	// console
	AppConsoleRef				mConsole;
	bool						showConsole;
	void						ShowAppConsole(bool* opened);

	// warps
	fs::path					mSettings;
	gl::TextureRef				mImage;
	WarpList					mWarps;

	// shaders
	gl::VboMeshRef				mMesh;
	gl::GlslProgRef				mProg;

};
