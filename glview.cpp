#include "glview.h"
#include "nbody.h"

#include <iostream>
#include <vector>

#include <GL/glew.h>
//#include <epoxy/gl.h>
//#include <epoxy/glx.h>
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/quaternion.hpp>

extern std::vector<Body*> univ;

const GLchar * pvShader =
        "#version 330 core\n"
        "layout (location=0) in vec3 vPos;"
        "layout (location=1) in float vMass;"
        "uniform mat4 mvp;"
        "void main(){"
        "  vec4 pos = mvp * vec4(vPos, 1.0f);"
        "  gl_Position = pos;"
        "  gl_PointSize = vMass;"
        "}";

const GLchar * lvShader =
        "#version 330 core\n"
        "layout (location=0) in vec3 vPos;"
        "uniform mat4 mvp;"
        "void main(){"
        "  gl_Position = mvp * vec4(vPos, 1.0f);"
        "}";

const GLchar * pfShader =
        "#version 330 core\n"
        "out vec4 col;"
        "void main(){"
        "  const vec4 color1 = vec4(0.6f, 0.0f, 0.0f, 1.0f);"
        "  const vec4 color2 = vec4(0.9f, 0.7f, 1.0f, 0.0f);"
        "  vec2 temp = gl_PointCoord.xy - vec2(0.5f,0.5f);"
        "  float f = dot(temp, temp);"
        "  if (f > 0.25f) discard;"
        "  col = mix(color1, color2, smoothstep(0.1f, 0.25f, f));"
        "}";

const GLchar * lfShader =
        "#version 330 core\n"
        "out vec4 col;"
        "void main(){"
        "  col = vec4(1.0f,1.0f,1.0f,1.0f);"
        "}";

const GLfloat black[] = {0.0f,0.0f,0.0f,1.0f};
const GLfloat white[] = {1.0f,1.0f,1.0f,1.0f};
const GLfloat depth[] = {-1.0f};

const GLfloat cube[][3] = {
    {-1.0f,-1.0f,-1.0f},
    {-1.0f,-1.0f, 1.0f},
    {-1.0f, 1.0f,-1.0f},
    {-1.0f, 1.0f, 1.0f},
    { 1.0f,-1.0f,-1.0f},
    { 1.0f,-1.0f, 1.0f},
    { 1.0f, 1.0f,-1.0f},
    { 1.0f, 1.0f, 1.0f}
};

const GLushort cubei[] = {0,1,2,3,4,5,6,7,
                          0,2,1,3,4,6,5,7,
                          0,4,1,5,2,6,3,7};

GLuint vao[2];
GLuint vbo[2];
GLuint ebo[1];
GLuint po[2];

glm::mat4 projection;
glm::mat4 view;
glm::mat4 model = glm::mat4(1.0f);

GLfloat fov = 60.0f;
double told = 0.0f;
GLboolean rotate = GL_TRUE;
glm::vec3 eye = glm::vec3(0.1f,0.1f,3.2f);

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
        //GLuint so = glCreateShaderProgramv(type[i],);
        //(GLenum type, GLsizei count, const GLchar * const * strings);
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

    //Gluint pp[2];
    //glCreateProgramPipelines(2,pp);
    //glBindProgramPipeline(pp[0]);
    //glUseProgramStages();
    //(GLuint pipeline, GLbitfield stages, GLuint program);

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

void onResize(GLFWwindow *, int w, int h)
{
    float aspect = float(w)/float(h);
    glViewport(0,0,w,h);
    glScissor(0,0,w,h);

    //adjust projection accordingly
    projection = glm::perspective(glm::radians(fov), aspect, 0.01f, 100.0f);
}

void onKey(GLFWwindow *window, int key, int, int action, int)
{
    if(key==GLFW_KEY_ESCAPE && action==GLFW_RELEASE){
        //exit on excape
        glfwSetWindowShouldClose(window,GLFW_TRUE);
    }
    if(key==GLFW_KEY_R && action==GLFW_RELEASE){
        //start stop auto rotatoe
        rotate = !rotate;
    }
    if(key==GLFW_KEY_LEFT && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(10.0f),
                                    glm::vec3(0.0f,1.0f,0.0f));
        eye = rot * glm::vec4(eye,1.0f);
        view = glm::lookAt(eye,
                           glm::vec3(0.0f,0.0f,0.0f),
                           glm::vec3(0.0f,1.0f,0.0f));
    }
    if(key==GLFW_KEY_RIGHT && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(-10.0f),
                                    glm::vec3(0.0f,1.0f,0.0f));
        eye = rot * glm::vec4(eye,1.0f);
        view = glm::lookAt(eye,
                           glm::vec3(0.0f,0.0f,0.0f),
                           glm::vec3(0.0f,1.0f,0.0f));
    }
    if(key==GLFW_KEY_UP && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(10.0f),
                                    glm::vec3(1.0f,0.0f,0.0f));
        eye = rot * glm::vec4(eye,1.0f);
        view = glm::lookAt(eye,
                           glm::vec3(0.0f,0.0f,0.0f),
                           glm::vec3(0.0f,1.0f,0.0f));
    }
    if(key==GLFW_KEY_DOWN && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(-10.0f),
                                    glm::vec3(1.0f,0.0f,0.0f));
        eye = rot * glm::vec4(eye,1.0f);
        view = glm::lookAt(eye,
                           glm::vec3(0.0f,0.0f,0.0f),
                           glm::vec3(0.0f,1.0f,0.0f));
    }

}

void onFocus(GLFWwindow *, int)
{

}

void onMouseButton(GLFWwindow *, int, int, int)
{

}

void onCursorPos(GLFWwindow *, double, double)
{
}

void onCursorEnter(GLFWwindow *, int )
{

}

void onScroll(GLFWwindow *, double, double dy)
{
    if(dy>0) { eye *= 1.1f;}
    else if(dy<0) { eye *= 0.9f;}
    view = glm::lookAt(eye,
                       glm::vec3(0.0f,0.0f,0.0f),
                       glm::vec3(0.0f,1.0f,0.0f));
}

void drawCube()
{
    glUseProgram(po[1]);

    GLint mvpLoc = glGetUniformLocation(po[1],"mvp");
    glm::mat4 mvp = projection * view * model;
    glUniformMatrix4fv(mvpLoc,1,GL_FALSE,glm::value_ptr(mvp));


    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glBufferData(GL_ARRAY_BUFFER,8*3*sizeof(GLfloat),cube,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,24*sizeof(GLushort),cubei,GL_STATIC_DRAW);

    GLint vPosLoc = glGetAttribLocation(po[1],"vPos");
    GLint vMassLoc = glGetAttribLocation(po[1],"vMass");

    glVertexAttribPointer(vPosLoc,3,GL_FLOAT,GL_FALSE,0,(const void*)0);
    glVertexAttrib1f(vMassLoc,1.0f);
    glEnableVertexAttribArray(vPosLoc);

    glDrawElements(GL_LINES,24,GL_UNSIGNED_SHORT,NULL);

    glDisableVertexAttribArray(vPosLoc);
    glDisableVertexAttribArray(vMassLoc);
    glInvalidateBufferData(GL_ELEMENT_ARRAY_BUFFER);
    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glBindVertexArray(0);
}

void drawPoints()
{
    glUseProgram(po[0]);

    GLint mvpLoc = glGetUniformLocation(po[0],"mvp");
    glm::mat4 mvp = projection * view * model;
    glUniformMatrix4fv(mvpLoc,1,GL_FALSE,glm::value_ptr(mvp));

    size_t sz = univ.size();


    glEnable(GL_PROGRAM_POINT_SIZE);
    glBindVertexArray(vao[1]);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[1]);
    glBufferData(GL_ARRAY_BUFFER,sz*4*sizeof(GLfloat),NULL,GL_STATIC_DRAW);

    struct vertices_t {
        glm::vec3 pos;
        GLfloat mass;
    } * buffer =  (vertices_t*)glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY);
    for(size_t i=0;i<univ.size();i++){
        Body *b = univ[i];
        buffer[i].pos = b->pos;
        buffer[i].mass = 8.0f*b->r/0.0062f;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    GLint vPosLoc = glGetAttribLocation(po[0],"vPos");
    GLint vMassLoc = glGetAttribLocation(po[0],"vMass");

    glVertexAttribPointer(vPosLoc,3,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),(const void*)0);
    glVertexAttribPointer(vMassLoc,1,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),(const void*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(vPosLoc);
    glEnableVertexAttribArray(vMassLoc);

    glDrawArrays(GL_POINTS, 0 ,sz);

    glDisableVertexAttribArray(vPosLoc);
    glDisableVertexAttribArray(vMassLoc);
    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);

}

void display()
{
    double t = glfwGetTime();
    float dt = t - told;
    told = t;

    joinNBody();
    iterateNBody(100.0*dt);
    //std::cout << univ.size() << std::endl;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(rotate){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(10.0f*dt),
                                    glm::vec3(0.0f,1.0f,0.0f));
        eye = rot * glm::vec4(eye,1.0f);
        view = glm::lookAt(eye,
                           glm::vec3(0.0f,0.0f,0.0f),
                           glm::vec3(0.0f,1.0f,0.0f));
    }


    drawPoints();
    drawCube();

}

int initGLView()
{
    glfwSetErrorCallback(onError);
    if(!glfwInit()) return -1;

    glfwDefaultWindowHints();
    //glfwWindowHint(GLFW_SAMPLES,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *mon = glfwGetPrimaryMonitor();
    const GLFWvidmode *mod = glfwGetVideoMode(mon);
    int w = mod->width/2;
    int h = mod->height/2;
    float aspect = float(w)/float(h);

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
    glfwSetTime(0.0f);

    glewInit();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //glDepthRangef(-1.0f,1.0f);
    glEnable(GL_SCISSOR_TEST);
    glViewport(0,0,w,h);
    glScissor(0,0,w,h);
    //glPolygon
    //glEnable(GL_MULTISAMPLE_ARB);
    //glEnable(GL_MULTISAMPLE);
    //glEnable(GL_SAMPLE_SHADING);
    //glMinSampleShading(0.25f);
    //glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);
    //glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    //glEnable(GL_PROGRAM_POINT_SIZE);
    //glEnable(GL_POINT_SPRITE);
    //glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glClearColor(0.0f,0.0f,0.0f,0.0f);

    glGenVertexArrays(2,vao);
    glGenBuffers(2,vbo);
    glGenBuffers(1,ebo);

    po[0] = loadShaders({GL_VERTEX_SHADER,GL_FRAGMENT_SHADER},{pvShader, pfShader});
    po[1] = loadShaders({GL_VERTEX_SHADER,GL_FRAGMENT_SHADER},{lvShader, lfShader});

    model = glm::mat4(1.0f);
    view = glm::lookAt(eye,
                glm::vec3(0.0f,0.0f,0.0f),
                glm::vec3(0.0f,1.0f,0.0f));
    projection = glm::perspective(glm::radians(fov),
                                     aspect,
                                     0.01f, 100.0f);

    do{
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }while(!glfwWindowShouldClose(window));

    glUseProgram(0);
    glDeleteProgram(po[0]);
    glDeleteProgram(po[1]);
    glDeleteBuffers(2,vbo);
    glDeleteBuffers(1,ebo);
    glDeleteVertexArrays(2,vao);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
