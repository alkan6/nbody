#include "nbody.h"

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>

#define G float(6.674e-11)
static float t0;

struct IBody {
  Body *body;
  glm::vec3 pos,vel;
};

std::vector<Body*> univ;

void initNBody(int n)
{
    std::vector<Body*>::iterator it;
    it = univ.begin();
    while(it < univ.end()){
        delete *it;
    }
    univ.clear();

   univ.resize(n);

   for(int i=0;i<n;i++){
        Body *b = new Body;
        b->mass = 1;
        b->vel = glm::vec3(0,0,0);
        b->pos = glm::vec3(2.0f*float(rand())/float(RAND_MAX)-1.0f,
                           2.0f*float(rand())/float(RAND_MAX)-1.0f,
                           2.0f*float(rand())/float(RAND_MAX)-1.0f);
        b->r = std::pow(b->mass * 3.0f / 4.0f / M_PI, 1.0f/3.0f);
        //if(i%2==0) b->pos = glm::vec3(0.1,0.1,0.1);
        //if(i%2==1) b->pos = glm::vec3(0.9f,0.9f,0.9f);
        univ[i] = b;
    }
   t0 = 0.0f;
}

void joinNBody()
{
    std::vector<Body*>::iterator i,j;
    i = univ.begin();

    while(i < univ.end()-1){

        Body *b1 = *i;
        j = i+1;
        while(j<univ.end()){

            Body *b2 = *j;
            float d = glm::distance(b1->pos,b2->pos);
            if(d>(b1->r+b2->r)) {j++;continue;}

            float m = b1->mass + b2->mass;
            b1->vel = glm::sqrt((b1->mass * glm::exp2(b1->vel) + b2->mass * glm::exp2(b2->vel))/m);
            b1->pos = (b1->mass * b1->pos + b2->mass * b2->pos)/m;
            b1->r = std::pow(m * 3.0f / 4.0f / M_PI, 1.0f/3.0f);
            b1->mass = b1->mass + b2->mass;

            j = univ.erase(j);
            delete b2;
        }
        i++;
    }
}

void iterateNBody(float t)
{
    float dt = t - t0;
    t0 = t;

    std::vector<IBody> nuniv;
    std::vector<Body*>::iterator i,j;
    i = univ.begin();
    while(i < univ.end()){

        Body *b1 = *i;
        IBody ib;
        ib.body = b1;
        ib.vel = b1->vel;
        ib.pos = b1->pos;

        j = univ.begin();
        while(j<univ.end()){

            if(i==j) {j++;continue;}
            Body *b2 = *j;
            float d = glm::distance(ib.pos,b2->pos);
            glm::vec3 f = G * b1->mass * b2->mass * (b2->pos-ib.pos)/d/d/d;
            glm::vec3 a = f/b1->mass;
            ib.vel += a * dt;
            j++;
        }
        ib.pos += b1->vel * dt;

        nuniv.push_back(ib);
        i++;
    }
    std::vector<IBody>::iterator ii = nuniv.begin();
    while(ii < nuniv.end()){

        Body *b = ii->body;
        b->pos = ii->pos;
        b->vel = ii->vel;
        ii++;
        //std::cout << b << " " << b->pos << " " << b->vel.length() << std::endl;
    }
    nuniv.clear();
}
