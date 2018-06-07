#ifndef NBODY_H
#define NBODY_H

#include <glm/vec3.hpp>

typedef struct {
    glm::vec3 pos;
    glm::vec3 vel;
    float mass;
    float r;
 } Body;

void initNBody(int n);
void joinNBody();
void iterateNBody(float dt);

#endif // NBODY_H
