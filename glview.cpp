#include "glview.h"
#include "nbody.h"

#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

extern std::vector<Body*> univ;

const GLchar * vShader =
        "#version 430 core\n"
        "layout (location=0) in vec3 vPos;"
        "layout (location=1) in vec4 vColor;"
        "out vec4 fColor;"
        "uniform mat4 view;"
        "uniform mat4 prj;"
        "void main(){"
        "  gl_Position = prj * vec4(vPos,1);"
        "  fColor = vColor;"
        "}";

const GLchar * fShader =
        "#version 430 core\n"
        "in vec4 fColor;"
        "out vec4 color;"
        "void main(){"
        "  color = fColor;"
        "}";

const GLfloat black[] = {0.0f,0.0f,0.0f,1.0f};
const GLfloat depth[] = {-1.0f};


GLuint ebo[1];
GLuint ubo[1];
GLuint po;



static const GLushort indicies[] = {0,1,2,3,4,5};


GLuint loadShaders(const std::vector<GLenum> &type,
                 const std::vector<std::string> &shader)
{
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
            std::cout << "shader " << i << " " << std::string(&msg[0]) << std::endl;
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

    return po;
//    glUseProgram(po);

//    for(size_t i=0;i<sos.size();i++){
//        glDetachShader(po, sos[i]);
//        glDeleteShader(sos[i]);
//    }
//    glDeleteProgram(po);
}

void reshape(GLFWwindow *window, int w, int h)
{
    glViewport(0,0,w,h);
    glScissor(0,0,w,h);
}

void display()
{
    double t = glfwGetTime();
    //joinNBody();
    iterateNBody(1000*t);

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

    GLuint vao[1];
    glGenVertexArrays(1,vao);
    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(const void*)0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,(const void*)(sz*3*sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glClearBufferfv(GL_COLOR,0,black);
    glClearBufferfv(GL_DEPTH,0,depth);

    glm::mat4 view = glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,0.0f));
    view = glm::rotate(view, glm::radians(0.0f), glm::vec3(1.0f,0.0f,0.0f));
    GLint viewLoc = glGetUniformLocation(po,"view");
    glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));

//    glm::mat4 prj = glm::lookAt(glm::vec3(1.0f,1.0f,-1.0f),
//                                glm::vec3(0.0f,0.0f,0.0f),
//                                glm::vec3(0.0f,1.0f,0.0f));
//    glm::mat4 prj = glm::frustum(-2.0f, 2.0f, -2.0f, 2.0f, 0.5f, 60000.5f);
    glm::mat4 prj = glm::ortho(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);
    GLint prjLoc = glGetUniformLocation(po,"prj");
    glUniformMatrix4fv(prjLoc,1,GL_FALSE,glm::value_ptr(prj));

    glBindVertexArray(vao[0]);
    glDrawArrays(GL_POINTS, 0 ,sz);

    //glDrawElements(GL_LINES,6,GL_UNSIGNED_SHORT,indicies);
    glDeleteBuffers(1,vbo);
    glDeleteVertexArrays(1,vao);
    free(vertices);
    free(colors);
}

void initGLView()
{
    glfwInit();
    //glfwWindowHint(GLFW_SAMPLES,4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *window = glfwCreateWindow(640,480,"nBody",NULL,NULL);
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window,reshape);
    glewInit();

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

    glGenBuffers(1,ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indicies),indicies,GL_STATIC_DRAW);

//    glGenBuffers(1,vbo);
//    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
//    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices)+sizeof(colors),nullptr,GL_STATIC_DRAW);
//    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(vertices),vertices);
//    glBufferSubData(GL_ARRAY_BUFFER,sizeof(vertices),sizeof(colors),colors);

    glGenBuffers(1,ubo);
    glBindBuffer(GL_UNIFORM_BUFFER,ubo[0]);
    glBufferStorage(GL_UNIFORM_BUFFER,16*sizeof(GLfloat),nullptr,GL_DYNAMIC_STORAGE_BIT);

    po = loadShaders({GL_VERTEX_SHADER,GL_FRAGMENT_SHADER},{vShader, fShader});
    glUseProgram(po);

//    glGenVertexArrays(1,vao);
//    glBindVertexArray(vao[0]);
//    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
//    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(const void*)0);
//    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,(const void*)(sizeof(vertices)));
//    glEnableVertexAttribArray(0);
//    glEnableVertexAttribArray(1);

    do{
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }while(glfwGetKey(window,GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           !glfwWindowShouldClose(window));

    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glfwDestroyWindow(window);
    glfwTerminate();
}
