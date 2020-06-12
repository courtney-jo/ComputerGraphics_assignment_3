// This example is heavily based on the tutorial at https://open.gl

// OpenGL Helpers to reduce the clutter
#include "Helpers.h"

// GLFW is necessary to handle the OpenGL context
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Linear Algebra Library
#include <Eigen/Core>
#include <Eigen/Dense>

// Timer
#include <chrono>
#include <stdlib.h>     /* srand, rand */

//file access library
#include <fstream>

// VertexBufferObject wrapper
VertexBufferObject VBO;
VertexBufferObject VBO_C;

Eigen::MatrixXf V(3, 36);		// Contains vertex positions (all objects fit within a cube)
Eigen::MatrixXf V_tmp(3, 0);	//tmp
Eigen::MatrixXf C(3, 36);		// Contains RGB values
Eigen::MatrixXf C_tmp(3, 0);	//tmp
Eigen::Matrix4f view(4, 4);		// View matrix
Eigen::Matrix4f P(4, 4);		// Projection matrix

Eigen::MatrixXf offTmp(3, 0);	// offFile vertex tmp
int V_Size = 0;
int closestVIndex = 0;
double center_x = 0.0;
double center_y = 0.0;

//Old Code
/*
void rotate(double a) {
	double x1 = V(0, closestVIndex);
	double x2 = V(0, closestVIndex + 1);
	double x3 = V(0, closestVIndex + 2);
	double y1 = V(1, closestVIndex);
	double y2 = V(1, closestVIndex + 1);
	double y3 = V(1, closestVIndex + 2);

	V(0, closestVIndex) = center_x + (center_x - x1) * cos(a) + (y1 - center_y) * sin(a);
	V(1, closestVIndex) = center_y + (center_x - x1) * sin(a) - (y1 - center_y) * cos(a);

	V(0, closestVIndex + 1) = center_x + (center_x - x2) * cos(a) + (y2 - center_y) * sin(a);
	V(1, closestVIndex + 1) = center_y + (center_x - x2) * sin(a) - (y2 - center_y) * cos(a);

	V(0, closestVIndex + 2) = center_x + (center_x - x3) * cos(a) + (y3 - center_y) * sin(a);
	V(1, closestVIndex + 2) = center_y + (center_x - x3) * sin(a) - (y3 - center_y) * cos(a);

	VBO.update(V);
}

void scale(double s) {
	double x1 = V(0, closestVIndex);
	double x2 = V(0, closestVIndex + 1);
	double x3 = V(0, closestVIndex + 2);
	double y1 = V(1, closestVIndex);
	double y2 = V(1, closestVIndex + 1);
	double y3 = V(1, closestVIndex + 2);

	V(0, closestVIndex) = (x1 - center_x)*s + center_x;
	V(1, closestVIndex) = (y1 - center_y)*s + center_y;

	V(0, closestVIndex + 1) = (x2 - center_x)*s + center_x;
	V(1, closestVIndex + 1) = (y2 - center_y)*s + center_y;

	V(0, closestVIndex + 2) = (x3 - center_x)*s + center_x;
	V(1, closestVIndex + 2) = (y3 - center_y)*s + center_y;

	VBO.update(V);
}

//counter_clockwise is positive angle	glRotatef()		glScalef()	***NEEDS WORK***
void rotate_scale(GLFWwindow* window, int key, int scancode, int action, int mods) {
	switch (key) {
		//rotation & scale	h=clockwise	j=counter-clockwise	k=scale up	l=scale down
	case  GLFW_KEY_H:	rotate((-10 * 180) / 3.14);		break;
	case  GLFW_KEY_J:	rotate((10 * 180) / 3.14);		break;
	case  GLFW_KEY_K:	scale(.75);		break;
	case  GLFW_KEY_L:	scale(1.25);		break;
	default:	glfwSetKeyCallback(window, key_callback); break;
	}
}*/

//function to find closest vertex from last mouse click
int closestVertex(double x, double y) {
	int index = 0;
	double delta = sqrt((abs(V(0, 0) - x)*abs(V(0, 0) - x)) + (abs(V(1, 0) - y)*abs(V(1, 0) - y)));

	for (int i = 0; i < V_Size; ++i) {
		double tmpDelta = sqrt((abs(V(0, i) - x)*abs(V(0, i) - x)) + (abs(V(1, i) - y)*abs(V(1, i) - y)));

		if (tmpDelta < delta) {
			delta = tmpDelta;
			index = i;
		}
	}

	return index;
}

//function to calculate center of triangle
void triangleCenter(double& center_x, double& center_y, int loc) {
	center_x = (V(0, loc) + V(0, loc + 1) + V(0, loc + 2)) / 3;
	center_y = (V(1, loc) + V(1, loc + 1) + V(1, loc + 2)) / 3;
}

//Finds area of triangle using coordinates
double triArea(double x1, double y1, double x2, double y2, double x3, double y3) {
	double a = x1 * (y2 - y3);
	double b = x2 * (y3 - y1);
	double c = x3 * (y1 - y2);

	return abs((a + b + c) / 2.0);
}

//returns true if point is within a triangle
bool selectedTri(double x, double y, int& index) {
	bool found = false;
	for (int i = 0; i < V_Size - 2; i = i + 3) {
		double a = triArea(V(0, i), V(1, i), V(0, i + 1), V(1, i + 1), V(0, i + 2), V(1, i + 2));
		double a1 = triArea(x, y, V(0, i), V(1, i), V(0, i + 1), V(1, i + 1));
		double a2 = triArea(x, y, V(0, i + 1), V(1, i + 1), V(0, i + 2), V(1, i + 2));
		double a3 = triArea(x, y, V(0, i), V(1, i), V(0, i + 2), V(1, i + 2));
		if (a == (a1 + a2 + a3)) {
			found = true;
			index = i;
			break;
		}
	}
	return found;
}

void cursorCall(GLFWwindow* window, double xpos, double ypos) {
	// Get the size of the window
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// Convert screen position to world coordinates
	Eigen::Vector4f p_screen(xpos, height - 1 - ypos, 0, 1);
	Eigen::Vector4f p_canonical((p_screen[0] / width) * 2 - 1, (p_screen[1] / height) * 2 - 1, 0, 1);
	Eigen::Vector4f p_world = view.inverse()*p_canonical;
	double xmove = p_world[0];
	double ymove = p_world[1]; // NOTE: y axis is flipped in glfw

	//move object with cursor
	V(0, closestVIndex) += (xmove - center_x);			V(1, closestVIndex) += (ymove - center_y);
	V(0, closestVIndex + 1) += (xmove - center_x);		V(1, closestVIndex + 1) += (ymove - center_y);
	V(0, closestVIndex + 2) += (xmove - center_x);		V(1, closestVIndex + 2) += (ymove - center_y);

	center_x = xmove;
	center_y = ymove;

	VBO.update(V);
}

//New Code
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the position of the mouse in the window
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Get the size of the window
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// Convert screen position to world coordinates
	Eigen::Vector4f p_screen(xpos, height - 1 - ypos, 0, 1);
	Eigen::Vector4f p_canonical((p_screen[0] / width) * 2 - 1, (p_screen[1] / height) * 2 - 1, 0, 1);
	Eigen::Vector4f p_world = view.inverse()*p_canonical;
	double xworld = p_world[0];
	double yworld = p_world[1]; // NOTE: y axis is flipped in glfw

	// Update the position of selected triangle
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (selectedTri(xworld, yworld, closestVIndex)) {
			center_x = xworld;
			center_y = yworld;
			glfwSetCursorPosCallback(window, cursorCall);
		}
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		glfwSetCursorPosCallback(window, 0);
	}

	// Upload the change to the GPU
	VBO.update(V);
}

//1.1 square at origin
void insertCube() {
	//Vertex
	V_tmp.resize(3, V_Size);	C_tmp.resize(3, V_Size);
	for (int i = 0; i < V_Size; ++i) {
		V_tmp.col(i) = V.col(i);
		C_tmp.col(i) = C.col(i);
	}
	V_Size += 36;	V.resize(3, V_Size); C.resize(3, V_Size);
	for (int i = 0; i < V_Size - 36; ++i) {
		V.col(i) = V_tmp.col(i);
		C.col(i) = C_tmp.col(i);
	}
	V_tmp.resize(3, 0);	C_tmp.resize(3, 0);

	//new square
	V.col(V_Size - 36)	<< -.5, -.5, -.5;
	V.col(V_Size - 35)	<< -.5, -.5, .5;
	V.col(V_Size - 34)	<< -.5, .5, .5;	//1
	V.col(V_Size - 33)	<< .5, .5, -.5;
	V.col(V_Size - 32)	<< -.5, -.5, -.5;
	V.col(V_Size - 31)	<< -.5, .5, -.5;	//2
	V.col(V_Size - 30)	<< .5, -.5, .5;
	V.col(V_Size - 29)	<< -.5, -.5, -.5;
	V.col(V_Size - 28)	<< .5, -.5, -.5;	//3
	V.col(V_Size - 27)	<< .5, .5, -.5;
	V.col(V_Size - 26)	<< .5, -.5, -.5;
	V.col(V_Size - 25)	<< -.5, -.5, -.5;	//4
	V.col(V_Size - 24)	<< -.5, -.5, -.5;
	V.col(V_Size - 23)	<< -.5, .5, .5;
	V.col(V_Size - 22)	<< -.5, .5, -.5;	//5
	V.col(V_Size - 21)	<< .5, -.5, .5;
	V.col(V_Size - 20)	<< -.5, -.5, .5;
	V.col(V_Size - 19)	<< -.5, -.5, -.5;	//6
	V.col(V_Size - 18)	<< -.5, .5, .5;
	V.col(V_Size - 17)	<< -.5, -.5, .5;
	V.col(V_Size - 16)	<< .5, -.5, .5;	//7
	V.col(V_Size - 15)	<< .5, .5, .5;
	V.col(V_Size - 14)	<< .5, -.5, -.5;
	V.col(V_Size - 13)	<< .5, .5, -.5;	//8
	V.col(V_Size - 12)	<< .5, -.5, -.5;
	V.col(V_Size - 11)	<< .5, .5, .5;
	V.col(V_Size - 10)	<< .5, -.5, .5;	//9
	V.col(V_Size - 9)	<< .5, .5, .5;
	V.col(V_Size - 8)	<< .5, .5, -.5;
	V.col(V_Size - 7)	<< -.5, .5, -.5;	//10
	V.col(V_Size - 6)	<< .5, .5, .5;
	V.col(V_Size - 5)	<< -.5, .5, -.5;
	V.col(V_Size - 4)	<< -.5, .5, .5;	//11
	V.col(V_Size - 3)	<< .5, .5, .5;
	V.col(V_Size - 2)	<< -.5, .5, .5;
	V.col(V_Size - 1)	<< .5, -.5, .5;	//12

	//color
	for (int i = V_Size - 36; i < V_Size; ++i) {
		double r = (rand() % 256) / 255.0, g = (rand() % 256) / 255.0, b = (rand() % 256) / 255.0;
		C.col(i) << r, g, b;
	}

	//Update
	VBO.update(V);
	VBO_C.update(C);
}

//1.1 choice 2 - bumby_square; choice 3 - bunny
void offFile(std::string file, double times) {
	std::ifstream myfile (file);
	if (myfile.is_open()) {
		std::string tmp;
		getline(myfile, tmp);
		getline(myfile, tmp);

		//Vertex
		V_tmp.resize(3, V_Size);	C_tmp.resize(3, V_Size);
		for (int i = 0; i < V_Size; ++i) {
			V_tmp.col(i) = V.col(i);
			C_tmp.col(i) = C.col(i);
		}
		V_Size += 3000;	V.resize(3, V_Size); C.resize(3, V_Size);
		for (int i = 0; i < V_Size - 3000; ++i) {
			V.col(i) = V_tmp.col(i);
			C.col(i) = C_tmp.col(i);
		}
		V_tmp.resize(3, 0);	C_tmp.resize(3, 0);
		
		offTmp.resize(3, 502);
		//grab file info
		for (int i = 0; i < 502; ++i) {
			myfile >> offTmp(0, i); offTmp(0, i) *= times;	//x cord of vertex
			myfile >> offTmp(1, i); offTmp(1, i) *= times;	//y cord of vertex
			myfile >> offTmp(2, i); offTmp(2, i) *= times;	//z cord of vertex
		}

		//make triangles
		int v = 0;
		for (int i = V_Size - 3000; i < V_Size-2; i=i+3) {
			myfile >> tmp;
			double r = (rand() % 256) / 255.0, g = (rand() % 256) / 255.0, b = (rand() % 256) / 255.0;
			myfile >> v; V.col(i) = offTmp.col(v); C.col(i) << r, g, b;
			myfile >> v; V.col(i+1) = offTmp.col(v); C.col(i+1) << r, g, b;
			myfile >> v; V.col(i+2) = offTmp.col(v); C.col(i+2) << r, g, b;
		}
		offTmp.resize(3, 0);

		//update
		VBO.update(V);
		VBO_C.update(C);
	}
	myfile.close();
}

//1.3 perspective view
void perspective(GLFWwindow* window) {
	// Get the position of the mouse in the window
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Get the size of the window
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// Convert screen position to world coordinates
	Eigen::Vector4f p_screen(xpos, height - 1 - ypos, 0, 1);
	Eigen::Vector4f p_canonical((p_screen[0] / width) * 2 - 1, (p_screen[1] / height) * 2 - 1, 0, 1);
	Eigen::Vector4f p_world = view.inverse()*p_canonical;

	double m_angle=45, m_aspect=1, m_near=1, m_far=100, m_at_dist=1;

	  //  m_angle  Field of view angle in degrees {45}
	  //  m_aspect  Aspect ratio {1}
	  //  m_near  near clipping plane {1e-2}
	  //  m_far  far clipping plane {100}
	  //  m_at_dist  distance of looking at point {1}

	double yScale = tan(3.14*0.5 - 0.5*m_angle*3.14 / 180.);
	double xScale = yScale / m_aspect;
	P <<
			xScale, 0, 0, 0,
			0, yScale, 0, 0,
			0, 0, -(m_at_dist + m_far + m_near) / ((m_at_dist + m_far) - m_near), -1,
			0, 0, -2.*m_near*(m_at_dist + m_far) / ((m_at_dist + m_far) - m_near), 0;
		P = P.transpose().eval();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixf(P.data());
}

//1.3 orthographic view
void orthographic(GLFWwindow* window) {
	// Get the position of the mouse in the window
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	// Get the size of the window
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// Convert screen position to world coordinates
	Eigen::Vector4f p_screen(xpos, height - 1 - ypos, 0, 1);
	Eigen::Vector4f p_canonical((p_screen[0] / width) * 2 - 1, (p_screen[1] / height) * 2 - 1, 0, 1);
	Eigen::Vector4f p_world = view.inverse()*p_canonical;

	double m_angle = 45, m_aspect = 1, m_near = 1 , m_far = 100, m_at_dist = 1;

	//  m_angle  Field of view angle in degrees {45}
	//  m_aspect  Aspect ratio {1}
	//  m_near  near clipping plane {1e-2}
	//  m_far  far clipping plane {100}
	//  m_at_dist  distance of looking at point {1}
	double left = (p_world[0] * -1) * m_aspect;
	double right = p_world[0] * m_aspect;
	double bottom = -1 * p_world[1];
	double top = p_world[1];
	double tx = (right + left) / (right - left);
	double ty = (top + bottom) / (top - bottom);
	double tz = (m_at_dist + m_far + m_near) / ((m_at_dist + m_far) - m_near);
	double z_fix = 0.5 / m_at_dist / tan(m_angle*0.5 * (3.14 / 180.));
	P <<
		z_fix * 2. / (right - left), 0, 0, -tx,
		0, z_fix*2. / (top - bottom), 0, -ty,
		0, 0, -z_fix * 2. / ((m_at_dist + m_far) - m_near), -tz,
		0, 0, 0, 1;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glLoadMatrixf(P.data());
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Update the position of the first vertex if the keys 1,2, or 3 are pressed
    switch (key)
    {
        case  GLFW_KEY_1:
			insertCube();	break;	//insert square
        case GLFW_KEY_2:
			offFile("C:/Users/cjb14/OneDrive/Documents/Graphics/Assignment_3-courtney-jo-master/data/bumpy_cube.off", .2);	break;	//insert bumpy_square
        case  GLFW_KEY_3:
			offFile("C:/Users/cjb14/OneDrive/Documents/Graphics/Assignment_3-courtney-jo-master/data/bunny.off", 4);	break;	//insert bunny
		case  GLFW_KEY_8:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	break;	//1.2 wire frame
		case GLFW_KEY_9:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	break;	//1.2 flat shading
		case  GLFW_KEY_0:
			break;	//1.2 phong shading
		case GLFW_KEY_E:
			//clear screen
			V.resize(3, 0);	C.resize(3, 0);
			break;
		case GLFW_KEY_A:
			//camera left
			break;
		case GLFW_KEY_D:
			//camera right
			break;
		case GLFW_KEY_W:
			//camera up
			break;
		case GLFW_KEY_S:
			//camera down
			break;
		case GLFW_KEY_O:
			//orthographic camera
			orthographic(window);
			break;
		case GLFW_KEY_P:
			//perspective camera
			perspective(window);
			break;
        default:
            break;
    }

    // Upload the change to the GPU
    VBO.update(V);
}

int main(void)
{
    GLFWwindow* window;

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(1900, 1450, "Assignment 3", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
    printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
    printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Initialize the VAO
    // A Vertex Array Object (or VAO) is an object that describes how the vertex
    // attributes are stored in a Vertex Buffer Object (or VBO). This means that
    // the VAO is not the actual object storing the vertex data,
    // but the descriptor of the vertex data.
    VertexArrayObject VAO;
    VAO.init();
    VAO.bind();

	// Initialize the VBO with the vertices data
	   // A VBO is a data container that lives in the GPU memory
	VBO.init();
	VBO.update(V);

	VBO_C.init();
	VBO_C.update(C);

	// Initialize the OpenGL Program
	// A program controls the OpenGL pipeline and it must contains
	// at least a vertex shader and a fragment shader to be valid
	Program program;
	//indivinual vertex colored
	const GLchar* vertex_shader =
		"#version 150 core\n"
		"in vec2 position;"
		"in vec3 color;"
		"out vec3 Color;"
		"void main()"
		"{"
		"    gl_Position = vec4(position, 0.0, 1.0);"
		"	 Color = color;"
		"}";
	const GLchar* fragment_shader =
		"#version 150 core\n"
		"in vec3 Color;"
		"out vec4 outColor;"
		"void main()"
		"{"
		"    outColor = vec4(Color, 1.0);"
		"}";

	// Compile the two shaders and upload the binary to the GPU
	// Note that we have to explicitly specify that the output "slot" called outColor
	// is the one that we want in the fragment buffer (and thus on screen)
	program.init(vertex_shader, fragment_shader, "outColor");
	program.bind();


	// The vertex shader wants the position of the vertices as an input.
	// The following line connects the VBO we defined above with the position "slot"
	// in the vertex shader
	program.bindVertexAttribArray("position", VBO);
	program.bindVertexAttribArray("color", VBO_C);

    // Save the current time --- it will be used to dynamically change the triangle color
    auto t_start = std::chrono::high_resolution_clock::now();

    // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Register the mouse callback
    //glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Bind your VAO (not necessary if you have only one)
        VAO.bind();

        // Bind your program
        program.bind();

        // Set the uniform value depending on the time difference
        auto t_now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
		srand(time);

		// Enable depth test
		glEnable(GL_DEPTH_TEST);

		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

        // Clear the framebuffer
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw a triangle
        glDrawArrays(GL_TRIANGLES, 0, V_Size);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Deallocate opengl memory
    program.free();
    VAO.free();
    VBO.free();
	VBO_C.free();

    // Deallocate glfw internals
    glfwTerminate();
    return 0;
}