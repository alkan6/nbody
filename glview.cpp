#include "glview.h"
#include "nbody.h"

#include <iostream>
#include <vector>

//#include <GL/glew.h>
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/quaternion.hpp>

extern std::vector<Body*> univ;

const GLchar * vShader =
        "#version 330 core\n"
        "layout (location=0) in vec3 vPos;"
        "layout (location=1) in vec4 vCol;"
        "out vec4 fCol;"
        "uniform mat4 model;"
        "uniform mat4 view;"
        "uniform mat4 prj;"
        "void main(){"
        "  gl_Position = prj * view * model * vec4(vPos, 1.0f);"
        "  fCol = vCol;"
        "}";

const GLchar * fShader =
        "#version 330 core\n"
        "in vec4 fCol;"
        "out vec4 col;"
        "void main(){"
        "  col = fCol;"
        "}";

const GLfloat black[] = {0.0f,0.0f,0.0f,1.0f};
const GLfloat depth[] = {-1.0f};

GLuint vao;
GLuint ebo[1];
GLuint ubo[1];
GLuint po;

GLint modelLoc;
GLint viewLoc;
GLint prjLoc;
GLfloat aspect;
GLfloat fov;

GLFWcursor * rotCursor;
GLboolean rotating = GLFW_FALSE;
glm::vec4 camPos = glm::vec4(0.0f,1.0f,1.0f,1.0f);

static const GLushort indicies[] = {0,1,2,3,4,5};

void onError(int err, const char * msg)
{
    std::cerr << err << ":" << msg << std::endl;
}

GLuint loadShaders(const std::vector<GLenum> &type,
                   const std::vector<std::string> &shader)
{
    if(type.empty() || shader.empty()) return 0;
    if(type.size() != shader.size()) return 0;
    GLint res;
    int infoLength;
    std::vector<GLuint> sos(type.size());
    GLuint po = glCreateProgram();

    for(size_t i=0;i<type.size();i++){
        GLuint so = glCreateShader(type[i]);
        const GLchar * p = shader[i].c_str();
        glShaderSource(so,1,&p,NULL);
        glCompileShader(so);
        glGetShaderiv(so, GL_COMPILE_STATUS, &res);
        glGetShaderiv(so, GL_INFO_LOG_LENGTH, &infoLength);
        if ( infoLength > 0 ){
            std::vector<char> msg(infoLength+1);
            glGetShaderInfoLog(so, infoLength, NULL, &msg[0]);
            std::cout << "shader " << i << " "
                      << std::string(&msg[0]) << std::endl;
        }
        glAttachShader(po, so);
        sos[i] = so;
    }

    glLinkProgram(po);
    glGetProgramiv(po, GL_LINK_STATUS, &res);
    glGetProgramiv(po, GL_INFO_LOG_LENGTH, &infoLength);
    if ( infoLength > 0 ){
        std::vector<char> msg(infoLength+1);
        glGetProgramInfoLog(po, infoLength, NULL, &msg[0]);
        std::cout << std::string(&msg[0]) << std::endl;
    }

    for(size_t i=0;i<sos.size();i++){
        glDetachShader(po, sos[i]);
        glDeleteShader(sos[i]);
    }

    return po;
}

void onResize(GLFWwindow *window, int w, int h)
{
    aspect = float(w)/float(h);
    glViewport(0,0,w,h);
    glScissor(0,0,w,h);

    //adjust projection accordingly
    glm::mat4 prj = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
    glUniformMatrix4fv(prjLoc,1,GL_FALSE,glm::value_ptr(prj));
}

void onKey(GLFWwindow *window, int key, int scan, int action, int mode)
{
    if(key==GLFW_KEY_ESCAPE && action==GLFW_RELEASE){
        //exit on excape
        glfwSetWindowShouldClose(window,GLFW_TRUE);
    }

}

void onFocus(GLFWwindow *window, int focused)
{

}

void onMouseButton(GLFWwindow *window, int button, int action, int mode)
{
    switch(button){
    case GLFW_MOUSE_BUTTON_LEFT:
        if(action == GLFW_PRESS){
            glfwSetCursor(window, rotCursor);
            rotating = GLFW_TRUE;
        } else if(action == GLFW_RELEASE){
            glfwSetCursor(window, NULL);
            rotating = GLFW_FALSE;
        }
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
    case GLFW_MOUSE_BUTTON_MIDDLE:
        break;
    }
}

void onCursorPos(GLFWwindow *window, double xpos, double ypos)
{
}

void onCursorEnter(GLFWwindow *window, int entered)
{

}

void onScroll(GLFWwindow *window, double dx, double dy)
{
    //cheap zoom on mouse wheel
    fov -= 1.0 * dy;
    fov = std::min(120.0f,std::max(fov,20.0f));
    glm::mat4 prj = glm::perspective(glm::radians(fov),
                                     aspect,
                                     0.1f, 100.0f);
    glUniformMatrix4fv(prjLoc,1,GL_FALSE,glm::value_ptr(prj));
}

void display()
{
    float dt = glfwGetTime();
    //joinNBody();
    iterateNBody(10000.0*dt);
    glfwSetTime(0.0);

    size_t sz = univ.size();
    GLfloat * vertices = (GLfloat*)malloc(sz*3*sizeof(GLfloat));
    GLfloat * colors = (GLfloat*)malloc(sz*4*sizeof(GLfloat));
    for(size_t i=0;i<univ.size();i++){
        Body *b = univ[i];
        *(vertices + 3*i + 0) = b->pos.x;
        *(vertices + 3*i + 1) = b->pos.y;
        *(vertices + 3*i + 2) = b->pos.z;
        *(colors + 4*i + 0) = 1.0f;
        *(colors + 4*i + 1) = 1.0f;
        *(colors + 4*i + 2) = 1.0f;
        *(colors + 4*i + 3) = 1.0f;
    }

    GLuint vbo[1];
    glGenBuffers(1,vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glBufferData(GL_ARRAY_BUFFER,sz*3*sizeof(GLfloat)+sz*4*sizeof(GLfloat),nullptr,GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER,0,sz*3*sizeof(GLfloat),vertices);
    glBufferSubData(GL_ARRAY_BUFFER,sz*3*sizeof(GLfloat),sz*4*sizeof(GLfloat),colors);

    glBindVertexArray(vao);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(const void*)0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,(const void*)(sz*3*sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                glm::radians(0.0f*dt),
                                glm::vec3(0.0f,1.0f,0.0f));
    camPos = rot * camPos;
    glm::mat4 view = glm::lookAt(glm::vec3(camPos.x, camPos.y, camPos.z),
                                 glm::vec3(0.0f,0.0f,0.0f),
                                 glm::vec3(0.0f,1.0f,0.0f));
    glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    //glClearBufferfv(GL_COLOR,0,black);
    //glClearBufferfv(GL_DEPTH,0,depth);

    glDrawArrays(GL_POINTS, 0 ,sz);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glDeleteBuffers(1,vbo);

    free(vertices);
    free(colors);
}

int initGLView()
{


    glfwSetErrorCallback(onError);
    if(!glfwInit()) return -1;

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *mon = glfwGetPrimaryMonitor();
    const GLFWvidmode *mod = glfwGetVideoMode(mon);
    int w = mod->width/2;
    int h = mod->height/2;
    aspect = float(w)/float(h);
    fov = 120.0f;

    GLFWwindow *window = glfwCreateWindow(w,h,"nBody",NULL,NULL);
    if(!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window,onResize);
    glfwSetWindowFocusCallback(window,onFocus);
    glfwSetKeyCallback(window,onKey);
    glfwSetMouseButtonCallback(window,onMouseButton);
    glfwSetCursorPosCallback(window, onCursorPos);
    glfwSetScrollCallback(window,onScroll);
    glfwSetCursorEnterCallback(window,onCursorEnter);
    glfwSwapInterval(1);

    rotCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    //glewInit();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthRangef(-1.0f,1.0f);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0,0,640,480);
    //glPolygon
    //glEnable(GL_MULTISAMPLE);
    //glEnable(GL_SAMPLE_SHADING);
    //glMinSampleShading(0.25f);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    //

    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);


    po = loadShaders({GL_VERTEX_SHADER,GL_FRAGMENT_SHADER},{vShader, fShader});
    glUseProgram(po);

    glGenBuffers(1,ubo);
    glBindBuffer(GL_UNIFORM_BUFFER,ubo[0]);
    glBufferStorage(GL_UNIFORM_BUFFER,
                    16*sizeof(GLfloat),
                    nullptr,
                    GL_DYNAMIC_STORAGE_BIT);

    modelLoc = glGetUniformLocation(po,"model");
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));

    viewLoc = glGetUniformLocation(po,"view");
    glm::mat4 view = glm::lookAt(glm::vec3(camPos.x,camPos.y,camPos.z),
                                 glm::vec3(0.0f,0.0f,0.0f),
                                 glm::vec3(0.0f,1.0f,0.0f));
    glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));

    prjLoc = glGetUniformLocation(po,"prj");
    //glm::mat4 prj = glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);
    glm::mat4 prj = glm::perspective(glm::radians(fov),
                                     aspect,
                                     0.1f, 100.0f);
    glUniformMatrix4fv(prjLoc,1,GL_FALSE,glm::value_ptr(prj));


    do{
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }while(!glfwWindowShouldClose(window));

    glDeleteProgram(po);
    glDeleteVertexArrays(1,&vao);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
