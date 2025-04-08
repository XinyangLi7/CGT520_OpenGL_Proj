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

#include "DebugCallback.h"
#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files

namespace window
{
   const char* const title = USER_NAME " " PROJECT_NAME; //defined in project settings
   int size[2] = {1024, 1024};
   float clear_color[4] = {0.35f, 0.35f, 0.35f, 1.0f};
}

namespace scene
{
   const std::string asset_dir = "assets/";
   const std::string mesh_name = "Amago0.obj";
   const std::string tex_name = "AmagoT.bmp";

   MeshData mesh;
   GLuint tex_id = -1; //Texture map for mesh
   float angle = 0.0f;
   float xangle = 0.2f;
   float eyeAngle = 0.0f;
   float scale = 1.0f;
   float lightPos[3] = { 0.0f, 1.0f, 0.0f };
   float eyePos[3] = { 0.0f, 0.0f, 1.0f };
   float shininess = 3.0f;
   float La[3] = { 0.3f, 0.3f, 0.3f };
   float Ld[3] = { 0.2f, 0.7f, 1.0f };
   float Ls[3] = { 1.0f, 0.7f, 0.2f };

   float Ka[3] = { 0.4f, 0.6f, 0.9f };
   float Kd[3] = { 0.4f, 0.6f, 0.9f };
   float Ks[3] = { 1.0f, 0.6f, 0.3f };

   bool useTexture = false;

   const std::string shader_dir = "shaders/";
   const std::string vertex_shader("lab3_vs.glsl");
   const std::string fragment_shader("lab3_fs.glsl");

   GLuint shader = -1;
}
//For an explanation of this program's structure see https://www.glfw.org/docs/3.3/quick.html 

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

//Draw the ImGui user interface
void draw_gui(GLFWwindow* window)
{
   //Begin ImGui Frame
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   //Draw Gui
   ImGui::Begin("Lab3 window");
   ImGui::SliderFloat("Light Height", &scene::lightPos[1], -5.0f, 5.0f);
  
   ImGui::ColorEdit3("light ambient", scene::La);
   ImGui::SameLine(); HelpMarker(
       "Click on the color square to open a color picker.\n"
       "Click and hold to use drag and drop.\n"
       "CTRL+click on individual component to input value.\n");
   ImGui::ColorEdit3("light diffuse", scene::Ld);
   ImGui::SameLine(); HelpMarker(
       "Click on the color square to open a color picker.\n"
       "Click and hold to use drag and drop.\n"
       "CTRL+click on individual component to input value.\n");
   ImGui::ColorEdit3("light specular", scene::Ls);
   ImGui::SameLine(); HelpMarker(
       "Click on the color square to open a color picker.\n"
       "Click and hold to use drag and drop.\n"
       "CTRL+click on individual component to input value.\n");

   ImGui::ColorEdit3("material ambient", scene::Ka);
   ImGui::SameLine(); HelpMarker(
       "Click on the color square to open a color picker.\n"
       "Click and hold to use drag and drop.\n"
       "CTRL+click on individual component to input value.\n");

   ImGui::ColorEdit3("material diffuse", scene::Kd);
   ImGui::SameLine(); HelpMarker(
       "Click on the color square to open a color picker.\n"
       "Click and hold to use drag and drop.\n"
       "CTRL+click on individual component to input value.\n");
   ImGui::ColorEdit3("material specular", scene::Ks);
   ImGui::SameLine(); HelpMarker(
       "Click on the color square to open a color picker.\n"
       "Click and hold to use drag and drop.\n"
       "CTRL+click on individual component to input value.\n");
   
   ImGui::SliderFloat("Shininess", &scene::shininess, 0.0f, 100.0f);
   ImGui::Checkbox("Set to the Texture", &scene::useTexture);
   ImGui::SliderFloat("Fish Rotation X", &scene::xangle, -glm::pi<float>(), +glm::pi<float>());
   ImGui::SliderFloat("Eye Rotation X", &scene::eyeAngle, -glm::pi<float>()/2.01f, +glm::pi<float>()/2.01f);
   ImGui::End();

   static bool show_test = false;
   if(show_test)
   {
      ImGui::ShowDemoWindow(&show_test);
   }

   //End ImGui Frame
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
   //Clear the screen to the color previously specified in the glClearColor(...) call.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glm::vec3 eye = glm::vec3(glm::rotate(scene::eyeAngle, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(scene::eyePos[0], scene::eyePos[1], scene::eyePos[2],1.0));
   glm::mat4 M = glm::rotate(scene::xangle, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(scene::angle, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(scene::scale * scene::mesh.mScaleFactor));
   glm::mat4 V = glm::lookAt(eye, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(glm::pi<float>()/2.0f, 1.0f, 0.1f, 100.0f);

   glUseProgram(scene::shader);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, scene::tex_id);
   int tex_loc = glGetUniformLocation(scene::shader, "diffuse_tex");
   if (tex_loc != -1)
   {
      glUniform1i(tex_loc, 0); // we bound our texture to texture unit 0
   }

   //Get location for shader uniform variable
   int PVM_loc = glGetUniformLocation(scene::shader, "PVM");
   if (PVM_loc != -1)
   {
      glm::mat4 PVM = P * V * M;
      //Set the value of the variable at a specific location
      glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
   }

   int M_loc = glGetUniformLocation(scene::shader, "M");
   if (M_loc != -1)
   {
       glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M));
   }

   int lightPos_loc = glGetUniformLocation(scene::shader, "lightPos");
   if (lightPos_loc != -1)
   {
       glUniform3fv(lightPos_loc, 1, scene::lightPos);
   }

   int eyePos_loc = glGetUniformLocation(scene::shader, "eyePos");
   if (eyePos_loc != -1)
   {
       glUniform3fv(eyePos_loc, 1, glm::value_ptr(eye));
   }

   int LightAmbient_loc = glGetUniformLocation(scene::shader, "La");
   if (LightAmbient_loc != -1)
   {
       glUniform3fv(LightAmbient_loc, 1, scene::La);
   }

   int LightDiffuse_loc = glGetUniformLocation(scene::shader, "Ld");
   if (LightDiffuse_loc != -1)
   {
       glUniform3fv(LightDiffuse_loc, 1, scene::Ld);
   }

   int LightSpecular_loc = glGetUniformLocation(scene::shader, "Ls");
   if (LightSpecular_loc != -1)
   {
       glUniform3fv(LightSpecular_loc, 1, scene::Ls);
   }

   int MaterialAmbient_loc = glGetUniformLocation(scene::shader, "Ka");
   if (MaterialAmbient_loc != -1)
   {
       glUniform3fv(MaterialAmbient_loc, 1, scene::Ka);
   }

   int MaterialDiffuse_loc = glGetUniformLocation(scene::shader, "Kd");
   if (MaterialDiffuse_loc != -1)
   {
       glUniform3fv(MaterialDiffuse_loc, 1, scene::Kd);
   }

   int MaterialSpecular_loc = glGetUniformLocation(scene::shader, "Ks");
   if (MaterialSpecular_loc != -1)
   {
       glUniform3fv(MaterialSpecular_loc, 1, scene::Ks);
   }

   int a_loc = glGetUniformLocation(scene::shader, "a");
   if (a_loc != -1)
   {
       glUniform1f(a_loc, scene::shininess);
   }

   int useTexture_loc = glGetUniformLocation(scene::shader, "useTexture");
   if (useTexture_loc != -1)
   {
       glUniform1i(useTexture_loc, scene::useTexture);
   }

   glBindVertexArray(scene::mesh.mVao);
   scene::mesh.DrawMesh();

   draw_gui(window);

   // Swap front and back buffers
   glfwSwapBuffers(window);
}

void idle()
{
   float time_sec = static_cast<float>(glfwGetTime());

   //Pass time_sec value to the shaders
   int time_loc = glGetUniformLocation(scene::shader, "time");
   if (time_loc != -1)
   {
      glUniform1f(time_loc, time_sec);
   }
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
void mouse_cursor(GLFWwindow* window, double x, double y)
{
   //std::cout << "cursor pos: " << x << ", " << y << std::endl;
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
   //std::cout << "button : "<< button << ", action: " << action << ", mods: " << mods << std::endl;
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
   scene::mesh = LoadMesh(scene::asset_dir + scene::mesh_name);
   scene::tex_id = LoadTexture(scene::asset_dir + scene::tex_name);
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
   glfwSetCursorPosCallback(window, mouse_cursor);
   glfwSetMouseButtonCallback(window, mouse_button);

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