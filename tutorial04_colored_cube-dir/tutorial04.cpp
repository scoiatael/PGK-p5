// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GL/glfw.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>

#include<sys/time.h>
#include <vector>

struct my_clock_class
{
  static int get_time()
  {
    timeval timer;
    gettimeofday(&timer, NULL);
    return timer.tv_sec*1000000 + timer.tv_usec;
  }

  std::vector< std::pair<int,int> > locks;
  int last_update;
  my_clock_class()
  :  locks(), last_update(get_time())
    {}

  int make_lock(const float& timeout)
  {
    locks.push_back(std::pair<float,float>(0.,timeout));
    return locks.size()-1;
  }
  int get_lock(const int& ID)
  {
    if(ID<locks.size())
      return locks[ID].first;
    return 0;
  }
  void reset_lock(const int& ID)
  {
    if(ID < locks.size())
      locks[ID].first = locks[ID].second;
  }
  void update()
  {
    int cur_time = get_time();
    int interval = cur_time-last_update;
    last_update = cur_time;
    for(unsigned int i=0; i<locks.size();i++)
      locks[i].first-=interval;
  }

};

int main( void )
{
    // Initialise GLFW
    if( !glfwInit() )
    {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      return -1;
    }


    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 1);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    if( !glfwOpenWindow( 1024, 768, 0,0,0,0, 32,0, GLFW_WINDOW ) )
    {
      fprintf( stderr, "Failed to open GLFW window. \n" );
      glfwTerminate();
      return -1;
    }

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
      fprintf(stderr, "Failed to initialize GLEW\n");
      return -1;
    }

    glfwSetWindowTitle( "Zadanie 05" );

    // Ensure we can capture the escape key being pressed below
    glfwEnable( GLFW_STICKY_KEYS );

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.3f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS); 

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders( "tvs.vertexshader", "cfs.fragmentshader" );
    GLuint program2ID = LoadShaders( "tvs.vertexshader", "cfs2.fragmentshader");
    GLuint pIDTable[2] = {programID, program2ID};
        // Get a handle for our "MVP" uniform
    GLuint MatrixIDTable[2] = { glGetUniformLocation(programID, "MVP"), glGetUniformLocation(program2ID, "MVP") };
    GLuint DepthIDTable[2] = { glGetUniformLocation(programID, "depth"), glGetUniformLocation(program2ID, "depth") };

    // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    glm::mat4 View       = glm::lookAt(
                            glm::vec3(0,0,-6), // Camera is at (4,3,-3), in World Space
                            glm::vec3(0,0,0), // and looks at the origin
                            glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                            );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model      = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP        = Projection * View * Model; // Remember, matrix multiplication is the other way around

    // Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
    // A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
    static const GLfloat g_vertex_buffer_data[] = { 
      -1.,-1.,0.,
      0.,1.,0.,
      1.,-1.,0.
      };

    // One color for each vertex. They were generated randomly.
    static const GLfloat g_color_buffer_data[] = { 
      1.,0.,0.,
      0.,1.,0.,
      0.,0.,1.,
      };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint colorbuffer;
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    GLfloat time=0;
    GLfloat depth=0;
    GLfloat timeout=0;
    my_clock_class cl;
    int keytimeoutID = cl.make_lock(100000);
    int FPSshowLockID = cl.make_lock(1000000);
    int frames = 0;
    do{
      frames+=1;
      time = (float)my_clock_class::get_time()/10000.0f;
      cl.update();
      if( glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS 
          && cl.get_lock(keytimeoutID) <= 0)
      {
        depth+=1;
        cl.reset_lock(keytimeoutID);
      }
      
      if(glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS 
          && cl.get_lock(keytimeoutID) <= 0)
      {
        depth-=1;
        cl.reset_lock(keytimeoutID);
      }
      
      if(cl.get_lock(FPSshowLockID) <= 0)
      {
        cl.reset_lock(FPSshowLockID);
        printf("FPS: %d time: %f\n", frames, time); 
        frames=0;
      }
            // Clear the screen
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLfloat scale = sin(time/100)+1;
      for(int i=0; i<2;i++)
      {
        glUseProgram(pIDTable[i]);
	

        glm::mat4 temp = MVP * glm::rotate(glm::mat4(1.0), time, glm::vec3(0.,1.,0.)) *glm::translate(glm::mat4(1.0), glm::vec3((2*i-1),0,0)) *glm::rotate(glm::mat4(1.0), i*180.0f,glm::vec3(1.,0.,0.)) * glm::scale(glm::mat4(1.0), glm::vec3(scale));
        glUniformMatrix4fv(MatrixIDTable[i], 1, GL_FALSE, &temp[0][0]);
        glUniform1i(DepthIDTable[i], depth);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
          );

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                              // stride
                (void*)0                
          );

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
      }

            // Swap buffers
      glfwSwapBuffers();
      
    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
               glfwGetWindowParam( GLFW_OPENED ) );

    // Cleanup VBO and shader
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

