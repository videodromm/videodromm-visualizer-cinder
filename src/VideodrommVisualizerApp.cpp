#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "MPEApp.hpp"
#include "MPEClient.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mpe;

// NEXT STEP: Create multiple targets
// https://github.com/wdlindmeier/Most-Pixels-Ever-Cinder/wiki/MPE-Setup-Tutorial-for-Cinder#3-create-multiple-targets

class VideoDrommVisualizerApp : public App, public MPEApp
{
public:
	void setup();
    void mpeReset();
    
	void update();
    void mpeFrameUpdate(long serverFrameNumber);
	
    void draw();
    void mpeFrameRender(bool isNewFrame);
    
    MPEClientRef mClient;
};

void VideoDrommVisualizerApp::setup()
{
    mClient = MPEClient::create(this);
}

void VideoDrommVisualizerApp::mpeReset()
{
    // Reset the state of your app.
    // This will be called when any client connects.
}

void VideoDrommVisualizerApp::update()
{
    if (!mClient->isConnected() && (getElapsedFrames() % 60) == 0)
    {
        mClient->start();
    }
}

void VideoDrommVisualizerApp::mpeFrameUpdate(long serverFrameNumber)
{
    // Your update code.
}

void VideoDrommVisualizerApp::draw()
{
    mClient->draw();
}

void VideoDrommVisualizerApp::mpeFrameRender(bool isNewFrame)
{
    gl::clear(Color(0.5,0.5,0.5));
    // Your render code.
}

// If you're deploying to iOS, set the Render antialiasing to 0 for a significant
// performance improvement. This value defaults to 4 (AA_MSAA_4) on iOS and 16 (AA_MSAA_16)
// on the Desktop.
#if defined( CINDER_COCOA_TOUCH )
CINDER_APP( VideoDrommVisualizerApp, RendererGl(RendererGl::AA_NONE) )
#else
CINDER_APP( VideoDrommVisualizerApp, RendererGl )
#endif
