#ifndef LIGHTSOURCE_H
#define LIGHTSOURCE_H
#include "vector_helper.h"

/*
==================================================================================================
Lights!
This script basically just defines the class; light is actualised in other parts of the pipeline.

TODO: currently, point and area lights are the same thing. 
lighting system should be reworked to allow for many types of lighting.
also; add more lighting types?
==================================================================================================
*/

class LightSource {
    protected:
        vector3 position;
        vector3 intensity;
        //colour maybe? probably too crazy
        
    public:
        LightSource () {}
        LightSource (vector3 p, vector3 d) : position(p), intensity(d) {}

        vector3 getPosition () { return position; }
        vector3 getIntensity () { return intensity; }
};

class PointLight : public LightSource {
    public:
        PointLight (vector3 p, vector3 d) : LightSource(p, d) {};
};

class AreaLight : public LightSource {
    public:
        AreaLight (vector3 p, vector3 d) : LightSource(p, d) {};
};

#endif