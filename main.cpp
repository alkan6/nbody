#include "nbody.h"
#include "glview.h"

int main(int argc, char **argv)
{
    initNBody(500);
    return initGLView();
}
