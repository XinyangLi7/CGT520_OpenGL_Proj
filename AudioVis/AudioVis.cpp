#include <windows.h>

//Imgui headers for UI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <random>

#include "DebugCallback.h"
#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files
#include "AudioProcessing.h"

namespace window
{
   const char* const title = USER_NAME " " PROJECT_NAME; //defined in project settings
   int size[2] = {2048, 1024};
   float clear_color[4] = {0.35f, 0.35f, 0.35f, 1.0f};
   bool mousePressed = false;
   double lastX, lastY; // Last mouse position
   float mouseSensitivity = 0.01f; // Sensitivity of mouse movement
   float scrollSensitivity = 0.05f; // Sensitivity of zoom
}

namespace scene
{
   const std::string asset_dir = "assets/";
   const std::string green_mesh_name = "green.obj";
   const std::string red_mesh_name = "red.obj";
   const std::string yellow_mesh_name = "yellow.obj";
   const std::string blue_mesh_name = "blue.obj";
   const std::string land_mesh_name = "land.obj";
   const std::string cube_mesh_name = "cube.obj";

   const std::string tex_name = "AmagoT.bmp";

   const std::string skybox_mesh_name = "cube_2x2x2.obj";
   //const std::string skybox_texture_name = "sorbin.png"; 
   //const std::string skybox_texture_name = "night1.png";
   const std::string skybox_texture_name = "dawn1.png";
   //const std::string skybox_texture_name = "snowland.png";
   //const std::string skybox_texture_name = "sunset1.png";
   //const std::string skybox_texture_name = "sunset2.png";
   
   MeshData skybox_mesh;
   GLuint skybox_tex_id = -1; //Texture map for skybox/cubemap

   GLuint landvao;

   std::string audio_filename = "SpaceWalk.wav";
   //std::string audio_filename = "ImperialMarch60.wav";
   //Test tones
   //std::string audio_filename = "sin_1000Hz.wav";
   //std::string audio_filename = "sweep.wav";

   MeshData greenMesh;
   MeshData redMesh;
   MeshData yellowMesh;
   MeshData blueMesh;
   MeshData landMesh;
   MeshData cubeMesh;
   GLuint tex_id = -1; //Texture map for mesh

   float angle = 0.0f;
   float Cangle = 0.0f;
   float Cdistance = 2.0f;
   float Cheight = 0.5f;
   float scale = 1.0f;
   
   const std::string shader_dir = "shaders/";
   const std::string vertex_shader("audio_vis_vs.glsl");
   const std::string fragment_shader("audio_vis_fs.glsl");

   float ratio = 2.0;
   float last_time_sec = 0.0;
   GLuint shader = -1;

   AudioProcessing audio_processing;
}
//For an explanation of this program's structure see https://www.glfw.org/docs/3.3/quick.html 

namespace Uniforms
{
    glm::mat4 PV;
    glm::mat4 M;
    int pass;
    float time;
    int build_count=0;
    glm::vec3 Tree = glm::vec3(0.05f, 0.25f, 0.1f);
    glm::vec3 Red = glm::vec3(0.9f, 0.15f, 0.15f);
    glm::vec3 Yellow = glm::vec3(0.95f, 0.85f, 0.2f);
    glm::vec3 Blue = glm::vec3(0.2f, 0.35f, 0.9f);

    glm::vec3 eye_w(0.0f, 0.5f, 2.0f); //world-space eye position
};

namespace UniformLocs
{
    int PV;
    int M;
    int pass;
    int time;
    int build_count;
    int Tree, Red, Yellow, Blue;
    int randColor;
    int eye_w;
    int diffuse_tex;
    int skybox_tex;
};

struct Particle {
    glm::vec3 position;  // Current position
    glm::vec3 velocity;  // Velocity (direction and speed)
    float lifetime;      // Time to live
};

const int MAX_PARTICLES = 1000;
Particle particles[MAX_PARTICLES];

void initParticles() {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles[i].position = glm::vec3(
            rand() % 200 / 100.0f - 1.0f,
            rand() % 100 / 100.0f + 0.5f,
            0.0f
        );
        particles[i].velocity = glm::vec3(
            rand() % 10 / 100.0f - 0.09f,  
            -rand() % 50 / 100.0f - 0.1f,  // Y: Slow downward velocity
            0.0f                           
        );
        particles[i].lifetime = 10.0f;       // Full lifetime
    }
}

unsigned int particleVAO, particleVBO;

void setupParticleVAO() {
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particles), particles, GL_DYNAMIC_DRAW);

    // Map 'pos_attrib' to position
    GLint pos_loc = glGetAttribLocation(scene::shader, "pos_attrib");
    glVertexAttribPointer(pos_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
    glEnableVertexAttribArray(pos_loc);

    glBindVertexArray(0);
}

void updateParticles(float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles[i].position += particles[i].velocity * deltaTime;
        particles[i].lifetime -= deltaTime;

        // Reset particle if it "dies"
        if (particles[i].lifetime <= 0.0f || particles[i].position.y < -1.0f) {
            particles[i].position = glm::vec3(
                rand() % 200 / 100.0f - 1.0f,
                rand() % 100 / 100.0f + 0.5f,
                0.0f
            );
            particles[i].velocity = glm::vec3(rand() % 10 / 100.0f - 0.09f, -rand() % 50 / 100.0f - 0.1f, 0.0f);
            particles[i].lifetime = 10.0f;
        }
    }
    // Update the buffer data
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(particles), particles);
}


//Draw the ImGui user interface
//void draw_gui(GLFWwindow* window)
//{
//   //Begin ImGui Frame
//   ImGui_ImplOpenGL3_NewFrame();
//   ImGui_ImplGlfw_NewFrame();
//   ImGui::NewFrame();
//
//   //Draw Gui
//   ImGui::Begin("Debug window");
//    if (ImGui::Button("Quit"))
//    {
//        glfwSetWindowShouldClose(window, GLFW_TRUE);
//    }
//
//    //ImGui::SliderFloat("Rotation angle", &scene::angle, -glm::pi<float>(), +glm::pi<float>());
//    //ImGui::SliderFloat("Scale", &scene::scale, -10.0f, +10.0f);
//
//    if (ImGui::SliderFloat("Camera Rotation angle", &scene::Cangle, -glm::pi<float>(), +glm::pi<float>())) {
//        Uniforms::eye_w = glm::vec3(scene::Cdistance * sin(scene::Cangle), scene::Cheight, scene::Cdistance * cos(scene::Cangle));
//    }
//    if(ImGui::SliderFloat("Camera Distance", &scene::Cdistance, 1, 3)) {
//        Uniforms::eye_w = glm::vec3(scene::Cdistance * sin(scene::Cangle), scene::Cheight, scene::Cdistance * cos(scene::Cangle));
//    }
//    if(ImGui::SliderFloat("Camera Height", &scene::Cheight, 0.2f, 5.0f)) {
//        Uniforms::eye_w = glm::vec3(scene::Cdistance * sin(scene::Cangle), scene::Cheight, scene::Cdistance * cos(scene::Cangle));
//    }
//
//    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//    ImGui::End();
//
//
//
//   static bool show_test = false;
//   if(show_test)
//   {
//      ImGui::ShowDemoWindow(&show_test);
//   }
//
//   //End ImGui Frame
//   ImGui::Render();
//   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}

// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
    //You don't need to clear the color buffer if drawing a fullscreen quad or skybox
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(scene::shader);

    glm::mat4 V = glm::lookAt(Uniforms::eye_w, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 P = glm::perspective(glm::pi<float>()/2.0f, scene::ratio, 0.1f, 100.0f);

    glUniform3fv(UniformLocs::eye_w, 1, glm::value_ptr(Uniforms::eye_w));


    //Pass 0: draw skybox
    glUniform1i(UniformLocs::pass, 0);

    glm::mat4 Vsky = V;
    Vsky[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    glm::mat4 PVsky = P * Vsky;
    glUniformMatrix4fv(UniformLocs::PV, 1, false, glm::value_ptr(PVsky));

    //Bind skybox texture to unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, scene::skybox_tex_id);
    glUniform1i(UniformLocs::skybox_tex, 1); // we bound our texture to texture unit 1

    //skybox render state:
    //Don't write to depth buffer. The sky is further away than anything else.
    //Nothing should fail the depth test with the sky.
    glDepthMask(GL_FALSE);
    glBindVertexArray(scene::skybox_mesh.mVao);
    scene::skybox_mesh.DrawMesh();
    glDepthMask(GL_TRUE);
    //End pass 0

    //Pass 1: draw tree
    scene::audio_processing.BindFftForRead(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene::tex_id);
    glUniform1i(UniformLocs::diffuse_tex, 0);

    glUniform1i(UniformLocs::pass, 1);
    Uniforms::M = glm::rotate(scene::angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(scene::scale*2.2));
    glUniformMatrix4fv(UniformLocs::M, 1, false, glm::value_ptr(Uniforms::M));
    Uniforms::PV = P * V; 
    glUniformMatrix4fv(UniformLocs::PV, 1, false, glm::value_ptr(Uniforms::PV));
   
    glBindVertexArray(scene::greenMesh.mVao);
    scene::greenMesh.DrawMesh();

    //Pass 2:draw yellow
    glUniform1i(UniformLocs::pass, 2);
    glBindVertexArray(scene::yellowMesh.mVao);
    scene::yellowMesh.DrawMesh();

    //Pass 3:draw red
    glUniform1i(UniformLocs::pass, 3);
    glBindVertexArray(scene::redMesh.mVao);
    scene::redMesh.DrawMesh();

    //Pass 4:draw blue
    glUniform1i(UniformLocs::pass, 4);
    glBindVertexArray(scene::blueMesh.mVao);
    scene::blueMesh.DrawMesh();

    //Pass 5:draw land
    glUniform1i(UniformLocs::pass, 5);
    Uniforms::M = glm::rotate(scene::angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(scene::scale * 3));
    glUniformMatrix4fv(UniformLocs::M, 1, false, glm::value_ptr(Uniforms::M));
    glBindVertexArray(scene::landMesh.mVao);
    scene::landMesh.DrawMesh();

    //Pass 6:draw building
    glUniform1i(UniformLocs::pass, 6);
    glBindVertexArray(scene::cubeMesh.mVao);

    Uniforms::build_count = 0;
    for (float x = 0.2; x < 2.7; ) {
        x = x + (0.002/x);
        std::mt19937 rng(static_cast<unsigned int>(x * 1000));
        std::uniform_real_distribution<float> dist(-0.03f, 0.03f);
        std::uniform_real_distribution<float> color(0.11f, 0.3f);
        float randomOffsetX = dist(rng);
        float randomOffsetZ = dist(rng);
        glm::vec3 randColor = glm::vec3(color(rng), color(rng), color(rng));
        glUniform3fv(UniformLocs::randColor, 1, glm::value_ptr(randColor));
        Uniforms::M = glm::rotate(scene::angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(scene::scale)) * glm::translate(glm::vec3(x*sin(x*60)+ randomOffsetX, 0.0f, x*cos(x*60)+ randomOffsetZ));
        glUniformMatrix4fv(UniformLocs::M, 1, false, glm::value_ptr(Uniforms::M));
        glUniform1i(UniformLocs::build_count, Uniforms::build_count);
        Uniforms::build_count++;
        scene::cubeMesh.DrawMesh();
    }

    //Pass 7:draw snow
    glUniform1i(UniformLocs::pass, 7);
    Uniforms::M = glm::rotate(scene::angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(scene::scale));
    glUniformMatrix4fv(UniformLocs::M, 1, false, glm::value_ptr(Uniforms::M));
    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, MAX_PARTICLES); // Render as points
    glBindVertexArray(0);

    //draw_gui(window);

    // Swap front and back buffers
    glfwSwapBuffers(window);
}

void idle()
{
    glUseProgram(scene::shader);
    float time_sec = static_cast<float>(glfwGetTime());
    glUniform1f(UniformLocs::time, static_cast<GLfloat>(time_sec));

   float deltaTime = time_sec - scene::last_time_sec;
   scene::last_time_sec = time_sec;
   updateParticles(deltaTime);
   scene::audio_processing.Compute();
}

void reload_shader()
{
   std::string vs = scene::shader_dir + scene::vertex_shader;
   std::string fs = scene::shader_dir + scene::fragment_shader;

   GLuint new_shader = InitShader(vs.c_str(), fs.c_str());

   if (new_shader == -1) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
   }
   else
   {
      glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);

      if (scene::shader != -1)
      {
         glDeleteProgram(scene::shader);
      }
      scene::shader = new_shader;

      //glUseProgram(scene::shader);

      //Get uniform locations
      UniformLocs::skybox_tex = glGetUniformLocation(scene::shader, "skybox_tex");
      UniformLocs::diffuse_tex = glGetUniformLocation(scene::shader, "diffuse_tex");

      UniformLocs::Tree = glGetUniformLocation(scene::shader, "tree");
      UniformLocs::Red = glGetUniformLocation(scene::shader, "red");
      UniformLocs::Yellow = glGetUniformLocation(scene::shader, "yellow");
      UniformLocs::Blue = glGetUniformLocation(scene::shader, "blue");
      UniformLocs::randColor = glGetUniformLocation(scene::shader, "randColor");

      UniformLocs::PV = glGetUniformLocation(scene::shader, "PV");
      UniformLocs::M = glGetUniformLocation(scene::shader, "M");

      UniformLocs::pass = glGetUniformLocation(scene::shader, "pass");
      UniformLocs::eye_w = glGetUniformLocation(scene::shader, "eye_w");
      UniformLocs::build_count = glGetUniformLocation(scene::shader, "buildCount");
      UniformLocs::time = glGetUniformLocation(scene::shader, "time");

      glUniform3fv(UniformLocs::Tree, 1, glm::value_ptr(Uniforms::Tree));
      glUniform3fv(UniformLocs::Red, 1, glm::value_ptr(Uniforms::Red));
      glUniform3fv(UniformLocs::Yellow, 1, glm::value_ptr(Uniforms::Yellow));
      glUniform3fv(UniformLocs::Blue, 1, glm::value_ptr(Uniforms::Blue));
   }
}

//This function gets called when a key is pressed
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   std::cout << "key : " << key << ", " << char(key) << ", scancode: " << scancode << ", action: " << action << ", mods: " << mods << std::endl;

   if (action == GLFW_PRESS)
   {
      switch (key)
      {
      case 'r':
      case 'R':
         reload_shader();
         break;

      case GLFW_KEY_ESCAPE:
         glfwSetWindowShouldClose(window, GLFW_TRUE);
         break;
      }
   }
}

//This function gets called when the mouse moves over the window.
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (window::mousePressed) {
        // Calculate mouse movement
        float xOffset = static_cast<float>(xpos - window::lastX) * window::mouseSensitivity*0.5;
        window::lastX = xpos;
        scene::Cangle -= xOffset;

        float yOffset = static_cast<float>(ypos - window::lastY) * window::mouseSensitivity;
        window::lastY = ypos;
        scene::Cheight = glm::clamp(scene::Cheight + yOffset, 0.2f, 3.0f);

        // Calculate new camera direction
        Uniforms::eye_w = glm::vec3(scene::Cdistance * sin(scene::Cangle), scene::Cheight, scene::Cdistance * cos(scene::Cangle));
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
     scene::Cdistance = glm::clamp(scene::Cdistance - static_cast<float>(yoffset) * window::scrollSensitivity, 1.0f, 3.0f); // Constrain zoom
     Uniforms::eye_w = glm::vec3(scene::Cdistance * sin(scene::Cangle), scene::Cheight, scene::Cdistance * cos(scene::Cangle));
}



//This function gets called when a mouse button is pressed.
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            window::mousePressed = true;
            glfwGetCursorPos(window, &window::lastX, &window::lastY); // Capture current mouse position

        }
        else if (action == GLFW_RELEASE) {
            window::mousePressed = false;
        }
    }
}


//Initialize OpenGL state. This function only gets called once.
void init()
{
   glewInit();
   RegisterDebugCallback();

   std::ostringstream oss;
   //Get information about the OpenGL version supported by the graphics driver.	
   oss << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   oss << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   oss << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   oss << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;

   //Output info to console
   std::cout << oss.str();

   //Output info to file
   std::fstream info_file("info.txt", std::ios::out | std::ios::trunc);
   info_file << oss.str();
   info_file.close();

   //Set the color the screen will be cleared to when glClear is called
   glClearColor(window::clear_color[0], window::clear_color[1], window::clear_color[2], window::clear_color[3]);

   glEnable(GL_DEPTH_TEST);

   reload_shader();

   scene::skybox_mesh = LoadMesh(scene::asset_dir + scene::skybox_mesh_name);
   scene::skybox_tex_id = LoadSkybox(scene::asset_dir + scene::skybox_texture_name);

   scene::greenMesh = LoadMesh(scene::asset_dir + scene::green_mesh_name);
   scene::redMesh = LoadMesh(scene::asset_dir + scene::red_mesh_name);
   scene::yellowMesh = LoadMesh(scene::asset_dir + scene::yellow_mesh_name);
   scene::blueMesh = LoadMesh(scene::asset_dir + scene::blue_mesh_name);
   scene::landMesh = LoadMesh(scene::asset_dir + scene::land_mesh_name);
   scene::cubeMesh = LoadMesh(scene::asset_dir + scene::cube_mesh_name);
   scene::tex_id = LoadTexture(scene::asset_dir + scene::tex_name); 
   scene::audio_processing.Init(scene::asset_dir + scene::audio_filename);
   initParticles();
   setupParticleVAO();
   glPointSize(3.0f);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
   glViewport(0, 0, width, height);
   scene::ratio = (float)width / height;
   //fix aspect in P
}

// C++ programs start executing in the main() function.
int main(void)
{
   GLFWwindow* window;

   // Initialize the library
   if (!glfwInit())
   {
      return -1;
   }

#ifdef _DEBUG
   glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

   // Create a windowed mode window and its OpenGL context
   window = glfwCreateWindow(window::size[0], window::size[1], window::title, NULL, NULL);
   if (!window)
   {
      glfwTerminate();
      return -1;
   }

   //Register callback functions with glfw. 
   glfwSetKeyCallback(window, keyboard);
   glfwSetMouseButtonCallback(window, mouse_button_callback);
   glfwSetCursorPosCallback(window, cursor_position_callback);
   glfwSetScrollCallback(window, scroll_callback);
   glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

   // Make the window's context current
   glfwMakeContextCurrent(window);

   init();

   // New in Lab 2: Init ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 150");

   // Loop until the user closes the window 
   while (!glfwWindowShouldClose(window))
   {
      idle();
      display(window);

      // Poll for and process events 
      glfwPollEvents();
   }

   // New in Lab 2: Cleanup ImGui
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwTerminate();
   return 0;
}