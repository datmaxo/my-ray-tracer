#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <vector>
#include <string>
#include <cmath>
#include <list>
#include <random>
#include <thread>
#include <chrono>
#include <ctime>
#include "vector_helper.h"
#include "scene.h"
#include "camera.h"
#include "ray.h"
#include "colours.h"
#include "acceleration hierarchy.h"

/*
==================================================================================================
Code needs to be pretty heavily restructured in order to get reflection working.
I'm not quite sure how to redo things at the moment.
I think rays need to cast through all objects individually
But that requires passing the scene through a lot of base classes - tried this and it's not fun!
The 'Hit' class would probably fix things; can store the intersection and colour for each object

Another problem is I currently need material to cast a ray and that's a problem.
Could change things up such that material changes the origin & direction of the ray, and somehow
restarts that rays trajectory.
Again could be solved with the hit class! If the closest hit has set 'bounce' to true or whatever
then we can alter the rays position and not increment the image.
Would require a while loop within the renderer as well.

Okay! Actually have a plan to get things moving forward now :) lets go
==================================================================================================
*/

class RayTracer {
    public:
        Camera cam;
        Scene scene;
        std::list<Ray> rays;
        std::list<Hit> hits;
        int bounces;
        std::string type;    //binary, phong, or pathfinder !
        double renderDistance;
        int totalObjs;

        RayTracer () {}
        RayTracer (Camera c, Scene s, std::list<Ray> r, std::list<Hit> h, int b, std::string t, int tO) : cam(c), scene(s), rays(r), hits(h), bounces(b), type(t), renderDistance(10.0), totalObjs(tO) {};
        ~RayTracer () {}

        Camera getCamera () const { return cam; }
        Scene getScene () const { return scene; }

        //typical rendering functions
        Image renderImage(int samples);
        Hit recursiveIntersect(Ray ray, vector3 look, std::list<Shape*> shapes);
        vector3 recursiveRaycast(Ray ray, vector3 look, int layer, bool lastWasRefract = false);
        double simpleShadow (Hit hit);

        //rendering with threading equations
        Image startThreadedRender(int samples, int threadCount);
        static void partialImageRenderer(RayTracer* r, Image* image, int samples, int rowID, int incr, int width, int pHeight, 
                                         int trueHeight, double hw, double hh, bool aliasing, vector3 camRight);
};

//recursively raytraces babey
//TODO: fix up refraction
vector3 RayTracer::recursiveRaycast (Ray ray, vector3 look, int bounce, bool lastWasRefract) {

    if (bounce == bounces) { 
        std::cout << "bounced outta the universe!" << std::endl;
        return {1,1,1}; 
    } //cap out the recursive bouncing once we hit the bounce limit

    Hit closestHit = intersectBVH(cam.getPosition(), ray, scene.getLights(), look, scene.getShapes(), true, true);
    //std::cout << closestHit.getChecks() << std::endl;

    vector3 reflectColour;
    vector3 refractColour;
    vector3 rayColour = scene.getBGColour();

    //if the ray hit an object, get the pixel colour from the closest one!
    if (closestHit.getT() > 0) {
        if (type == "binary") { 
            //std::cout << "binary test" << std::endl;
            return vector3{255, 0, 0}; 
        }
        else if (type == "phong") {
            //safeguard against infinite loops where rays intersect with the surface of their casted object
            vector3 pNorm = closestHit.getNormal();

            //'getBounce()' is the check for reflection; not the most inutitive call, I apologise !
            if (closestHit.getBounce() && closestHit.getPoint() != ray.getOrigin()) {
                vector3 newdir = vectNormalize(reflect(ray.getDirection(), closestHit.getNormal()));
                //vector3 pNorm = closestHit.getNormal(); //vectNormalize(closestHit.getNormal() - closestHit.getPoint());
                Ray r = Ray(closestHit.getPoint() + 0.0001 * newdir, newdir);
                reflectColour = recursiveRaycast(r, pNorm, bounce + 1);
            }

            //calculate diffuse, specular, and shadows; these will be overwritten if object is refractive
            double shadows = simpleShadow(closestHit);
            rayColour = closestHit.getColour();
            //if (closestHit.getHitID() == 2 && shadows < 1) { rayColour = vector3{0,0,1}; } //colour shadows on cylinder blue; for debug
            //std::cout << shadows << std::endl;

            //refraction currently kind of works? I'm not sure; there's not really a benchmark to test it against
            if (closestHit.getRefract()) {
                // Calculate the cosine of the angle between the incoming ray and the normal
                //std::cout << "refracting lol" << std::endl;
                vector3 dir = ray.getDirection();
                double cosTheta = dotProduct(dir, pNorm);
                double sinTheta = sqrt(1.0 - cosTheta*cosTheta);

                // Calculate the refracted direction using Snell's Law
                double refractiveIndexRatio = 1 / closestHit.getRefractiveIndex();
                if (lastWasRefract) { refractiveIndexRatio = closestHit.getRefractiveIndex(); }

                double discriminant = 1.0f - refractiveIndexRatio * refractiveIndexRatio * (1.0f - cosTheta * cosTheta);

                // Check for total internal reflection
                if (refractiveIndexRatio * sinTheta > 1.0) {
                    vector3 newdir = vectNormalize(reflect(ray.getDirection(), closestHit.getNormal()));
                    //vector3 pNorm = closestHit.getNormal(); //vectNormalize(closestHit.getNormal() - closestHit.getPoint());
                    Ray r = Ray(closestHit.getPoint() + 0.0001 * newdir, newdir);
                    rayColour = recursiveRaycast(r, pNorm, bounce + 1, true);
                }
                else {
                    vector3 newDir = refractiveIndexRatio * dir + (refractiveIndexRatio * cosTheta - sqrt(discriminant)) * pNorm;
                    Ray r = Ray(closestHit.getPoint() + 0.0001 * newDir, newDir);
                    rayColour = recursiveRaycast(r, pNorm, bounce + 1);
                    shadows = 1;
                }
            }

            int bounced = closestHit.getBounce();
            double rf = closestHit.getReflectivity();
            vector3 finalColour = shadows * (rayColour * (1 - std::pow(rf * bounced, 2))) + (reflectColour * std::pow(rf * bounced, 2));
            
            //LightSource* l = scene.getLights().front();
            //finalColour = vectNormalize(l->getPosition() - closestHit.getPoint()).absolute(); //debug; colour by normals
            //if (shadows < 1) {return {0,0,0};}

            return finalColour;
        }
    }
    return rayColour;
}

/*
Calculates whether a given pixel should be in shadow or not.
Currently ignores shadows on shapes caused by the same shape; caused visual issues when combined with phong shading.
TODO: introduce a more advanced shadow calculation method when moving forward with BDSF raytracing.
*/
double RayTracer::simpleShadow (Hit hit) {

    vector3 position = hit.getPoint();
    double shadowMultiplier = 1;
    int lightCount = scene.getLights().size();

    //should shadows ignore their own objects? tricky one
    for (LightSource* light : scene.getLights()) {
        vector3 lpos = light->getPosition();
        vector3 lightDir = vectNormalize(lpos - position);
        double distance = (lpos - position).magnitude();
        //std::cout << "Dist: " << distance << std::endl;
        Ray lightRay = Ray(position, lightDir);

        Hit closestHit = intersectBVH(cam.getPosition(), lightRay, scene.getLights(), cam.getLook(), scene.getShapes(), false);

        if (closestHit.getHitID() != hit.getHitID()) {
            if (closestHit.getT() > 0.0 && closestHit.getT() <= distance) { 
                shadowMultiplier -= 0.5 / lightCount; 
                if (hit.getHitID() == 99) {
                    std::cout << "hit; closest hit id: " << closestHit.getHitID() << std::endl;
                }
            }
            else { 
                if (hit.getHitID() == 99) {
                    std::cout << "miss 1; closest hit id: " << closestHit.getHitID() << std::endl;
                }
            } 
        }
        else {
            if (hit.getHitID() == 99) {
                std::cout << "miss 2; closest hit id: " << closestHit.getHitID() << std::endl;
            }
        }
    }

    //max filter the shadows to soften them; stops them from ever appearing 100% dark
    return shadowMultiplier;
};

//Somehow this works and it's beautiful
Image RayTracer::startThreadedRender(int samples, int threadCount) {
        
    //Initialise renderer - this step is exactly the same as renderImage();
    int imageWidth = cam.getWidth();
    int imageHeight = cam.getHeight();
    double aspectRatio = ((double) imageWidth) / (double (imageHeight));
    double halfWidth = std::tan( (cam.getFov() / 2.0) * (M_PI / 180) );
    double halfHeight = halfWidth / aspectRatio;
    vector3 cameraRight = vectNormalize(crossProduct(cam.getLook(), cam.getCamUp()) * -1);
    if (type == "binary") {samples = 1;} //keep binary rendering rough (jagged edges) but fast
    double linearToneMapScale = 1.25; //the scale for linear tone mapping
    bool antiAliasing = true;
    if (samples < 1) { //if samples is 0 or less, no anti-aliasing will be applied
        samples = 1;
        antiAliasing = false;
    }
    Image image = Image(imageWidth, imageHeight);

    Image* partials[threadCount];
    std::vector<std::thread> threads;
    auto start = std::chrono::system_clock::now();

    //Launch the threads
    for (int i = 0; i < threadCount; i++) {
        partials[i] = new Image(imageWidth, imageHeight / threadCount);
        threads.emplace_back(&partialImageRenderer, this, partials[i], samples, i, threadCount, imageWidth, imageHeight / threadCount, 
                             imageHeight, halfWidth, halfHeight, antiAliasing, cameraRight);
    }

    // Join threads
    for (auto& thread : threads) {
        thread.join();
    }

    // build full composite render from the threaded images
    for (int y = 0; y < imageHeight; y++) {
        Image* ref = partials[y % threadCount];
        for (int x = 0; x < imageWidth; x++) {
            vector3 pixl = ref->getPixel(x, y / threadCount);
            image.setPixel(x,y,pixl);
        }
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout << "Render Complete!\nElapsed time : " << elapsed_seconds.count() << "s" << std::endl;

    return image;
} 

//assembles an image the same width as the final render, but with 1/threadCount rows
//every row in the partial image is a rowID % threadCount row in the final image
void RayTracer::partialImageRenderer(RayTracer* r, Image* image, int samples, int rowID, int incr, int width, int pHeight, int trueHeight, double hw, double hh, bool aliasing, vector3 camRight) {
    vector3 origin = r->cam.getPosition();

    for (int y = rowID; y < trueHeight; y += incr) {
        //std::cout << rowID << "] Scanlines remaining: " << (pHeight - (y / incr)) << std::endl;
        for (int x = 0; x < width; x++) {

            vector3 colour;

            //antialiasing: cast a ray for as mamy samples as there are, set final colour to the mean rgb values
            for (int i = 0; i < samples; i++) {

                double rayX = x;
                double rayY = y;
                if (aliasing) {
                    rayX += 0.5f + (static_cast<double>(rand()) / RAND_MAX - 0.5f);
                    rayY += 0.5f + (static_cast<double>(rand()) / RAND_MAX - 0.5f);
                }
            
                double screenX = (2.0f * (rayX + 0.5f) / width - 1.0f) * hw;
                double screenY = (1.0f - 2.0f * (rayY + 0.5f) / trueHeight) * hh;

                vector3 xVect = camRight * screenX;
                vector3 yVect = r->cam.getCamUp() * screenY;
                vector3 dir = vectNormalize(r->cam.getLook() + xVect + yVect);

                vector3 pNorm = r->cam.getLook();
                Ray ray = Ray(origin, dir);
                vector3 c = r->recursiveRaycast(ray, r->cam.getLook(), 0);

                colour = colour + c;
                //rays.push_back(r);  // -> don't need atm
            }

            colour = colour / samples;
            colour = vectClamp(colour * 1.25, 0.0, 1.0); ; //linear tone mapping; gamma also implemented
            colour = increaseSaturation(colour, 0.5); //gpt generated script to make colours pop better!
            image->setPixel(x, y / incr, convertColourVector(colour.toStdVector()));
        }
    }
} 

//Original rendering function; as far as I am aware this has been fully replaced with startThreadedRender, and is hence depreciated.
/*Image RayTracer::renderImage(int samples) {
    int imageWidth = cam.getWidth();
    int imageHeight = cam.getHeight();
    double aspectRatio = ((double) imageWidth) / (double (imageHeight));
    double halfWidth = std::tan( (cam.getFov() / 2.0) * (M_PI / 180) );
    double halfHeight = halfWidth / aspectRatio;
    //cam right is inverted because currently all -values appear right-wards and vis versa
    vector3 cameraRight = vectNormalize(crossProduct(cam.getLook(), cam.getCamUp()) * -1);
    if (type == "binary") {samples = 1;} //keep binary rendering rough (jagged edges) but fast
    
    //Variables for experimenting with during development! Don't really need to be taken as inputs
    double linearToneMapScale = 1.25; //the scale for linear tone mapping
    bool antiAliasing = true;
    if (samples < 1) { //if samples is 0 or less, no anti-aliasing will be applied
        samples = 1;
        antiAliasing = false;
    }

    //gpt didn't give an origin for the rays; for now I will assume that the camera position will suffice
    vector3 origin = cam.getPosition();

    Image image = Image(imageWidth, imageHeight);

    //incredibly inefficient but oh boy this actually renders spheres! progress baby!
    for (int y = 0; y < imageHeight; y++) {
        std::clog << "\rScanlines remaining: " << (imageHeight - y) << ' ' << std::flush; //stolen from the tutorial hehe
        for (int x = 0; x < imageWidth; x++) {

            vector3 colour;

            //antialiasing: cast a ray for as mamy samples as there are, set final colour to the mean rgb values
            for (int i = 0; i < samples; i++) {

                double rayX = x;
                double rayY = y;
                if (antiAliasing) {
                    rayX += 0.5f + (static_cast<double>(rand()) / RAND_MAX - 0.5f);
                    rayY += 0.5f + (static_cast<double>(rand()) / RAND_MAX - 0.5f);
                }
            
                double screenX = (2.0f * (rayX + 0.5f) / imageWidth - 1.0f) * halfWidth;
                double screenY = (1.0f - 2.0f * (rayY + 0.5f) / imageHeight) * halfHeight;

                vector3 xVect = cameraRight * screenX;
                vector3 yVect = cam.getCamUp() * screenY;
                vector3 dir = vectNormalize(cam.getLook() + xVect + yVect);

                vector3 pNorm = cam.getLook();
                Ray r = Ray(origin, dir);
                vector3 c = recursiveRaycast(r, cam.getLook(), 0);

                colour = colour + c;
                //rays.push_back(r);  // -> don't need atm
            }

            colour = colour / samples;
            colour = vectClamp(colour * linearToneMapScale, 0.0, 1.0); ; //linear tone mapping; gamma also implemented
            colour = increaseSaturation(colour, 0.5); //gpt generated script to make colours pop better!
            image.setPixel(x, y, convertColourVector(colour.toStdVector()));
        }
    }

    return image;
}*/ 

#endif