#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLWindowSettings settings;
    settings.setSize(2560, 1440);
    settings.windowMode = OF_WINDOW;
    settings.setGLVersion(3, 2);
    ofCreateWindow(settings);
    ofRunApp(make_shared<ofApp>());
}
