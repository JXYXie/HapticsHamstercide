//==============================================================================
/*
	Haptic Hamstercide Game
	Version: 0.0.7 (Apr 15, 2019)
	Authors: Jack Xie & Alan Fung

*/
//==============================================================================
#include <ctime>
#include <time.h>
#include <string>
#include <iostream>

//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
	C_STEREO_DISABLED:            Stereo is disabled
	C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
	C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
	C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;

//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld *world;

// a camera to render the world in the window display
cCamera *camera;

// game background
cBackground* background;

// a light source to illuminate the objects in the world
cDirectionalLight *light;

//------------------------------------------------------------------------------
// Sound
//------------------------------------------------------------------------------
// audio device to play sound
cAudioDevice* audioDevice;
// audio buffers to store sound files
cAudioBuffer* audioGroundImpact;
cAudioBuffer* audioGroundTouch;
cAudioBuffer* audioHamsterImpact;
cAudioBuffer* audioHamsterTouch;
cAudioBuffer* audioHamsterHit;

cAudioSource* audioSourceHit;

/*
  0 = bottom
  1 = upwards
  2 = top
  3 = downwards
  4 = unconcious
 */
vector<vector<int>> hamsterState(3, vector<int>(3));
// objects
vector<vector<cMultiMesh *>> hamsters;

cMultiMesh *hammer;
cMultiMesh *game_world;

// a haptic device handler
cHapticDeviceHandler *handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a virtual tool representing the haptic device in the scene
cToolCursor *tool;

// a font for rendering text
cFontPtr font;
cFontPtr scoreFont;

// a label to display the rate [Hz] at which the simulation is running
cLabel *labelRates;
cLabel *labelScore;

// a flag that indicates if the haptic simulation is currently running
bool simulationRunning = false;

// a flag that indicates if the haptic simulation has terminated
bool simulationFinished = true;

// display level for collision tree
int collisionTreeDisplayLevel = 0;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread *hapticsThread;

// a handle to window display context
GLFWwindow *window = NULL;

// current width of window
int width = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;

// root resource path
string resourceRoot;

// define the radius of the tool (sphere)
double toolRadius = 0.2;

//------------------------------------------------------------------------------
// GAME VARIABLES
//------------------------------------------------------------------------------
int hits;
int misses;
int score;
int hiscore;

bool raised = true;
bool vibrate = false;

cVector3d camPos = cVector3d(2.0, 0.0, 1.5);
cVector3d camLook = cVector3d(0.0, 0.0, 0.0);

//------------------------------------------------------------------------------
// DECLARED MACROS
//------------------------------------------------------------------------------

// convert to resource path
#define RESOURCE_PATH(p) (char *)((resourceRoot + string(p)).c_str())

//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

void startGame();

// callback when the window display is resized
void windowSizeCallback(GLFWwindow *a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char *a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow *a_window, int a_key, int a_scancode, int a_action, int a_mods);

// callback to render graphic scene
void updateGraphics(void);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);

int main(int argc, char *argv[])
{
	//--------------------------------------------------------------------------
	// INITIALIZATION
	//--------------------------------------------------------------------------

	cout << endl;
	cout << "-----------------------------------" << endl;
	cout << "CHAI3D Haptic Hamstercide" << endl;
	cout << "Copyright 2003-2019" << endl;
	cout << "-----------------------------------" << endl
		 << endl
		 << endl;
	cout << "Keyboard Options:" << endl
		 << endl;
	cout << "[f] - Enable/Disable full screen mode" << endl;
	cout << "[m] - Enable/Disable vertical mirroring" << endl;
	cout << "[q] - Exit application" << endl;
	cout << endl
		 << endl;

	// parse first arg to try and locate resources
	resourceRoot = string(argv[0]).substr(0, string(argv[0]).find_last_of("/\\") + 1);

	//--------------------------------------------------------------------------
	// OPEN GL - WINDOW DISPLAY
	//--------------------------------------------------------------------------

	// initialize GLFW library
	if (!glfwInit())
	{
		cout << "failed initialization" << endl;
		cSleepMs(1000);
		return 1;
	}

	// set error callback
	glfwSetErrorCallback(errorCallback);

	// compute desired size of window
	const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int w = 0.8 * mode->height;
	int h = 0.5 * mode->height;
	int x = 0.5 * (mode->width - w);
	int y = 0.5 * (mode->height - h);

	// set OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	// set active stereo mode
	if (stereoMode == C_STEREO_ACTIVE)
	{
		glfwWindowHint(GLFW_STEREO, GL_TRUE);
	}
	else
	{
		glfwWindowHint(GLFW_STEREO, GL_FALSE);
	}

	// create display context
	window = glfwCreateWindow(w, h, "CHAI3D", NULL, NULL);
	if (!window)
	{
		cout << "failed to create window" << endl;
		cSleepMs(1000);
		glfwTerminate();
		return 1;
	}

	// get width and height of window
	glfwGetWindowSize(window, &width, &height);

	// set position of window
	glfwSetWindowPos(window, x, y);

	// set key callback
	glfwSetKeyCallback(window, keyCallback);

	// set resize callback
	glfwSetWindowSizeCallback(window, windowSizeCallback);

	// set current display context
	glfwMakeContextCurrent(window);

	// sets the swap interval for the current display context
	glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
	// initialize GLEW library
	if (glewInit() != GLEW_OK)
	{
		cout << "failed to initialize GLEW library" << endl;
		glfwTerminate();
		return 1;
	}
#endif

	//--------------------------------------------------------------------------
	// WORLD - CAMERA - LIGHTING
	//--------------------------------------------------------------------------

	// create a new world.
	world = new cWorld();

	// set the background color of the environment
	world->m_backgroundColor.setBlack();

	// create a camera and insert it into the virtual world
	camera = new cCamera(world);
	world->addChild(camera);

	// define a basis in spherical coordinates for the camera
	camera->set(camPos,					   // camera position (eye)
				camLook,				   // look at position (target)
				cVector3d(0.0, 0.0, 1.0)); // direction of the (up) vector

	// set the near and far clipping planes of the camera
	// anything in front or behind these clipping planes will not be rendered
	camera->setClippingPlanes(0.001, 1000.0);

	// set stereo mode
	camera->setStereoMode(stereoMode);

	// set stereo eye separation and focal length (applies only if stereo is enabled)
	camera->setStereoEyeSeparation(0.01);
	camera->setStereoFocalLength(0.5);

	// set vertical mirrored display mode
	camera->setMirrorVertical(mirroredDisplay);

	// enable multi-pass rendering to handle transparent objects
	camera->setUseMultipassTransparency(true);

	// create a light source
	light = new cDirectionalLight(world);

	// attach light to camera
	camera->addChild(light);

	// enable light source
	light->setEnabled(true);

	// define the direction of the light beam
	light->setDir(-3.0, -0.5, 0.0);

	// set lighting conditions
	light->m_ambient.set(0.4f, 0.4f, 0.4f);
	light->m_diffuse.set(0.8f, 0.8f, 0.8f);
	light->m_specular.set(1.0f, 1.0f, 1.0f);

	//--------------------------------------------------------------------------
	// HAPTIC DEVICES / TOOLS
	//--------------------------------------------------------------------------

	// create a haptic device handler
	handler = new cHapticDeviceHandler();

	// get access to the first available haptic device found
	handler->getDevice(hapticDevice, 0);

	// retrieve information about the current haptic device
	cHapticDeviceInfo hapticDeviceInfo = hapticDevice->getSpecifications();

	// create a tool (cursor) and insert into the world
	tool = new cToolCursor(world);
	world->addChild(tool);

	// connect the haptic device to the virtual tool
	tool->setHapticDevice(hapticDevice);

	// if the haptic device has a gripper, enable it as a user switch
	hapticDevice->setEnableGripperUserSwitch(true);

	// define a radius for the tool
	tool->setRadius(toolRadius);

	// hide the device sphere. only show proxy.
	tool->setShowContactPoints(false, false);

	// create a white cursor
	//tool->m_hapticPoint->m_sphereProxy->m_material->setWhite();

	// map the physical workspace of the haptic device to a larger virtual workspace.
	tool->setWorkspaceRadius(1.0);

	tool->enableDynamicObjects(true);

	// oriente tool with camera
	//tool->setLocalRot(camera->getLocalRot());

	// haptic forces are enabled only if small forces are first sent to the device;
	// this mode avoids the force spike that occurs when the application starts when
	// the tool is located inside an object for instance.
	tool->setWaitForSmallForce(true);

	// start the haptic tool
	tool->start();

	// read the scale factor between the physical workspace of the haptic
	// device and the virtual workspace defined for the tool
	double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

	// stiffness properties
	double maxStiffness = hapticDeviceInfo.m_maxLinearStiffness / workspaceScaleFactor;

	//--------------------------------------------------------------------------
	// SETUP AUDIO MATERIAL
	//--------------------------------------------------------------------------

	// create an audio device to play sounds
	audioDevice = new cAudioDevice();

	// attach audio device to camera
	camera->attachAudioDevice(audioDevice);

	audioGroundImpact = new cAudioBuffer();
	audioGroundImpact->loadFromFile("resources/sounds/ground_impact.wav");

	audioGroundTouch = new cAudioBuffer();
	audioGroundTouch->loadFromFile("resources/sounds/ground_scrape.wav");

	audioHamsterImpact = new cAudioBuffer();
	audioHamsterImpact->loadFromFile("resources/sounds/hamster_impact.wav");

	audioHamsterTouch = new cAudioBuffer();
	audioHamsterTouch->loadFromFile("resources/sounds/hamster_squeak.wav");

	audioHamsterHit = new cAudioBuffer();
	audioHamsterHit->loadFromFile("resources/sounds/hamster_hit.wav");

	audioSourceHit = new cAudioSource;

	audioSourceHit->setAudioBuffer(audioHamsterHit);
	audioSourceHit->setGain(1.4);

	// here we convert all files to mono. this allows for 3D sound support. if this code
	// is commented files are kept in stereo format and 3D sound is disabled. Compare both!
	audioGroundImpact->convertToMono();
	audioGroundTouch->convertToMono();
	audioHamsterImpact->convertToMono();
	audioHamsterTouch->convertToMono();
	audioHamsterHit->convertToMono();

	// create an audio source for this tool.
	tool->createAudioSource(audioDevice);

	//--------------------------------------------------------------------------
	// Game World Object
	//--------------------------------------------------------------------------
	game_world = new cMultiMesh();

	game_world->loadFromFile("resources/models/game_world.obj");
	game_world->setLocalPos(cVector3d(0.0, 0.0, -0.2));

	// define a default stiffness for the object
	game_world->setStiffness(0.9 * maxStiffness, true);

	game_world->computeBoundaryBox(true);
	// enable display list for faster graphic rendering
	game_world->setUseDisplayList(true);

	game_world->setUseTransparency(false, true);

	game_world->setUseCulling(false);

	game_world->setFriction(0.75, 0.5, true);

	// set audio properties
	for (int i = 0; i < (game_world->getNumMeshes()); i++) {
		(game_world->getMesh(i))->m_material->setAudioFrictionBuffer(audioGroundTouch);
		(game_world->getMesh(i))->m_material->setAudioFrictionGain(0.4);
		(game_world->getMesh(i))->m_material->setAudioFrictionPitchGain(0.2);
		(game_world->getMesh(i))->m_material->setAudioFrictionPitchOffset(0);
		(game_world->getMesh(i))->m_material->setAudioImpactBuffer(audioGroundImpact);
		(game_world->getMesh(i))->m_material->setAudioImpactGain(0.5);
	}

	// compute collision detection algorithm
	game_world->createAABBCollisionDetector(toolRadius);

	//--------------------------------------------------------------------------
	// Hamster Objects
	//--------------------------------------------------------------------------

	srand(time(NULL));

	startGame();

	//--------------------------------------------------------------------------
	// Hammer Object
	//--------------------------------------------------------------------------
	hammer = new cMultiMesh();
	// add hammer to tool
	tool->m_image = hammer;
	hammer->loadFromFile("resources/models/hammer.obj");

	// compute collision detection algorithm
	hammer->createAABBCollisionDetector(toolRadius);

	// define a default stiffness for the object
	hammer->setStiffness(0.9 * maxStiffness, true);

	hammer->computeBoundaryBox(true);
	//hammer->setShowBoundaryBox(true);
	// enable display list for faster graphic rendering
	hammer->setUseDisplayList(true);

	hammer->setUseTransparency(false, true);

	hammer->setUseCulling(false);

	//--------------------------------------------------------------------------
	// WIDGETS
	//--------------------------------------------------------------------------

	// create a font
	font = NEW_CFONTCALIBRI20();
	scoreFont = NEW_CFONTCALIBRI40();

	// create a label to display the haptic and graphic rate of the simulation
	labelRates = new cLabel(font);
	labelRates->m_fontColor.setBlack();
	camera->m_frontLayer->addChild(labelRates);

	labelScore = new cLabel(scoreFont);
	labelScore->m_fontColor.setRedCrimson();
	camera->m_frontLayer->addChild(labelScore);

	// create a background
	background = new cBackground();

	background->loadFromFile("resources/images/background.jpg");
	camera->m_backLayer->addChild(background);

	// set background properties
	background->setCornerColors(cColorf(0.95f, 0.95f, 0.95f),
								cColorf(0.95f, 0.95f, 0.95f),
								cColorf(0.80f, 0.80f, 0.80f),
								cColorf(0.80f, 0.80f, 0.80f));

	//--------------------------------------------------------------------------
	// START SIMULATION
	//--------------------------------------------------------------------------

	// create a thread which starts the main haptics rendering loop
	hapticsThread = new cThread();
	hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

	// setup callback when application exits
	atexit(close);

	//--------------------------------------------------------------------------
	// MAIN GRAPHIC LOOP
	//--------------------------------------------------------------------------

	// call window size callback at initialization
	windowSizeCallback(window, width, height);

	// main graphic loop
	while (!glfwWindowShouldClose(window))
	{
		// get width and height of window
		glfwGetWindowSize(window, &width, &height);

		// render graphics
		updateGraphics();

		// swap buffers
		glfwSwapBuffers(window);

		// process events
		glfwPollEvents();

		// signal frequency counter
		freqCounterGraphics.signal(1);
	}

	// close window
	glfwDestroyWindow(window);

	// terminate GLFW library
	glfwTerminate();

	// exit
	return 0;
}

void startGame() {


	world->addChild(game_world);
	for (int i = 0; i < 3; ++i)
	{
		hamsters.push_back(vector<cMultiMesh *>());
		for (int j = 0; j < 3; ++j)
		{
			// create a virtual mesh
			cMultiMesh *hamster = new cMultiMesh();
			hamster->loadFromFile("resources/models/hamster.obj");
			hamster->setUseTransparency(false, true);
			hamster->m_name = "hamster" + to_string(i * 3 + j);

			// add object to world
			world->addChild(hamster);

			// disable culling so that faces are rendered on both sides
			hamster->setUseCulling(false);

			// compute a boundary box
			hamster->computeBoundaryBox(true);

			// show/hide boundary box
			hamster->setShowBoundaryBox(false);

			// define a default stiffness for the object
			hamster->setStiffness(0.1 * 100, true);

			// define some haptic friction properties
			hamster->setFriction(0.4, 0.2, true);

			// enable display list for faster graphic rendering
			hamster->setUseDisplayList(true);

			// set location of objects
			hamster->setLocalPos(cVector3d((double)(i - 1) * 1, (double)(j - 1) * 1, -0.8));

			// compute all edges of object for which adjacent triangles have more than 40 degree angle
			hamster->computeAllEdges(40);

			// set audio properties
			for (int i = 0; i < (hamster->getNumMeshes()); i++) {
				(hamster->getMesh(i))->m_material->setAudioFrictionBuffer(audioHamsterTouch);
				(hamster->getMesh(i))->m_material->setAudioFrictionGain(0.8);
				(hamster->getMesh(i))->m_material->setAudioFrictionPitchGain(0.8);
				(hamster->getMesh(i))->m_material->setAudioFrictionPitchOffset(0.8);
				(hamster->getMesh(i))->m_material->setAudioImpactBuffer(audioHamsterImpact);
				(hamster->getMesh(i))->m_material->setAudioImpactGain(0.8);
			}

			// compute collision detection algorithm
			hamster->createAABBCollisionDetector(toolRadius);

			hamsters[i].push_back(hamster);
		}
	}
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow *a_window, int a_width, int a_height)
{
	// update window size
	width = a_width;
	height = a_height;
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char *a_description)
{
	cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow *a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
	// filter calls that only include a key press
	if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
	{
		return;
	}

	// option - exit
	else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
	{
		glfwSetWindowShouldClose(a_window, GLFW_TRUE);
	}

	// option - toggle fullscreen
	else if (a_key == GLFW_KEY_F)
	{
		// toggle state variable
		fullscreen = !fullscreen;

		// get handle to monitor
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();

		// get information about monitor
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);

		// set fullscreen or window mode
		if (fullscreen)
		{
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			glfwSwapInterval(swapInterval);
		}
		else
		{
			int w = 0.8 * mode->height;
			int h = 0.5 * mode->height;
			int x = 0.5 * (mode->width - w);
			int y = 0.5 * (mode->height - h);
			glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
			glfwSwapInterval(swapInterval);
		}
	}

	// option - toggle vertical mirroring
	else if (a_key == GLFW_KEY_M)
	{
		mirroredDisplay = !mirroredDisplay;
		camera->setMirrorVertical(mirroredDisplay);
	}
}

//------------------------------------------------------------------------------

void close(void)
{
	// stop the simulation
	simulationRunning = false;

	// wait for graphics and haptics loops to terminate
	while (!simulationFinished)
	{
		cSleepMs(100);
	}

	// close haptic device
	tool->stop();

	// delete resources
	delete hapticsThread;
	delete world;
	delete handler;
	delete audioDevice;
	delete audioGroundImpact;
	delete audioGroundTouch;
	delete audioHamsterImpact;
	delete audioHamsterTouch;
	delete audioHamsterHit;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
	/////////////////////////////////////////////////////////////////////
	// UPDATE WIDGETS
	/////////////////////////////////////////////////////////////////////

	// update haptic and graphic rate data
	labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
						cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

	// update position of label
	labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);

	// update scores
	labelScore->setText("HITS: " + to_string(hits) + " " + "MISSES: " + to_string(misses));

	// update position of label
	labelScore->setLocalPos((int)(0.5 * (width - labelScore->getWidth())), 0.925 * height);

	/////////////////////////////////////////////////////////////////////
	// RENDER SCENE
	/////////////////////////////////////////////////////////////////////

	// update shadow maps (if any)
	world->updateShadowMaps(false, mirroredDisplay);

	// render world
	camera->renderView(width, height);

	// wait until all GL commands are completed
	glFinish();

	// check for any OpenGL errors
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
		cout << "Error: " << gluErrorString(err) << endl;
}

//------------------------------------------------------------------------------

enum cMode
{
	IDLE,
	SELECTION
};

void updateHaptics(void)
{
	cMode state = IDLE;
	cGenericObject *selectedObject = NULL;
	cGenericObject *collidedObject = NULL;
	cTransform tool_T_object;

	// simulation in now running
	simulationRunning = true;
	simulationFinished = false;

	cPrecisionClock timeClock;
	timeClock.start();

	cPrecisionClock vibrateTimer;

	cPrecisionClock forceClock;
	forceClock.reset();

	cVector3d devicePositionPrevious = tool->getDeviceLocalPos();

	// main haptic simulation loop
	while (simulationRunning)
	{
		forceClock.stop();
		double forceTimeInterval = forceClock.getCurrentTimeSeconds();
		forceClock.reset();
		forceClock.start();

		double vibrateInterval = vibrateTimer.getCurrentTimeSeconds();

		/////////////////////////////////////////////////////////////////////////
		// Game Loop
		/////////////////////////////////////////////////////////////////////////
		// Reset missed flag when hammer moves up
		if (tool->getDeviceLocalLinVel().z() > 4)
		{
			raised = true;
		}

		/////////////////////////////////////////////////////////////////////////
		// Hamster Movements
		/////////////////////////////////////////////////////////////////////////

		for (int k = 0; k < 25; k++)
		{
			int i = rand() % 3;
			int j = rand() % 3;
			cVector3d hamsterPos = hamsters[i][j]->getLocalPos();
			// Hamster is in bottom position
			if (hamsterState[i][j] == 0)
			{
				int random = (rand() % 10000) + 1;
				if (random <= 2)
				{
					hamsterState[i][j] = 1;
				}
			}
			// Hamster is in moving upwards
			else if (hamsterState[i][j] == 1)
			{
				// And is not at the top yet
				if (hamsterPos.z() < -0.199)
				{
					hamsters[i][j]->translate(cVector3d(0.0, 0.0, 0.001));
				}
				// And is at the top
				else
				{
					hamsterState[i][j] = 2;
				}
			}
			// Hamster is at the top
			else if (hamsterState[i][j] == 2)
			{
				int random = (rand() % 10000) + 1;
				if (random <= 4)
				{
					hamsterState[i][j] = 4;
				}
			}
			// Hamster is moving downwards
			else if (hamsterState[i][j] == 3) {
				// And is not at the bottom yet
				if (hamsterPos.z() > -0.8) {
					hamsters[i][j]->translate(cVector3d(0.0, 0.0, -0.001));
				}
				// And is at the bottom
				else
				{
					hamsterState[i][j] = 0;
				}
			}
			// Hamster is knocked out
			else
			{
				// But is not at the bottom
				if (hamsterPos.z() > -0.8)
				{
					// Then move to the bottom
					hamsters[i][j]->translate(cVector3d(0.0, 0.0, -0.0015));
				}
				else
				{
					// Low chance of hamster coming back to bottom state
					int random = (rand() % 10000) + 1;
					if (random <= 1)
					{
						hamsterState[i][j] = 0;
					}
				}
			}
		}

		/////////////////////////////////////////////////////////////////////////
		// HAPTIC RENDERING
		/////////////////////////////////////////////////////////////////////////

		// signal frequency counter
		freqCounterHaptics.signal(1);

		/////////////////////////////////////////
		cVector3d devicePositionCurrent = tool->getDeviceLocalPos();
		cVector3d deviceDelta = devicePositionCurrent - devicePositionPrevious;
		devicePositionPrevious = devicePositionCurrent;

		tool->translate(cVector3d(deviceDelta.x(), deviceDelta.y(), 0));

		camera->translate(cVector3d(deviceDelta.x(), deviceDelta.y(), 0));

		// compute global reference frames for each object
		world->computeGlobalPositions(true);

		// update position and orientation of tool
		tool->updateFromDevice();

		// compute interaction forces
		tool->computeInteractionForces();
		//Calculate elapsed time

		if (vibrate && vibrateInterval > 0.4) {
			vibrate = false;
		}

		if (vibrate) {
			// Vibration effect
			double timer = timeClock.getCPUTimeSeconds();

			double vibrateX = sin(2.0 * M_PI * 120.0 * timer);
			double vibrateY = cos(2.0 * M_PI * 120.0 * timer);

			tool->addDeviceLocalForce(cVector3d(1.5* vibrateX, 1.5* vibrateY, 0.0));
		}

		// When there is a collision
		if (tool->m_hapticPoint->getNumCollisionEvents() > 0)
		{
			// get contact event
			cCollisionEvent *collisionEvent = tool->m_hapticPoint->getCollisionEvent(0);

			// get object from contact event
			collidedObject = collisionEvent->m_object->getParent();

			double zForce = tool->getDeviceGlobalForce().z();

			// Make sure the hammer movement was an attempt to hit something (It has to be fast enough)
			if (tool->getDeviceLocalLinVel().z() < -9)
			{
				// If the collided object is a hamster
				if (collidedObject->m_name[0] == 'h')
				{
					// Get the id of hamster hit
					int hamsterID;
					hamsterID = int(collidedObject->m_name[7]) - '0';
					int i = hamsterID / 3;
					int j = hamsterID % 3;
					// If the hamster is not hiding
					if (hamsterState[i][j] != 0)
					{
						// Apply reaction force
						const double forceMultiplier = 6.0;
						cVector3d pos = collidedObject->getLocalPos();

						// Force effect
						double ReactionForceY = cMax(pow(cAbs(tool->getDeviceLocalLinVel().z()), 1.2), 2.0);
						cVector3d ReactionForce = cVector3d(-(tool->getDeviceLocalLinVel().x()), -(tool->getDeviceLocalLinVel().y()), -ReactionForceY);
						tool->addDeviceLocalForce(ReactionForce);

						// Move hamsters down forcefully
						double posZ = cClamp(pos.z() - forceMultiplier * forceTimeInterval * zForce, -0.8, -0.2);
						collidedObject->setLocalPos(pos.x(), pos.y(), posZ);

						// Hammer is no longer in raised position
						raised = false;

						// If hamster is not knocked out
						if (hamsterState[i][j] != 5)
						{
							hamsterState[i][j] = 5;
							vibrateTimer.start();
							vibrate = true;
							hits++;
							audioSourceHit->play();
						}
					}
				}
				// Missed hamster
				else if (collidedObject->m_name[0] != 'h' && raised)
				{
					raised = false;
					misses++;
				}
			}
		}
		// send forces to haptic device
		tool->applyToDevice();
	}

	// exit haptics thread
	simulationFinished = true;
}

//------------------------------------------------------------------------------
