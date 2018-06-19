#include <stdlib.h>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

enum {PRG_CUBE,PRG_BODY,PRG_CNT};
enum {VAO_CUBE,VAO_BODY,VAO_CNT};
enum {VBO_CUBE,VBO_BODY,VBO_CNT};
enum {EBO_CUBE,EBO_CNT};
enum {XFO_BODY,XFO_CNT};
enum {XFB_BODY,XFB_CNT};
enum {TBO_POS,TBO_MASS,TBO_CNT};
enum {TBB_POS,TBB_MASS,TBB_CNT};

typedef struct {
    glm::vec4 pos;
    glm::vec3 vel;
    GLfloat mass;
} Body;

typedef struct {
    GLFWwindow *wnd;
    double t;
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 model;
    GLfloat fov;
    glm::vec4 eye;
    GLboolean autoRot;
    GLuint prg[PRG_CNT];//program
    GLuint vao[VAO_CNT];//vertex array
    GLuint vbo[VBO_CNT];//vertex buffer
    GLuint ebo[EBO_CNT];//element buffer
    GLuint xfo[XFO_CNT];//feedback transform
    GLuint xfb[XFB_CNT];//feedback transform buffer
    GLuint tbo[TBO_CNT];//buffer texture
    GLuint tbb[TBB_CNT];//buffer texture buffer
    std::vector<Body> bodies;
} UserData;

static const GLchar * cubeVShader =
        "#version 420 core\n"
        "layout (location=0) in vec4 vPos;"
        "uniform mat4 mvp;"
        "void main(){"
        "  gl_Position = mvp * vPos;"
        "}";

static const GLchar * cubeFShader =
        "#version 420 core\n"
        "out vec4 col;"
        "void main(){"
        "  col = vec4(0.0f,1.0f,1.0f,1.0f);"
        "}";

static const GLchar * bodyVShader =
        "#version 420 core\n"
        "struct Body {"
        "  vec4 pos;"
        "  vec3 vel;"
        "  float mass;"
        "};"
        ""
        "layout (location=0) in vec4 vPos;"
        "layout (location=1) in vec3 vVel;"
        "layout (location=2) in float vMass;"
        ""
        "uniform mat4 mvp;"
        "uniform float dt;"
        "uniform int cnt;"
        "uniform samplerBuffer posBuffer;"
        "uniform samplerBuffer massBuffer;"
        ""
        "out vec4 nPos;"
        "out vec3 nVel;"
        "out float nMass;"
        ""
        "void main(){"
        "  const float G = 6.674e-11;"
        "  vec3 f = vec3(0.0f,0.0f,0.0f);"
        "  for(int i=0;i<cnt;i++){"
        "    vec4 pos = texelFetch(posBuffer,i);"
        "    float mass = texelFetch(massBuffer,i).r;"
        "    float d = distance(pos, vPos);"
        "    if(d<0.000001f) continue;"
        "    f += G * vMass * mass * (pos.xyz - vPos.xyz)/d/d/d;"
        "  }"
        "  vec3 accel = f/vMass;"
        "  nVel = vVel + accel * dt;"
        "  nPos = vPos + vec4(vVel * dt, 0.0f);"
        "  nMass = vMass;"
        "  gl_Position = mvp * nPos;"
        "}";



static const GLchar * bodyFShader =
        "#version 420 core\n"
        "out vec4 col;"
        "void main(){"
        "  col = vec4(1.0f,1.0f,1.0f,1.0f);"
        "}";



static const GLfloat cube[][4] = {
    {-1.0f,-1.0f,-1.0f, 1.0f},
    {-1.0f,-1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f,-1.0f, 1.0f},
    {-1.0f, 1.0f, 1.0f, 1.0f},
    { 1.0f,-1.0f,-1.0f, 1.0f},
    { 1.0f,-1.0f, 1.0f, 1.0f},
    { 1.0f, 1.0f,-1.0f, 1.0f},
    { 1.0f, 1.0f, 1.0f, 1.0f}
};

static const GLushort cubei[] = {0,1,2,3,4,5,6,7,
                                 0,2,1,3,4,6,5,7,
                                 0,4,1,5,2,6,3,7};

GLuint loadShaders(const std::vector<GLenum> &type,
                   const std::vector<std::string> &shader,
                   const std::vector<const GLchar *> &xfb = std::vector<const GLchar*>())
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
        if ( !res ){
            glGetShaderiv(so, GL_INFO_LOG_LENGTH, &infoLength);
            std::vector<char> msg(infoLength+1);
            glGetShaderInfoLog(so, infoLength, NULL, &msg[0]);
            std::cerr << "shader " << i << " " << std::string(&msg[0]);
            return 0;
        }
        glAttachShader(po, so);
        sos[i] = so;
    }

    if(!xfb.empty()){
        glTransformFeedbackVaryings(po,
                                    xfb.size(), xfb.data(),
                                    GL_INTERLEAVED_ATTRIBS);
    }

    glLinkProgram(po);
    glGetProgramiv(po, GL_LINK_STATUS, &res);
    if ( !res ){
        glGetProgramiv(po, GL_INFO_LOG_LENGTH, &infoLength);
        std::vector<char> msg(infoLength+1);
        glGetProgramInfoLog(po, infoLength, NULL, &msg[0]);
        std::cerr << std::string(&msg[0]) << std::endl;
        return 0;
    }

    for(size_t i=0;i<sos.size();i++){
        glDetachShader(po, sos[i]);
        glDeleteShader(sos[i]);
    }

    return po;
}

void onError(int err, const char * msg)
{
    std::cerr << err << ":" << msg << std::endl;
    exit(-err);
}

void onResize(GLFWwindow *wnd, int w, int h)
{
    UserData *d = (UserData*)glfwGetWindowUserPointer(wnd);

    float aspect = float(w)/float(h);
    glViewport(0,0,w,h);
    glScissor(0,0,w,h);

    d->proj = glm::perspective(glm::radians(d->fov), aspect, 0.01f, 100.0f);
}

void onFocus(GLFWwindow *, int)
{

}

void onKey(GLFWwindow *wnd, int key, int, int action, int)
{
    UserData *d = (UserData*)glfwGetWindowUserPointer(wnd);

    if(key==GLFW_KEY_ESCAPE && action==GLFW_RELEASE){
        glfwSetWindowShouldClose(wnd,GLFW_TRUE);
    }
    if(key==GLFW_KEY_R && action==GLFW_RELEASE){
        d->autoRot = !d->autoRot;
    }
    if(key==GLFW_KEY_LEFT && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(10.0f),
                                    glm::vec3(0.0f,1.0f,0.0f));
        d->eye = rot * d->eye;
        d->view = glm::lookAt(glm::vec3(d->eye),
                              glm::vec3(0.0f,0.0f,0.0f),
                              glm::vec3(0.0f,1.0f,0.0f));
    }
    if(key==GLFW_KEY_RIGHT && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(-10.0f),
                                    glm::vec3(0.0f,1.0f,0.0f));
        d->eye = rot * d->eye;
        d->view = glm::lookAt(glm::vec3(d->eye),
                              glm::vec3(0.0f,0.0f,0.0f),
                              glm::vec3(0.0f,1.0f,0.0f));
    }
    if(key==GLFW_KEY_UP && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(10.0f),
                                    glm::vec3(1.0f,0.0f,0.0f));
        d->eye = rot * d->eye;
        d->view = glm::lookAt(glm::vec3(d->eye),
                              glm::vec3(0.0f,0.0f,0.0f),
                              glm::vec3(0.0f,1.0f,0.0f));
    }
    if(key==GLFW_KEY_DOWN && action==GLFW_PRESS){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(-10.0f),
                                    glm::vec3(1.0f,0.0f,0.0f));
        d->eye = rot * d->eye;
        d->view = glm::lookAt(glm::vec3(d->eye),
                              glm::vec3(0.0f,0.0f,0.0f),
                              glm::vec3(0.0f,1.0f,0.0f));
    }

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

void onScroll(GLFWwindow *wnd, double, double dy)
{
    UserData *d = (UserData*)glfwGetWindowUserPointer(wnd);

    if(dy>0) { d->eye *= 1.1f;}
    else if(dy<0) { d->eye *= 0.9f;}
    d->view = glm::lookAt(glm::vec3(d->eye),
                          glm::vec3(0.0f,0.0f,0.0f),
                          glm::vec3(0.0f,1.0f,0.0f));
}

void init(UserData *d, GLuint cnt)
{
    //USer Data
    d->autoRot = GLFW_TRUE;
    d->fov = 60.0f;
    d->eye = glm::vec4(0.1f, 0.1f, 3.2f, 1.0f);
    d->proj = glm::mat4(1.0f);
    d->model = glm::mat4(1.0f);
    d->view = glm::lookAt(glm::vec3(d->eye),
                          glm::vec3(0.0f,0.0f,0.0f),
                          glm::vec3(0.0f,1.0f,0.0f));

    //GLFW and Window
    glfwSetErrorCallback(onError);
    if(!glfwInit()) exit(-1);

    glfwDefaultWindowHints();
    //glfwWindowHint(GLFW_SAMPLES,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *mon = glfwGetPrimaryMonitor();
    if(!mon) exit(-1);
    const GLFWvidmode *mod = glfwGetVideoMode(mon);
    if(!mod) exit(-1);
    int w = mod->width/2;
    int h = mod->height/2;

    GLFWwindow *window = glfwCreateWindow(w,h,"nBody",NULL,NULL);
    if(!window) exit(-1);
    d->wnd = window;
    glfwSetWindowUserPointer(window,d);

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
    d->t = glfwGetTime();

    //GLEW
    GLenum ret = glewInit();
    if(ret != GLEW_OK){
        std::cerr << glewGetErrorString(ret);
        exit(-1);
    }

    onResize(window,w,h);

    //GL init
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f,0.0f,0.0f,0.0f);

    //load shaders
    GLuint prg;
    prg = loadShaders(
    {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER},
    {cubeVShader, cubeFShader}, {});
    if(!prg) exit(-1);
    d->prg[PRG_CUBE] = prg;

    prg = loadShaders(
    {GL_VERTEX_SHADER,GL_FRAGMENT_SHADER},
    {bodyVShader, bodyFShader}, {"nPos", "nVel", "nMass"});
    if(!prg) exit(-1);
    d->prg[PRG_BODY] = prg;

    //generate objects
    glGenVertexArrays(VAO_CNT,d->vao);
    glGenBuffers(VBO_CNT,d->vbo);
    glGenBuffers(EBO_CNT,d->ebo);
    glGenTransformFeedbacks(XFO_CNT,d->xfo);
    glGenBuffers(XFB_CNT,d->xfb);
    glGenTextures(TBO_CNT,d->tbo);
    glGenBuffers(TBB_CNT,d->tbb);

    //ranfom initial bodies
    d->bodies.resize(cnt);
    for(GLuint i=0;i<cnt;i++){
        Body &b = d->bodies[i];
        b.mass = 1;
        b.vel = glm::vec3(0,0,0);
        float pa = (float)rand();
        float pb = (float)rand();
        float pr = float(rand())/float(RAND_MAX);
        b.pos = glm::vec4(pr*std::cos(pb)*std::cos(pa),
                          pr*std::sin(pb),
                          pr*std::cos(pb)*std::sin(pa),
                          1.0f);
    }
}



void drawCube(UserData *d)
{
    GLuint prg = d->prg[PRG_CUBE];
    GLuint vao = d->vao[VAO_CUBE];
    GLuint vbo = d->vbo[VBO_CUBE];
    GLuint ebo = d->ebo[EBO_CUBE];

    glUseProgram(prg);

    GLint mvpLoc = glGetUniformLocation(prg,"mvp");
    GLint vPosLoc = glGetAttribLocation(prg,"vPos");

    glm::mat4 mvp = d->proj * d->view * d->model;
    glUniformMatrix4fv(mvpLoc,1,GL_FALSE,glm::value_ptr(mvp));

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,8*sizeof(glm::vec4),cube,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,24*sizeof(GLushort),cubei,GL_STATIC_DRAW);

    glVertexAttribPointer(vPosLoc,4,GL_FLOAT,GL_FALSE,0,(const void*)0);
    glEnableVertexAttribArray(vPosLoc);

    glDrawElements(GL_LINES,24,GL_UNSIGNED_SHORT,NULL);

    glDisableVertexAttribArray(vPosLoc);

    glInvalidateBufferData(GL_ELEMENT_ARRAY_BUFFER);
    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void drawBodies(UserData *d, double dt)
{
    GLuint prg = d->prg[PRG_BODY];
    GLuint vao = d->vao[VAO_BODY];
    GLuint vbo = d->vbo[VBO_BODY];
    GLuint xfo = d->xfo[XFO_BODY];
    GLuint xfb = d->xfb[XFB_BODY];
    GLuint tboPos = d->tbo[TBO_POS];
    GLuint tbbPos = d->tbb[TBB_POS];
    GLuint tboMass = d->tbo[TBO_MASS];
    GLuint tbbMass = d->tbb[TBB_MASS];
    GLuint sz = d->bodies.size();

    glUseProgram(prg);

    //locations
    GLint mvpLoc = glGetUniformLocation(prg,"mvp");
    GLint dtLoc = glGetUniformLocation(prg,"dt");
    GLint cntLoc = glGetUniformLocation(prg,"cnt");
    GLint vPosLoc = glGetAttribLocation(prg,"vPos");
    GLint vVelLoc = glGetAttribLocation(prg,"vVel");
    GLint vMassLoc = glGetAttribLocation(prg,"vMass");
    GLint posLoc = glGetUniformLocation(prg,"posBuffer");
    GLint massLoc = glGetUniformLocation(prg,"massBuffer");

    //uniform mvp and dt
    glm::mat4 mvp = d->proj * d->view * d->model;
    glUniformMatrix4fv(mvpLoc,1,GL_FALSE,glm::value_ptr(mvp));
    glUniform1f(dtLoc,dt);
    glUniform1i(cntLoc,sz);

    //vertices, pos, vel and mass to the shader
    glBindVertexArray(vao);

    //pso vel mass in
    {glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,sz*sizeof(Body),NULL,GL_STATIC_DRAW);
        Body * buffer =  (Body*)glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY);
        memcpy(buffer,d->bodies.data(),sz*sizeof(Body));
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    glVertexAttribPointer(vPosLoc,4,GL_FLOAT,GL_FALSE,sizeof(Body),
                          (const void*)0);
    glVertexAttribPointer(vVelLoc,3,GL_FLOAT,GL_FALSE,sizeof(Body),
                          (const void*)(sizeof(glm::vec4)));
    glVertexAttribPointer(vMassLoc,1,GL_FLOAT,GL_FALSE,sizeof(Body),
                          (const void*)(sizeof(glm::vec4) + sizeof(glm::vec3)));
    glEnableVertexAttribArray(vPosLoc);
    glEnableVertexAttribArray(vVelLoc);
    glEnableVertexAttribArray(vMassLoc);

    //pos texture buffer in
    {glBindBuffer(GL_TEXTURE_BUFFER, tbbPos);
        glBufferData(GL_TEXTURE_BUFFER,sz*sizeof(glm::vec4),NULL,GL_STATIC_DRAW);
        glm::vec4 * buffer =  (glm::vec4*)glMapBuffer(GL_TEXTURE_BUFFER,GL_WRITE_ONLY);
        for(size_t i=0;i<sz;i++){
            *(buffer+i) = d->bodies[i].pos;
        }
        glUnmapBuffer(GL_TEXTURE_BUFFER);
    }
    glBindTexture(GL_TEXTURE_BUFFER,tboPos);
    glTextureBuffer(tboPos,GL_RGBA32F, tbbPos);
    glBindBuffer(GL_ARRAY_BUFFER,tbbPos);
    glVertexAttribPointer(posLoc, 4, GL_FLOAT,GL_FALSE, 0, (const void*)0);
    glEnableVertexAttribArray(posLoc);

    //mass texture buffer in
    {glBindBuffer(GL_TEXTURE_BUFFER, tbbMass);
        glBufferData(GL_TEXTURE_BUFFER,sz*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
        GLfloat * buffer =  (GLfloat*)glMapBuffer(GL_TEXTURE_BUFFER,GL_WRITE_ONLY);
        for(size_t i=0;i<sz;i++){
            *(buffer+i) = d->bodies[i].mass;
        }
        glUnmapBuffer(GL_TEXTURE_BUFFER);
    }
    glBindTexture(GL_TEXTURE_BUFFER,tboMass);
    glTextureBuffer(tboMass,GL_R32F, tbbMass);
    glBindBuffer(GL_ARRAY_BUFFER,tbbMass);
    glVertexAttribPointer(massLoc, 1, GL_FLOAT,GL_FALSE, 0, (const void*)0);
    glEnableVertexAttribArray(massLoc);

    //feedback, new pos and vels and mass out
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,xfo);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,xfb);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,sz*sizeof(Body),NULL,GL_DYNAMIC_COPY);
    glTransformFeedbackBufferBase(xfo,0,xfb);

    //draw, capture new pos,vel,mass
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0 ,sz);
    glEndTransformFeedback();

    //update new pos and vels and mass from shader
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,xfo);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,xfb);
    Body * feedback = (Body*)glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,GL_READ_ONLY);
    memcpy(d->bodies.data(),feedback,sz*sizeof(Body));
    glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
    std::cout << std::scientific << glm::distance(d->bodies[0].pos,d->bodies[1].pos) << std::endl;

    glDisableVertexAttribArray(vPosLoc);
    glDisableVertexAttribArray(vVelLoc);
    glDisableVertexAttribArray(vMassLoc);

    //clean up
    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,0);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,0);
    glBindBuffer(GL_TEXTURE_BUFFER,0);
    glBindTexture(GL_TEXTURE_BUFFER,0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void display(UserData *d)
{
    double t = glfwGetTime();
    float dt = t - d->t;
    d->t = t;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(d->autoRot){
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                    glm::radians(10.0f*dt),
                                    glm::vec3(0.0f,1.0f,0.0f));
        d->eye = rot * d->eye;
        d->view = glm::lookAt(glm::vec3(d->eye),
                              glm::vec3(0.0f,0.0f,0.0f),
                              glm::vec3(0.0f,1.0f,0.0f));
    }
    drawCube(d);
    drawBodies(d, 60.0f * 60.0f * dt);
}
void finalize(UserData *d)
{
    glUseProgram(0);
    for(int i=0;i<PRG_CNT;i++) glDeleteProgram(d->prg[0]);
    glDeleteBuffers(VBO_CNT,d->vbo);
    glDeleteBuffers(EBO_CNT,d->ebo);
    glDeleteVertexArrays(VAO_CNT,d->vao);

    glDeleteTransformFeedbacks(XFO_CNT,d->xfo);
    glDeleteBuffers(XFB_CNT,d->xfb);

    glDeleteBuffers(TBB_CNT,d->tbb);
    glDeleteTextures(TBO_CNT,d->tbo);

    glfwDestroyWindow(d->wnd);
    glfwTerminate();

    d->bodies.clear();
}

int main(int , char **)
{
    UserData d;
    init(&d, 2);
    do{
        display(&d);
        glfwSwapBuffers(d.wnd);
        glfwPollEvents();
    }while(!glfwWindowShouldClose(d.wnd));

    finalize(&d);
    return 0;
}
