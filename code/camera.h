#ifndef CAMERA_H
#define CAMERA_H
#include <vector>

class Camera {
    protected:
        int width;
        int height;
        vector3 position;
        vector3 lookAt;
        vector3 upVector;
        double fov;
        double exposure;

    public:
        Camera () {}
        Camera (int w, int h, vector3 pos, vector3 look, vector3 up, double fieldov, double exp) {
            width = w;
            height = h;
            position = pos;
            lookAt = vector3 (look.x(), look.y(), look.z()); //used to have to invert y axis before i changed getLook()
            upVector = up;
            fov = fieldov * 1.5; //(13.0 / 9); //based on comparisons of my renders & the generic scene class, my fov needs scaling up from the json input
            exposure = exp;
        }

        virtual void example_void () {}

        int getWidth () { return width; }
        int getHeight () { return height; }
        double getFov () { return fov; }
        double getExposure () { return exposure; }
        vector3 getPosition () { return position; }
        vector3 getLook () { return vectNormalize(lookAt - position); }
        vector3 getCamUp () { return upVector; };
};

class PinholeCamera : public Camera {
    public:
        PinholeCamera (int w, int h, vector3 pos, vector3 look, vector3 up, double fieldov, double exp) :
        Camera(w, h, pos, look, up, fieldov, exp) {}
};

class ThinLens : public Camera {
    public:
        ThinLens (int w, int h, vector3 pos, vector3 look, vector3 up, double fieldov, double exp) :
        Camera(w, h, pos, look, up, fieldov, exp) {}
};

#endif