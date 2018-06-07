#include "glview.h"
#include "nbody.h"

#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>

GLuint vao[1];

void loadShaders(const std::vector<GLenum> &type,
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

    glUseProgram(po);

    for(size_t i=0;i<sos.size();i++){
        glDetachShader(po, sos[i]);
        glDeleteShader(sos[i]);
    }
    glDeleteProgram(po);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(vao[0]);
    glDrawArrays(GL_TRIANGLES, 0 ,6);

}

void initGLView()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow *window = glfwCreateWindow(640,480,"nBody",NULL,NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    glClearColor(0,0,0,1);
    glEnable(GL_DEPTH_TEST);

    static const GLfloat vertices[][6] = {
        {-0.90, -0.90, 1.0, 0.0, 0.0, 1.0},
        { 0.85, -0.90, 1.0, 0.0, 0.0, 1.0},
        {-0.90,  0.85, 1.0, 0.0, 0.0, 1.0},
        { 0.90, -0.85, 1.0, 0.0, 0.0, 1.0},
        { 0.90,  0.90, 1.0, 0.0, 0.0, 1.0},
        {-0.85,  0.90, 1.0, 1.0, 0.0, 1.0}
    };

    GLuint vbo[1];
    glGenBuffers(1,vbo);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

    loadShaders({GL_VERTEX_SHADER,
                 GL_FRAGMENT_SHADER},

    {"#version 330 core\n"
     "in vec2 vPos;"
     "in vec4 vColor;"
     "out vec4 fColor;"
     "void main(){"
     "  gl_Position.xy = vPos;"
     "  gl_Position.z = 0;"
     "  gl_Position.w = 1;"
     "  fColor = vColor;"
     "}",

     "#version 330 core\n"
     "in vec4 fColor;"
     "out vec4 color;"
     "void main(){"
     "  color = fColor;"
     "}"});


    glGenVertexArrays(1,vao);

    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,
                          6*sizeof(GLfloat),(const void*)0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,
                          6*sizeof(GLfloat),(const void*)(2*sizeof(GLfloat)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    do{
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }while(glfwGetKey(window,GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           !glfwWindowShouldClose(window));

    glDeleteBuffers(1,vbo);
    glDeleteVertexArrays(1,vao);
    glfwDestroyWindow(window);
    glfwTerminate();
}
