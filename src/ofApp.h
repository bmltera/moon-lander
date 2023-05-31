#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include  "ofxAssimpModelLoader.h"
#include "Octree.h"
#include <glm/gtx/intersect.hpp>
#include "Particle.h"
#include "ParticleEmitter.h"

/*
* A class for the lander object
*/
class Lander {
public:
	Lander() { //load and set up the model
		lander.loadModel("geo/ship3.obj");
		lander.setScale(1.3, 1.3, 1.3);
		lander.setScaleNormalization(false);

	}

	//set the position of the lander
	void setLanderPosition(float x, float y, float z) {
		lander.setPosition(x, y, z);
	}

	//set the rotation of the lander
	void setLanderRotation(float angle) {
		lander.setRotation(0, angle, 0, 1, 0);
	}

	//return the header for forward and backward movement using trig
	glm::vec3 header()
	{
		glm::vec3 d = glm::vec3(sin(ofDegToRad(lander.getRotationAngle(0) + 180)), 0, cos(ofDegToRad(lander.getRotationAngle(0) + 180)));
		return glm::normalize(d);
	}

	//return the header for leftward and rightward movement using trig
	glm::vec3 leftRightHeader() {
		glm::vec3 d = glm::vec3(sin(ofDegToRad(lander.getRotationAngle(0) - 90)), 0, cos(ofDegToRad(lander.getRotationAngle(0) - 90)));
		return glm::normalize(d);
	}

	/*
	* integrate function. Move and ratation the lander based on phyics
	*/
	float integrate();

	ofxAssimpModelLoader lander;
	float rotation = 0;

	//physic varibles
	glm::vec3 velocity = glm::vec3(0,0,0);

	glm::vec3 thrustForce = glm::vec3(0, 2, 0);
	glm::vec3 downwardForce = glm::vec3(0, -2, 0);

	//angular varible 
	float angularVelocity = 0;
	float angularAcceleration = 0;
	ofVec3f acceleration = glm::vec3(0, 0, 0);
	float angularForces = 12;
	ofVec3f turbForce;

	float damping = .99;
	float gravity = -0.3;

	//movement boolean variables
	bool moveForward = false;
	bool moveBackward = false;
	bool moveLeft = false;
	bool moveRight = false;
	bool thrustUp = false;
	bool moveDown = false;
	bool rotateLeft = false;
	bool rotateRight = false;

	float fuelTimeLeft = 120;
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent2(ofDragInfo dragInfo);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void initLightingAndMaterials();
		void savePicture();
		void toggleWireframeMode();
		void togglePointsDisplay();
		void toggleSelectTerrain();
		void setCameraTarget();
		bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point);
		void rayAltitudeSensor();
		void checkCollide();
		void applyCollide();
		void checkLanding();
		void thrustLoadVbo();
		void explodeLoadVbo();
		void setThurstEmitter();
		void setExplodeEmitter();
		void gameStart();
		void reset();
		void endGameMsg();
		void startMenu();
		glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 p , glm::vec3 n);

		//camera
		ofEasyCam cam;
		ofEasyCam trackCam;
		ofEasyCam botCam;
		ofEasyCam frontCam;

		//lander and bounding box varibles
		ofxAssimpModelLoader mars;
		ofLight light;
		Box boundingBox, landerBounds;
		Box testBox;
		vector<Box> colBoxList;
		bool bLanderSelected = false;
		Octree octree;
		TreeNode selectedNode;
		TreeNode altitudeNode;
		glm::vec3 mouseDownPos, mouseLastPos;
		bool bInDrag = false;

		ofxIntSlider numLevels;

		//boolean varibles
		bool bAltKeyDown;
		bool bCtrlKeyDown;
		bool bWireframe;
		bool bDisplayPoints;
		bool bDisplayOctree = false;
		bool bDisplayBBoxes = false;
		bool bAltitude = false;
		bool bCollide = false;
		bool bToggleShipLight = false;
		bool clipped = false;
		bool bCrash = false;
		bool bGeneralCam = true;
		bool bTrackCam = false;
		bool bBotCam = false;
		bool bFrontCam = false;
		bool bShowAltitude = true;
		bool bLanding = false;
		bool bFuelOut = false;
		
		bool bLanderLoaded;
		bool bTerrainSelected;

		bool bEndScreen = false;
		bool bWinScreen = false;
		bool bStartGame = false;
	
		ofVec3f selectedPoint;
		ofVec3f intersectPoint;

		vector<Box> bboxList;

		const float selectionRange = 4.0;

		Lander *obj = NULL;
		map<int, bool> keymap;

		float startThrust = 0;
		float altitude = -1;

		// thrust Emitter and some forces;
		//
		ParticleEmitter thrustEmitter;

		TurbulenceForce* turbForce;
		GravityForce* gravityForce;
		ImpulseRadialForce* radialForce;
		CyclicForce* cyclicForce;

		// Explodsion Emitter and some forces;
		//
		ParticleEmitter explodeEmitter;

		TurbulenceForce* explodeTurbForce;
		GravityForce* explodeGravityForce;
		ImpulseRadialForce* explodeRadialForce;
		CyclicForce* explodeCyclicForce;

		// textures
		//
		ofTexture  particleTex;

		// shaders
		//
		ofVbo vbo;
		ofShader shader;

		// lights
		ofLight light1, light2, light3, shipLight;
		ofImage backgroundImage;

		//sound
		ofSoundPlayer thrustSound;
		ofSoundPlayer explodeSound;
		ofSoundPlayer collideSound;

		//coordinates of the landing areas
		glm::vec3 landingArea1 = glm::vec3(0.129794, 0, 17.3758);
		glm::vec3 landingArea2 = glm::vec3(2.80003, 0, -76.8603);
		glm::vec3 landingArea3 = glm::vec3(-43.1438, 0, 96.1508);
		vector<glm::vec3> landingAreas;
		float landingAreaRadius = 2.5;
		float landerHalfLength = 1;

		//different scores
		float score1 = 100;
		float score2 = 200;
		float score3 = 300;
		float finalScore = 0;;
};
