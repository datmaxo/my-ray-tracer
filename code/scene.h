#ifndef SCENE_H
#define SCENE_H
#include <vector>
#include <list>
#include "shape.h"
#include "vector_helper.h"
#include "lightsource.h"
#include "acceleration hierarchy.h"

class Scene {
    private:
        vector3 bgcolour;
        std::list<LightSource*> lights;
        BVHNode* shapes;

    public:
        Scene () {}
        Scene (vector3 b, std::list<LightSource*> l, BVHNode* s) : bgcolour(b), lights(l), shapes(s) {}

        vector3 getBGColour () { return bgcolour; }
        BVHNode* getShapes () { return shapes; }
        std::list<LightSource*> getLights () { return lights; }
};

#endif