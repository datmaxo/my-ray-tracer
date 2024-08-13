#ifndef RAY_H
#define RAY_H

#include <vector>
#include <string>
#include <list>
#include "shape.h"
#include "material.h"
#include "vector_helper.h"
#include "lightsource.h"
#include "camera.h"

/* container class for information regarding the point along a ray where an intersection occured, the normal, etc.
probably stores too much information currently? */
class Hit {
    private:
        double t;
        vector3 hitPoint;
        vector3 hitNormal;
        vector3 colour;
        int hitShapeId;  //the id of the shape that created this Hit object
        double bounceMult;
        double refIndex;
        bool bounce;
        bool refract;
        bool boundingBox;
        int checks; //debugging acceleration only 

    public:
        Hit () { t = -1; checks = 0; hitShapeId = -1; }
        Hit (double ti, vector3 hit, vector3 norm, vector3 col, int id, double cm, double ri) : t(ti), hitPoint(hit), hitNormal(norm), colour(col), 
                                                                                hitShapeId(id), bounceMult(cm), refIndex(ri), bounce(false), refract(false),
                                                                                boundingBox(false), checks(0) {}
        ~Hit () { }

        double getT () const { return t; }
        vector3 getPoint () const { return hitPoint; }
        vector3 getNormal () const { return hitNormal; }
        vector3 getColour () const { return colour; }
        int getHitID () const { return hitShapeId; }
        double getReflectivity () const { return bounceMult; }
        double getRefractiveIndex() const { return refIndex; }
        bool getBounce () const { return bounce; }
        bool getRefract () const { return refract; }
        bool getBounding () const { return boundingBox; }
        int getChecks () const { return checks; }

        void setColour (vector3 c) { colour = c; }
        void setBounce (bool b) { bounce = b; }
        void setRefract (bool b) { refract = b; }
        void setBounding (bool b) { boundingBox = b; }
        void setChecks (int i) { checks = i; }
        void setID (int i) { hitShapeId = i; } //should not be used normally; good for debugging certain interactions tho!
};

/* a ray that is (typically) cast from a camera to detect objects in the scene.
 stores a reference to the most recently intersected object as lastHit. */
class Ray {
    private:
        vector3 origin;
        vector3 direction;
        vector3 colour;
        Hit* lastHit;

    public:
        Ray () {}
        Ray (vector3 o, vector3 d) : origin(o), direction(d), colour(vector3{1,0,0}) {}
        ~Ray () { }

        vector3 getOrigin () const { return origin; } 
        vector3 getDirection () const { return direction; }
        vector3 getColour () const { return colour; }
        //std::list<Hit*> getHits () const { return hits; }
        Hit* getLastHit () const { return lastHit; }

        //get the position at a given distance along a ray
        vector3 at (double pos) {
            return origin + (direction * pos);
        }

        void addHit (Hit* hit) { lastHit = hit; }
        //void clearHit () {  hits.clear(); } //given how the ray is redefined upon a bounce, this probably isn't needed
        void setColour (vector3 c) { colour = c; }
};

#endif