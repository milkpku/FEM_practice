#include <stdio.h>
#include <ctime>

#include <string>
#include <sstream>
#include <iomanip>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "controls.h"
#include "arcball.h"

#include "Types.h"
#include "Dynamic.h"

extern int shadFlag;

namespace Control{

/* arcball object */
ArcBall arcball;

/* Engine */
BallonFEM::Engine engine;
BallonFEM::TetraMesh* m_tetra;

static double force = 0;
static int modified = 0;
static int outputcount = 0;		// output id

void mAddParameter()
{
	force += 5e-5;
	modified = 0;
	printf("force = %.4f\n", force);
}

/* Do something to tetra mesh */
void mProcess()
{
	std::vector<double> solveTime;
	mOutput();
	for (size_t i = 0; i < 20; i++)
	{
		mAddParameter();
		if (modified == 0){
			/* add force to tetra mesh */
			//m_tetra->vertices[9].m_f_ext = BallonFEM::Vec3(0, force, force);
			engine.setAirModel(new BallonFEM::AirModel_Isobaric(force, 0));
			engine.inputData();
			modified = 1;
		}

		/* compute deformation */

		std::clock_t start;
		start = std::clock();
		engine.solveStaticPosMat();
		solveTime.push_back((std::clock() - start) / (double) CLOCKS_PER_SEC);

		engine.stepToNext();
		engine.outputData();
		mOutput();
		shadFlag = 1;
	}

	double count = 0;
	for (size_t i = 0; i < solveTime.size(); i++)
	{
		printf("solve time for step %d: %f \n", i, solveTime[i]);
		count += solveTime[i];
	}
	printf("total time spent %f", count);
}

/* Reset the tetra mesh to original state */
void mReset()
{
	for (size_t i = 0; i < m_tetra->vertices.size(); i++)
	{
		BallonFEM::Vertex &v = m_tetra->vertices[i];
		v.m_pos = v.m_cord;
		v.m_f_ext = BallonFEM::Vec3(0);
	}
	force = 0;
	modified = 0;
	shadFlag = 1;
	outputcount = 0;
}

void mOutput()
{
	std::ostringstream info;
	info << "#";
	info << "pressure = ";
	info << force;

	std::ostringstream ss;
	ss << "output/p_";
	ss << std::setw(7) << std::setfill('0') << outputcount;
	ss << ".obj";
	std::string name(ss.str());

    printf("Write to %s. \n", name.c_str());

    m_tetra->write(name, info.str());
	outputcount++;
}

/* rotation quaternion and translation vector for the object */
glm::dquat  ObjRot(1, 0, 0, 0);
glm::vec3   camera(0, 0, 2);

/* inner variables */
int win_width, win_height;
double startx, starty;
int gButton; 
int gState; 

/* inner mantained */
glm::dmat4 Model;
glm::mat4 View;
glm::mat4 Projection;

/* controler initialize */
void control_init(GLFWwindow* window, BallonFEM::TetraMesh* tetra)
{
    glfwGetWindowSize(window, &win_width, &win_height);
    m_tetra = tetra;
    engine = BallonFEM::Engine(
		tetra, 
		new BallonFEM::Elastic_neohookean(0.4, 0.4), 
		new BallonFEM::AirModel_Isobaric(force, 0)
		);
}

/* update at each main loop */
void computeMatrixFromInputs()
{
    Model = glm::toMat4(ObjRot);
    View = glm::lookAt(
                        camera,                             /* Camera location */
                        glm::vec3(camera.x, camera.y, 0), /* and looks at this point */
                        glm::vec3(0, 1, 0)                    /* Head is up */
            );
    Projection = glm::perspective(45.0f, win_width/(float) win_height, 0.1f, 500.0f);
}

glm::vec3 getCamera(){
    return camera;
}

glm::mat4 getMVP(){
	return Projection * View * glm::mat4(Model);
}

glm::mat4 getView(){
    return View;
}

glm::mat4 getModel(){
    return glm::mat4(Model);
}

glm::mat4 getProjection(){
    return Projection;
}


/*! mouse click call back function */
void  mouseClick(GLFWwindow* window, int button, int action, int mods) {
	/* set up an arcball around the Eye's center
	switch y coordinates to right handed system  */
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    gState = action;
    
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		gButton = GLFW_MOUSE_BUTTON_LEFT;
		arcball.reset(win_width, win_height, xpos - win_width / 2, win_height - ypos - win_height / 2);
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		startx = xpos;
		starty = ypos;
		gButton = GLFW_MOUSE_BUTTON_RIGHT;
	}
	return;
}

/*! mouse motion call back function */
void mouseMove(GLFWwindow* window, double xpos, double ypos)
{
    glm::vec3   trans;
    glm::dquat  rot;

    if (gState!=GLFW_PRESS)
    {
        return;
    }

	/* rotation, call arcball */
	if (gButton == GLFW_MOUSE_BUTTON_LEFT)
	{
		rot = arcball.update(xpos - win_width / 2, win_height - ypos - win_height / 2);
		ObjRot = rot * ObjRot;
	}

	/*xy translation */
	if (gButton == GLFW_MOUSE_BUTTON_RIGHT)
	{
		double scale = abs( camera.z / win_height );
		trans = glm::vec3(scale*(xpos - startx), scale*(starty - ypos), 0);
		startx = xpos;
		starty = ypos;
		camera = camera - trans;
	}

}

/* mouse middle scroll call back */
void mouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	double scale = abs(40. * camera.z / win_height);
	glm::vec3  trans = glm::vec3(0, 0, - scale * yoffset);
	camera = camera + trans;
}

/*! helper function to remind the user about commands, hot keys */
void help()
{
	printf("w  -  Wireframe Display\n");
	printf("f  -  Flat Shading \n");
    printf("r  -  Reset All Parames \n");
    printf("a  -  Add Force \n");
    printf("space - Solve Static Position \n");
	printf("o  -  Output obj file\n");
	printf("?  -  Help Information\n");
	printf("q - quit\n");
}

/*! Keyboard call back function */
void keyBoard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS){
        return;
    }

	switch (key)
	{
	case GLFW_KEY_F:
		//Flat Shading
		glPolygonMode(GL_FRONT, GL_FILL);
		break;
	case GLFW_KEY_W:
		//Wireframe mode
		glPolygonMode(GL_FRONT, GL_LINE);
		break;
    case GLFW_KEY_SPACE:
        mProcess();
        break;
	case GLFW_KEY_A:
		mAddParameter();
		break;
	case GLFW_KEY_R:
		mReset();
		break;
    case GLFW_KEY_O:
        mOutput();
		break;
	case GLFW_KEY_H:
	case GLFW_KEY_UNKNOWN:
		help();
		break;
	case GLFW_KEY_Q:
	case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
		break;
	}
}

/*! Called when a "resize" event is received by the window. */
void reshape(GLFWwindow* window, int w, int h)
{
	win_width = w;
	win_height = h;

    /* re project */
    glViewport(0, 0, win_width, win_height);
}

}
