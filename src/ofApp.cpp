
//--------------------------------------------------------------
// Jun Wu, Bill Li
//  Date: <Dec 09, 2022>


#include "ofApp.h"
#include "Util.h"

/*
* integrate function. Move and rotation the lander based on physics
*/
float Lander :: integrate() {
	float dt;
	float framerate = ofGetFrameRate();
	if (framerate == 0) //check if the framerate is 0
		dt = 1;
	else
		dt = 1 / framerate;

	//change the lander position based on the velocity
	glm::vec3 pos = lander.getPosition();
	pos += (velocity * dt);
	setLanderPosition(pos.x, pos.y, pos.z);

	glm::vec3 accel = acceleration;

	//lander thrust up or move down
	if (thrustUp) {
		accel += thrustForce;
	}
	if (moveDown) {
		accel += downwardForce;
	}

	//lander forward, backward, left and right movement
	if (moveForward) {
		glm::vec3 forces = (header() * 3);
		accel += forces;
	}
	else if (moveBackward){
		glm::vec3 forces = (header() * 3);
		accel -= forces;
	}
	else if (moveLeft) {
		glm::vec3 forces = (leftRightHeader() * 3);
		accel += forces;
	}
	else if (moveRight) {
		glm::vec3 forces = (leftRightHeader() * 3);
		accel -= forces;
	}

	accel += turbForce; //add turblence force
	velocity += accel * dt; //add accel to the velocity
	velocity *= damping; //add damping

	//lander rotation
	float rot = lander.getRotationAngle(0);
	rot += (angularVelocity * dt);
	setLanderRotation(rot);

	float a = angularAcceleration;
	if (rotateLeft)
		a += angularForces;
	if (rotateRight)
		a -= angularForces;

	angularVelocity += a * dt;
	angularVelocity *= damping;

	turbForce.set(0, 0, 0); //reset turbForce

	return (angularVelocity * dt); //return the angular velocity for the lighting
}


//--------------------------------------------------------------
// setup scene, lighting, state and load geometry
//
void ofApp::setup(){
	ofSetFrameRate(60);
	bWireframe = false;
	bDisplayPoints = false;
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bLanderLoaded = true;
	bTerrainSelected = true;
	ofSetVerticalSync(true);

	//set up the camares
	cam.setNearClip(.1);
	cam.setFov(65.5);   // approx equivalent to 28mm in 35mm format
	cam.disableMouseInput();
	trackCam.setDistance(100);
	trackCam.setNearClip(.1);
	trackCam.setFov(65.5);
	trackCam.disableMouseInput();
	trackCam.setPosition(20, 15, 0);
	botCam.setNearClip(.1);
	botCam.setFov(65.5);
	botCam.disableMouseInput();
	frontCam.setNearClip(.1);
	frontCam.setFov(65.5);
	frontCam.disableMouseInput();

	ofEnableSmoothing();
	ofEnableDepthTest();

	ofDisableArbTex();     // disable rectangular textures

	// load textures
	//
	if (!ofLoadImage(particleTex, "images/dot.png")) {
		cout << "Particle Texture File: images/dot.png not found" << endl;
		ofExit();
	}

	// load background image
	if (!backgroundImage.load("images/background.jpg")) {
		cout << "Can't load image" << endl;
		ofExit();
	}

	// load the shader
	//
	shader.load("shaders_gles/shader");

	//load the sounds
	thrustSound.load("sounds/thrustSound.mp3");
	thrustSound.setLoop(true);
	explodeSound.load("sounds/explosion2.mp3");
	explodeSound.setVolume(0.5f);
	collideSound.load("sounds/collideSound.mp3");
	collideSound.setVolume(0.2f);

	//set up the thurst emitter
	setThurstEmitter();

	//set up the explode emitter
	setExplodeEmitter();

	// setup rudimentary lighting 
	//
	initLightingAndMaterials();

	mars.loadModel("geo/terrain8.obj");
	mars.setScaleNormalization(false);

	//creating the lander object for the lander model
	obj = new Lander();
	obj->lander.setScaleNormalization(false);
	obj->setLanderPosition(37, 30, 57);
	obj->setLanderRotation(obj->rotation);
	obj->turbForce.set(0, 0, 0);

	//set the starting camera postion
	setCameraTarget();
	botCam.setPosition(ofVec3f(obj->lander.getPosition()));
	botCam.setOrientation(ofVec3f(-90, 0, 0));
	
	ofLoadImage(particleTex, "images/dot.png");

	//  Create Octree for testing.
	octree.create(mars.getMesh(0), 20);
	
	testBox = Box(Vector3(3, 3, 0), Vector3(5, 5, 2));

	landingAreas.push_back(landingArea1);
	landingAreas.push_back(landingArea2);
	landingAreas.push_back(landingArea3);
}

// load vertex buffer for thrust emitter in preparation for rendering
//
void ofApp::thrustLoadVbo() {
	if (thrustEmitter.sys->particles.size() < 1) return;

	vector<ofVec3f> sizes;
	vector<ofVec3f> points;
	for (int i = 0; i < thrustEmitter.sys->particles.size(); i++) {
		points.push_back(thrustEmitter.sys->particles[i].position);
		sizes.push_back(ofVec3f(5));
	}
	// upload the data to the vbo
	//
	int total = (int)points.size();
	vbo.clear();
	vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);
	vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);
}

// load vertex buffer for explode emitter in preparation for rendering
//
void ofApp::explodeLoadVbo() {
	if (explodeEmitter.sys->particles.size() < 1) return;

	vector<ofVec3f> sizes;
	vector<ofVec3f> points;
	for (int i = 0; i < explodeEmitter.sys->particles.size(); i++) {
		points.push_back(explodeEmitter.sys->particles[i].position);
		sizes.push_back(ofVec3f(20));
	}
	// upload the data to the vbo
	//
	int total = (int)points.size();
	vbo.clear();
	vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);
	vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);
}
 
//--------------------------------------------------------------
// incrementally update scene (animation)
//
void ofApp::update() {
	if (bStartGame) {
		ofSeedRandom();

		// set ship light location
		ofVec3f landerPos = obj->lander.getPosition();
		landerPos.y -= 5;
		shipLight.setPosition(obj->lander.getPosition());
		light3.setPosition(landerPos);

		//update position, rotaketion or target of camares
		trackCam.setTarget(landerPos);
		botCam.setPosition(ofVec3f(landerPos.x, landerPos.y - 0.2, landerPos.z));
		botCam.setOrientation(ofVec3f(-90, obj->lander.getRotationAngle(0), 0));
		float angle = obj->lander.getRotationAngle(0);
		frontCam.setPosition(ofVec3f(landerPos.x - 4 * sin(ofDegToRad(angle)), landerPos.y + 2, landerPos.z - 4 * cos(ofDegToRad(angle))));
		frontCam.setOrientation(ofVec3f(0, angle, 0));

		//toggle for the light pointing to the ship
		if (bToggleShipLight) {
			shipLight.enable();
		}
		else {
			shipLight.disable();
		}

		if (keymap[' ']) { //space key to thrust upward
			if (obj->fuelTimeLeft > 0) {
				startThrust = 1 / ofGetFrameRate(); 
				obj->thrustUp = true;
				thrustEmitter.sys->reset(); //start the thrust emitter for the visual effect
				thrustEmitter.start();

				if (!thrustSound.isPlaying()) //play thrust sound effect
					thrustSound.play();
			}
			else { //if the lander is out of fuel, lander can't move upward
				obj->thrustUp = false;
				obj->fuelTimeLeft = 0;
				bFuelOut = true;
			}
		}
		if (keymap['d'] || keymap['D']) { //d key to move downward
			obj->moveDown = true;
		}
		if (keymap[OF_KEY_UP]) { //up arrow to move forward
			obj->moveForward = true;
		}
		if (keymap[OF_KEY_DOWN]) { //down arrow to move backward
			obj->moveBackward = true;
		}
		if (keymap[OF_KEY_LEFT]) { //left arrow to move leftward
			obj->moveLeft = true;
		}
		if (keymap[OF_KEY_RIGHT]) { //right arrow to move rightward
			obj->moveRight = true;
		}
		if (keymap['z'] || keymap['Z']) { //z key to rotake left
			obj->rotateLeft = true;
		}
		if (keymap['x'] || keymap['X']) { //x key to rotake right
			obj->rotateRight = true;
		}

		//update the gravity and the turblence forces
		obj->acceleration = glm::vec3(0, obj->gravity, 0);
		obj->turbForce.x += ofRandom(-0.13, 0.13);
		obj->turbForce.y += ofRandom(-0.01, 0.01);
		obj->turbForce.z += ofRandom(-0.13, 0.13);

		//check and update the altitude between the lander and the terrain
		rayAltitudeSensor();
		if (bAltitude) {
			ofVec3f p = octree.mesh.getVertex(altitudeNode.points[0]);
			altitude = obj->lander.getPosition().y - p.y;
		}

		//check if lander collide with the terrain
		checkCollide();

		//reaction for the collidesion
		applyCollide();

		//if mouse doesn't drag the lander, apply integretion
		if (!bInDrag) {
			float angularChange = obj->integrate();
			shipLight.rotate(angularChange, 0, 1, 0);
		}

		//calculating the ramaining fuel time
		if (obj->thrustUp) {
			obj->fuelTimeLeft = obj->fuelTimeLeft - startThrust;
		}
	}

	//update the position of the particle emitters
	ofVec3f ePos = obj->lander.getPosition();
	thrustEmitter.position = ofVec3f(ePos.x, ePos.y + 2, ePos.z);
	explodeEmitter.position = ofVec3f(ePos.x, ePos.y + 1.5, ePos.z);
	thrustEmitter.update();
	explodeEmitter.update();

	//go to the game end screen if the lander crash or lander on the ground and fuel is out, or lander landed in landing areas
	 if (bCrash || (bCollide && bFuelOut) || bLanding) { 
		bStartGame = false;
		bEndScreen = true;
	}
}
//--------------------------------------------------------------
void ofApp::draw() {
	//if player in game or end game screen
	if (bStartGame || bEndScreen) {
		thrustLoadVbo();
		explodeLoadVbo();
		ofBackground(ofColor::black);

		glDepthMask(false);
		ofSetColor(ofColor::white);
		backgroundImage.draw(0, 0, ofGetScreenWidth(), ofGetScreenHeight()); //draw 2D background image
		glDepthMask(true);

		//select different camera based on the key pressed
		if (bTrackCam) 
			trackCam.begin();
		else if (bBotCam)
			botCam.begin();
		else if (bFrontCam)
			frontCam.begin();
		else
			cam.begin();
		ofPushMatrix();
		if (bWireframe) {                    // wireframe mode  
			ofDisableLighting();
			ofSetColor(ofColor::slateGray);
			mars.drawWireframe();
			if (bLanderLoaded) {
				obj->lander.drawWireframe();
			}
		}
		else {
			ofEnableLighting();              // shaded mode
			mars.drawFaces();
			ofMesh mesh;
			if (bLanderLoaded) {
				obj->lander.drawFaces();
				if (bLanderSelected) {

					ofVec3f min = obj->lander.getSceneMin() + obj->lander.getPosition();
					ofVec3f max = obj->lander.getSceneMax() + obj->lander.getPosition();

					Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
					ofNoFill();
					ofSetColor(ofColor::white);
					Octree::drawBox(bounds);
				}
			}
		}

		if (bDisplayPoints) {                // display points as an option    
			glPointSize(3);
			ofSetColor(ofColor::green);
			mars.drawVertices();
		}

		// recursively draw octree
		//
		ofDisableLighting();
		int level = 0;

		if (bDisplayOctree) {
			ofNoFill();
			ofSetColor(ofColor::white);
			octree.draw(numLevels, 0);
		}

		ofPopMatrix();
		if (bTrackCam) //end the camare
			trackCam.end();
		else if (bBotCam)
			botCam.end();
		else if (bFrontCam)
			frontCam.end();
		else
			cam.end();

		glDepthMask(GL_FALSE);

		ofSetColor(255, 100, 90);

		// this makes everything look glowy :)
		//
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnablePointSprites();

		// begin drawing in the camera
		//
		shader.begin();
		if (bTrackCam)
			trackCam.begin();
		else if (bBotCam)
			botCam.begin();
		else if (bFrontCam)
			frontCam.begin();
		else
			cam.begin();

		// draw particle emitter here..
		//
		particleTex.bind();
		vbo.draw(GL_POINTS, 0, (int)thrustEmitter.sys->particles.size());
		vbo.draw(GL_POINTS, 0, (int)explodeEmitter.sys->particles.size());
		particleTex.unbind();

		//  end drawing in the camera
		// 
		if (bTrackCam)
			trackCam.end();
		else if (bBotCam)
			botCam.end();
		else if (bFrontCam)
			frontCam.end();
		else
			cam.end();
		shader.end();

		ofDisablePointSprites();
		ofDisableBlendMode();
		ofEnableAlphaBlending();

		// set back the depth mask
		//
		glDepthMask(GL_TRUE);

		//display the remaining fuel time, altitude and framerate to the screen
		string str, altitudeStr;
		str = "Remaining Fuel Time: " + std::to_string(obj->fuelTimeLeft) + "      Framerate: " + std::to_string(ofGetFrameRate());
		altitudeStr = "Altitude: " + std::to_string(altitude);
		ofSetColor(ofColor::white);
		if (bShowAltitude)
			ofDrawBitmapString(altitudeStr, ofGetWindowWidth() / 2 - 100, 15);
		ofDrawBitmapString(str, ofGetWindowWidth() - 500, 15);

		if (bEndScreen) {
			endGameMsg(); //display end game message on the screen
		}
	}
	//menu
	else if (!bStartGame) {
		startMenu(); //display the starting menu to the screen
	}

}


/*
* set up all the variable for the thurst emiiter
*/
void ofApp::setThurstEmitter() {
	// Create Forces
	//
	turbForce = new TurbulenceForce(ofVec3f(-10, 0, -10), ofVec3f(10, 0, 10));
	gravityForce = new GravityForce(ofVec3f(0, -0.5, 0));
	radialForce = new ImpulseRadialForce(10);

	// set up the emitter
	// 
	thrustEmitter.sys->addForce(turbForce);
	thrustEmitter.sys->addForce(gravityForce);
	thrustEmitter.sys->addForce(radialForce);

	thrustEmitter.setVelocity(ofVec3f(0, -5, 0));
	thrustEmitter.setOneShot(true);
	thrustEmitter.setEmitterType(DirectionalEmitter);
	thrustEmitter.setGroupSize(100);
	thrustEmitter.setRandomLife(true);
	thrustEmitter.setLifespanRange(ofVec2f(0.5, 0.7));
}

/*
* set up all the variable for the thurst emiiter
*/
void ofApp::setExplodeEmitter() {
	// Create Forces
	//
	explodeTurbForce = new TurbulenceForce(ofVec3f(-3,0,-3), ofVec3f(3,3,3));
	explodeGravityForce = new GravityForce(ofVec3f(0, -20, 0));
	explodeRadialForce = new ImpulseRadialForce(150);

	// set up the emitter
	// 
	explodeEmitter.sys->addForce(explodeTurbForce);
	explodeEmitter.sys->addForce(explodeGravityForce);
	explodeEmitter.sys->addForce(explodeRadialForce);

	explodeEmitter.setVelocity(ofVec3f(0, 10, 0));
	explodeEmitter.setOneShot(true);
	explodeEmitter.setEmitterType(RadialEmitter);
	explodeEmitter.setGroupSize(900);
	explodeEmitter.setRandomLife(true);
	explodeEmitter.setLifespanRange(ofVec2f(1, 2));
}

/*
* using ray detection to the the altitude of the lander
*/
void ofApp::rayAltitudeSensor() {
	glm::vec3 p = obj->lander.getPosition();
	ofVec3f rayPoint = p; //lander position as the ray point
	ofVec3f rayDir = ofVec3f(0, -1, 0); //downward direction as the ray direction
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z), Vector3(rayDir.x, rayDir.y, rayDir.z)); //creat a ray

	bAltitude = octree.intersect(ray, octree.root, altitudeNode); //using the ray-intersect method of the octree
}

/*
* check if the lander collide with the terrain using box-box intersect method of the octree
*/
void ofApp::checkCollide() {
	ofVec3f min = obj->lander.getSceneMin() + obj->lander.getPosition();
	ofVec3f max = obj->lander.getSceneMax() + obj->lander.getPosition();

	Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

	colBoxList.clear();
	bCollide = octree.intersect(bounds, octree.root, colBoxList);
}

/*
* react to the collidesion between lander and the terrain
*/
void ofApp::applyCollide() {
	if (!bCollide) {
		clipped = false;
	}
	if (bCollide) {

		if (!clipped) {

			//check if the velocity during the collision is too large
			if (abs(obj->velocity.x) > 1.8 || abs(obj->velocity.y) > 1.8 || abs(obj->velocity.z) > 1.8) {
				explodeEmitter.sys->reset();
				explodeEmitter.start();
				explodeSound.play();
				bCrash = true;
			}

			//play collide sound;
			collideSound.play();

			//reverse velocity (bounce)
			obj->velocity = -0.5 * obj->velocity;
			obj->acceleration = -6 * obj->acceleration;

			//anti clipping
			if (obj->velocity.x <= 0) {
				obj->velocity.x = 0.1;
			}
			
			checkLanding(); //check if the lander land in one of the lander areas
		}
		clipped = true; //for lander to not stuck in the terrain
	}
}

/*
* check if lander lands in any of the landing areas
*/
void ofApp:: checkLanding() {
	for (int i = 0; i < 3; i++)
	{
		float deltaX = abs(obj->lander.getPosition().x - landingAreas[i].x);
		float deltaZ = abs(obj->lander.getPosition().z - landingAreas[i].z);
		float distance = sqrt(deltaX * deltaX + deltaZ * deltaZ); //using the distance formula to check the distance between lander and the lander areas
		if (distance <= landingAreaRadius) {
			//if lander landed successfully, check the score that player got
			if (distance <= landingAreaRadius - landerHalfLength) { //if the lander landed perfectly in the landing areas
				if (i == 0)
					finalScore = score1;
				else if (i == 1)
					finalScore = score2;
				else
					finalScore = score3;
			}
			else {  //if the lander landed slightly off the landing areas, player get half of the score of the current landing area
				if (i == 0)
					finalScore = score1/2;
				else if (i == 1)
					finalScore = score2/2;
				else
					finalScore = score3/2;
			}
			bLanding = true;
		}
	}
}

void ofApp::keyPressed(int key) {
	keymap[key] = true;
	if (bStartGame) {
		if (keymap['1']) { //general interactive cam
			bGeneralCam = true;
			bTrackCam = false;
			bBotCam = false;
			bFrontCam = false;
		}
		if (keymap['2']) { //tracking cam for the lander
			bGeneralCam = false;
			bTrackCam = true;
			bBotCam = false;
			bFrontCam = false;
		}
		if (keymap['3']) { //bottom view cam from the lander
			bGeneralCam = false;
			bTrackCam = false;
			bBotCam = true;
			bFrontCam = false;
		}
		if (keymap['4']) { //front view cam from the lander
			bGeneralCam = false;
			bTrackCam = false;
			bBotCam = false;
			bFrontCam = true;
		}
		if (keymap['5']) { //enable light1
			if (light1.getIsEnabled())
				light1.disable();
			else
				light1.enable();
		}
		if (keymap['6']) {//enable light2
			if (light2.getIsEnabled())
				light2.disable();
			else
				light2.enable();
		}
		if (keymap['7']) {//enable light3, light to light the lander
			if (light3.getIsEnabled())
				light3.disable();
			else
				light3.enable();
		}
		if (keymap['C'] || keymap['c']) {//enable or disable mouse input for the cam1
			if (cam.getMouseInputEnabled()) cam.disableMouseInput();
			else cam.enableMouseInput();
		}
		if (keymap['A'] || keymap['a']) {//show or hide the altitude sensor
			bShowAltitude = !bShowAltitude;
		}
		if (keymap['F'] || keymap['f']) {
			ofToggleFullscreen();
		}
		if (keymap['L'] || keymap['l']) {//enable or disable the front light of the lander
			bToggleShipLight = !bToggleShipLight;
		}
		if (keymap['O'] || keymap['o']) {
			bDisplayOctree = !bDisplayOctree;
		}
		if (keymap['r']) {
			cam.reset();
		}
		if (keymap['s']) {
			savePicture();
		}
		if (keymap['t']) { //retarget cam1 behind the lander
			setCameraTarget();
		}
		if (keymap['v']) {
			togglePointsDisplay();
		}
		if (keymap['w']) {
			toggleWireframeMode();
		}
		if (keymap[OF_KEY_ALT]) {
			cam.enableMouseInput();
			bAltKeyDown = true;
		}
		if (keymap[OF_KEY_CONTROL]) {
			bCtrlKeyDown = true;
		}
	}
	else if (keymap['P'] || keymap['p']) { //reset the game and go back the starting menu
		reset();
	}
	else { //in the starting menu
		gameStart();
	}

}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

void ofApp::keyReleased(int key) {
	if (keymap[OF_KEY_ALT]) {
		cam.disableMouseInput();
		bAltKeyDown = false;
	}
	if (keymap[OF_KEY_CONTROL]) {
		bCtrlKeyDown = false;
	}

	if (keymap[' ']) {
		obj->thrustUp = false;
		thrustEmitter.stop();
		thrustSound.stop();
	}
	if (keymap['d'] || keymap['D']) {
		obj->moveDown = false;
	}
	if (keymap[OF_KEY_UP]) {
		obj->moveForward = false;
	}
	if (keymap[OF_KEY_DOWN]) {
		obj->moveBackward = false;
	}
	if (keymap[OF_KEY_LEFT]) {
		obj->moveLeft = false;
	}
	if (keymap[OF_KEY_RIGHT]) {
		obj->moveRight = false;
	}
	if (keymap['z'] || keymap['Z']) {
		obj->rotateLeft = false;
	}
	if (keymap['x'] || keymap['X']) {
		obj->rotateRight = false;
	}
	keymap[key] = false;
}



//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	
}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	// if moving camera, don't allow mouse interaction
//
	if (cam.getMouseInputEnabled()) return;

	// if rover is loaded, test for selection
	//
	if (bLanderLoaded) {
		glm::vec3 origin = cam.getPosition();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

		ofVec3f min = obj->lander.getSceneMin() + obj->lander.getPosition();
		ofVec3f max = obj->lander.getSceneMax() + obj->lander.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
		if (hit) {
			bLanderSelected = true;
			mouseDownPos = getMousePointOnPlane(obj->lander.getPosition(), cam.getZAxis());
			mouseLastPos = mouseDownPos;
			bInDrag = true;
		}
		else {
			bLanderSelected = false;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	if (bInDrag) {

		glm::vec3 landerPos = obj->lander.getPosition();
		glm::vec3 mousePos = getMousePointOnPlane(landerPos, cam.getZAxis());
		glm::vec3 delta = mousePos - mouseLastPos;
	
		landerPos += delta;
		obj->lander.setPosition(landerPos.x, landerPos.y, landerPos.z);
		mouseLastPos = mousePos;

		ofVec3f min = obj->lander.getSceneMin() + obj->lander.getPosition();
		ofVec3f max = obj->lander.getSceneMax() + obj->lander.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

		colBoxList.clear();
		octree.intersect(bounds, octree.root, colBoxList);
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bInDrag = false;
}



// Set the camera to use the selected point as it's new target
//  
void ofApp::setCameraTarget() {
	ofVec3f position = obj->lander.getPosition();
	float angle = obj->lander.getRotationAngle(0);

	cam.setPosition(position.x + 17 * sin(ofDegToRad(angle)), position.y + 4, position.z + 17 * cos(ofDegToRad(angle)));
	cam.setOrientation(ofVec3f(-15, angle, 0));
}


//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
// setup basic ambient lighting in GL  (for now, enable just 1 light)
//
void ofApp::initLightingAndMaterials() {
	// setup 3 lights
	shipLight.setup();
	shipLight.enable();
	shipLight.setSpotlight();
	shipLight.setScale(.05);
	shipLight.setSpotlightCutOff(25);
	shipLight.setAttenuation(2, .002, .002);
	shipLight.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	shipLight.setDiffuseColor(ofFloatColor(1, 1, 1));
	shipLight.setSpecularColor(ofFloatColor(1, 1, 1));
	shipLight.rotate(0, ofVec3f(0, 1, 0));
	shipLight.setPosition(-5, 5, 5);

	//ambient light
	light1.setup();
	light1.enable();
	light1.setAreaLight(1, 1);
	light1.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	light1.setDiffuseColor(ofFloatColor(1, 1, 1));
	light1.setSpecularColor(ofFloatColor(1, 1, 1));
	light1.rotate(45, ofVec3f(0, 1, 0));
	light1.rotate(-45, ofVec3f(1, 0, 0));
	light1.setPosition(15, 90, -195);

	// cloud light
	light2.setup();
	light2.enable();
	light2.setSpotlight();
	light2.setScale(.45);
	light2.setSpotlightCutOff(60);
	light2.setAttenuation(29, .001, .001);
	light2.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	light2.setDiffuseColor(ofFloatColor(1, 2, 2));
	light2.setSpecularColor(ofFloatColor(1, 1, 1));
	light2.rotate(270, ofVec3f(1, 0, 0));
	light2.setPosition(10, 60, 35);

	// light up ship
	light3.setup();
	light3.enable();
	light3.setSpotlight();
	light3.setScale(.05);
	light3.setSpotlightCutOff(25);
	light3.setAttenuation(1, .002, .002);
	light3.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	light3.setDiffuseColor(ofFloatColor(1, 1, 1));
	light3.setSpecularColor(ofFloatColor(1, 1, 1));
	light3.rotate(90, ofVec3f(1, 0, 0));
	light3.setPosition(-5, 5, 5);

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
} 

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

void ofApp::dragEvent2(ofDragInfo dragInfo) {
}

bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point) {
	ofVec2f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
}

//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
	// Setup our rays
	//
	glm::vec3 origin = cam.getPosition();
	glm::vec3 camAxis = cam.getZAxis();
	glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		// We use the parameteric line or vector representation of a line to compute
		//
		// p' = p + s * dir;
		//
		glm::vec3 intersectPoint = origin + distance * mouseDir;

		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}

// GAME STATES
//--------------------------------------------------------------
//reset all the variables for the new game
void ofApp::reset() {
	bEndScreen = false;
	bStartGame = false;
	obj->setLanderPosition(37, 30, 57);
	obj->setLanderRotation(0);
	obj->velocity = ofVec3f(0, 0, 0);
	obj->acceleration = ofVec3f(0, 0, 0);
	obj->angularVelocity = 0;
	obj->angularAcceleration = 0;
	obj->fuelTimeLeft = 120;

	botCam.setPosition(ofVec3f(obj->lander.getPosition()));
	botCam.setOrientation(ofVec3f(-90, 0, 0));
	bGeneralCam = true;
	bTrackCam = false;
	bBotCam = false;
	bFrontCam = false;
	setCameraTarget();
	
	bCollide = false;
	bToggleShipLight = false;
	clipped = false;
	bCrash = false;
	bShowAltitude = true;
	bLanding = false;
	bFuelOut = false;
}

void ofApp::gameStart() {
	bStartGame = true;
}

/*
* A function to display messages after the game end
*/
void ofApp::endGameMsg() {
	string endStr, endStr2, endStr3;

	//display end game message based the end condition
	if (bCrash) {
		finalScore = 0;
		endStr = "Game Over...";
		endStr2 = "The AstroBoy has Crashed";
		endStr3 = "Your Score: " + std::to_string(finalScore);
	}
	else if (bCollide && bFuelOut && !bLanding) {
		finalScore = 0;
		endStr = "Game Over...";
		endStr2 = "The AstroBoy has Run Out of Fuel";
		endStr3 = "Your Score: " + std::to_string(finalScore);
	}
	else if (bLanding) {
		endStr = "CONGRATULATIONS!";
		endStr2 = "You have Successfully Landed!";
		endStr3 = "Your Score: " + std::to_string(finalScore);
	}
	string endStr4 = "Press the 'p' key to return to start screen.";
	ofSetColor(ofColor::white);
	ofDrawBitmapString(endStr, ofGetWindowWidth() / 2 - 100, ofGetWindowHeight() / 2);
	ofDrawBitmapString(endStr2, ofGetWindowWidth() / 2 - 100, ofGetWindowHeight() / 2 + 30);
	ofDrawBitmapString(endStr3, ofGetWindowWidth() / 2 - 100, ofGetWindowHeight() / 2 + 60);
	ofDrawBitmapString(endStr4, ofGetWindowWidth() / 2 - 100, ofGetWindowHeight() / 2 + 90);
}

/*
* A function to display the starting menu
*/
void ofApp::startMenu() {
	glDepthMask(false);
	ofSetColor(ofColor::white);
	backgroundImage.draw(0, 0, ofGetScreenWidth(), ofGetScreenHeight());
	glDepthMask(true);

	string title = "Astroboy Lander";
	string start = "Press any key to start game";
	string instructions = "Land the spacecraft gently in any of the landing pads before your fuel runs out!";
	string instructions2 = "Accurate landing and landing in the mountain pads rewards more points.";

	string controls = "Controls: ";
	string c1 = "D - Descend";
	string c2 = "Space - Thrust";
	string c3 = "Arrow Keys - Forward, Backward, Left, Right Movement";
	string c4 = "Z - Rotate Left";
	string c5 = "X - Rotate Right";

	string hotkeys = "Hotkeys: ";
	string hotkey1 = "1,2,3,4 - Camera POV";
	string hotkey2 = "5,6,7 - Light Toggles";
	string hotkey3 = "T - Reset Freecam";
	string hotkey4 = "C - Toggle Freecam Interaction";
	string hotkey5 = "A - Toggle Altitude";
	string hotkey6 = "L - Toggle Spacecraft Light";

	ofSetColor(ofColor::white);
	ofDrawBitmapString(title, ofGetWindowWidth() - 820, 140);
	ofDrawBitmapString(start, ofGetWindowWidth() - 820, 180);
	ofDrawBitmapString(instructions, ofGetWindowWidth() - 920, 220);
	ofDrawBitmapString(instructions2, ofGetWindowWidth() - 920, 235);
	ofDrawBitmapString(controls, ofGetWindowWidth() - 820, 300);
	ofDrawBitmapString(c1, ofGetWindowWidth() - 820, 320);
	ofDrawBitmapString(c2, ofGetWindowWidth() - 820, 340);
	ofDrawBitmapString(c3, ofGetWindowWidth() - 820, 360);
	ofDrawBitmapString(c4, ofGetWindowWidth() - 820, 380);
	ofDrawBitmapString(c5, ofGetWindowWidth() - 820, 400);
	ofDrawBitmapString(hotkeys, ofGetWindowWidth() - 820, 440);
	ofDrawBitmapString(hotkey1, ofGetWindowWidth() - 820, 460);
	ofDrawBitmapString(hotkey2, ofGetWindowWidth() - 820, 480);
	ofDrawBitmapString(hotkey3, ofGetWindowWidth() - 820, 500);
	ofDrawBitmapString(hotkey4, ofGetWindowWidth() - 820, 520);
	ofDrawBitmapString(hotkey5, ofGetWindowWidth() - 820, 540);
	ofDrawBitmapString(hotkey6, ofGetWindowWidth() - 820, 560);
}
