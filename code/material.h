#ifndef MATERIAL_H
#define MATERIAL_H
#include <algorithm>
#include <list>
#include "ray.h"
#include "vector_helper.h"
#include "lightsource.h"
#include "scene.h"
#include "image.h"

class Material {
    private:
        bool e; //flag to check if material was initialised via the input json
        double ks;
        double kd; 
        int specularexponent;
        vector3 diffusecolor;
        bool useDiffuseTexture;
        Image diffTex;
        vector3 specularcolor;
        bool isreflective;
        double reflectivity;
        bool isrefractive;
        double refractiveindex;

    public:
        Material () { e = false; isreflective = false; isrefractive = false; reflectivity = 0; refractiveindex = 0; }
        Material (double s, double d, int se, vector3 dc, bool ut, Image dimg, vector3 sc, bool irl, double refl, bool irr, double refr) :
                 e(true), ks(s), kd(d), specularexponent(se), diffusecolor(dc), useDiffuseTexture(ut), diffTex(dimg),
                 specularcolor(sc), isreflective(irl), reflectivity(refl), isrefractive(irr), refractiveindex(refr) {};

        bool isReflective () { return isreflective; }
        double getReflectivity () { return reflectivity; }
        bool isRefractive () { return isrefractive; }
        double getRefIndex () { return refractiveindex; }
        bool exists () { return e; } //check if material was initialised properly
        bool hasTexture () { return useDiffuseTexture; }
        Image getTexture () { return diffTex; }
        vector3 getDiffuse () { return diffusecolor; }
        vector3 getSpecular () { return specularcolor; }

        //I don't know where else to put this tbh! Bit annoying as now I have to pass lights into the shape intersection thing but w/e
        //Should hopefully return the proper colour now !
        //TODO: set hit diffuse and specular tones seperately -> apply them independently in raytracer maybe?
        vector3 calculatePhongShading (Hit* hit, vector3 normal, vector3 intersectionPoint, std::list<LightSource*> l, vector3 camLook, vector3 texCoords) {
            vector3 intensity = {0,0,0};

            if (e) {
                //set hit properties; one recursive ray will be cast per true value.
                hit->setBounce(isreflective);
                hit->setRefract(isrefractive);

                for (LightSource* light : l) {
                    // Calculate the vectors needed for Phong shading
                    vector3 lightDirection = vectNormalize(light->getPosition() - intersectionPoint);
                    vector3 reflectionDirection = reflect(lightDirection, normal);

                    // Calculate the diffuse and specular components -> the float in diffuse max controls the harshness of the darkness spots!
                    // specular is currently a little incorrect for reflections
                    double diffuse = std::max(0.0, dotProduct(normal, lightDirection)) + (0.1 / l.size());
                    double specular = pow(std::max(0.0, dotProduct(reflectionDirection, camLook)), specularexponent); 

                    vector3 difCol = diffusecolor;
                    if (useDiffuseTexture) {
                        difCol = diffTex.getPixel(texCoords.x(), texCoords.y());
                    }
                
                    // Calculate the final intensity using Phong reflection model
                    intensity = intensity +
                                light->getIntensity() * kd * (diffuse * difCol) + 
                                light->getIntensity() * ks * (specular * specularcolor);
                }
            }

            //std::cout << intensity.x() << " : " << intensity.y() <<  " : "  << intensity.z() << std::endl;
            return vectClamp(intensity,0.0,1.0);
        }
};

#endif