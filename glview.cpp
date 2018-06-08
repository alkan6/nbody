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

const GLchar * vShader =
        "#version 330 core\n"
        "layout (location=0) in vec3 vPos;"
        "layout (location=1) in vec4 vColor;"
        "out vec4 fColor;"
        "uniform mat4 view;"
        "void main(){"
        "  gl_Position = view * vec4(vPos,1);"
        "  fColor = vColor;"
        "}";

const GLchar * fShader =
        "#version 330 core\n"
        "in vec4 fColor;"
        "out vec4 color;"
        "void main(){"
        "  color = fColor;"
        "}";

const GLfloat black[] = {0.0f,0.0f,0.0f,1.0f};

GLuint vao[2];
GLuint vbo[1];
GLuint ebo[1];
GLuint ubo[1];
GLuint po;

static const GLfloat vertices[][3] = {
    //x,y
    {-0.90, -0.90, 0.5},
    { 0.85, -0.90, 0.5},
    {-0.90,  0.85, 0.5},
    { 0.90, -0.85, -0.5},
    { 0.90,  0.90, -0.5},
    {-0.85,  0.90, -0.5}
};

static const GLfloat colors[][4] = {
    //r,g,b,a
    {1.0, 0.0, 1.0, 0.1},
    {1.0, 0.0, 0.0, 0.1},
    {1.0, 0.0, 0.0, 0.1},
    {1.0, 0.0, 0.0, 0.1},
    {1.0, 1.0, 0.0, 0.1},
    {1.0, 0.0, 0.0, 0.1}
};

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

void display()
{
    glClearBufferfv(GL_COLOR_BUFFER_BIT,0,black);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(vao[0]);
    //glDrawArrays(GL_TRIANGLES, 0 ,6);

    glm::mat4 view = glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,0.0f,0.0f));
    view = glm::rotate(view, glm::radians(75.0f), glm::vec3(1.0f,0.0f,0.0f));
    GLint loc = glGetUniformLocation(po,"view");
    glUniformMatrix4fv(loc,1,GL_FALSE,glm::value_ptr(view));

    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,indicies);
}

void initGLView()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *window = glfwCreateWindow(640,480,"nBody",NULL,NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0,0,640,480);


    glGenBuffers(1,ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indicies),indicies,GL_STATIC_DRAW);

    glGenBuffers(1,vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices)+sizeof(colors),nullptr,GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER,0,sizeof(vertices),vertices);
    glBufferSubData(GL_ARRAY_BUFFER,sizeof(vertices),sizeof(colors),colors);

    glGenBuffers(1,ubo);
    glBindBuffer(GL_UNIFORM_BUFFER,ubo[0]);
    glBufferStorage(GL_UNIFORM_BUFFER,16*sizeof(GLfloat),nullptr,GL_DYNAMIC_STORAGE_BIT);

    po = loadShaders({GL_VERTEX_SHADER,GL_FRAGMENT_SHADER},{vShader, fShader});
    glUseProgram(po);

    glGenVertexArrays(1,vao);
    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(const void*)0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,(const void*)(sizeof(vertices)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    do{
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }while(glfwGetKey(window,GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           !glfwWindowShouldClose(window));

    glInvalidateBufferData(GL_ARRAY_BUFFER);
    glDeleteBuffers(1,vbo);
    glDeleteVertexArrays(1,vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}
