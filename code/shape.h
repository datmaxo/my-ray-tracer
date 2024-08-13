#ifndef SHAPE_H
#define SHAPE_H
#include <string>
#include <list>
#include <cmath>
#include <algorithm>
#include "vector_helper.h"
#include "lightsource.h"
#include "material.h"
#include "ray.h"

class Shape {
    protected:
        int id; //a unique identifier used to refer to the shape intersected by a Hit
        Material material;

    public:
        Shape () { material = Material(); id = 10000; }
        Shape (Material m, int ID) : material(m), id(ID) {}
        ~Shape () { }

        virtual Hit intersect (Ray& ray, std::list<LightSource*> l, vector3 camLook, bool calcMaterial = true) { return Hit(); }
        virtual vector3 mapTexture (Ray& ray, vector3 hit, vector3 hitNormal = {0,0,0}) { return {0,0,0}; }

        virtual vector3 getCenter () { return {0,0,0}; }
        virtual vector3 getMinimums () { return {0,0,0}; }
        virtual vector3 getMaximums () { return {0,0,0}; }
        virtual std::string getType () { return "null"; }
};

class Sphere : public Shape {
    private:
        vector3 center;
        double radius;

    public:
        Sphere (vector3 c, double r, Material m, int id) : Shape(m, id), center(c), radius(r) {}

        vector3 getCenter () override { return center; }
        double getRadius () const { return radius; }
        std::string getType () override { return "sphere"; }

        // Function to calculate the intersection points with a sphere
        Hit intersect (Ray& ray, std::list<LightSource*> l, vector3 camLook, bool calcMaterial = true) override {
            double threshold = 0;
            if (!calcMaterial) {threshold = 1e-4;}
            
            vector3 OC = ray.getOrigin() - center;
            float a = dotProduct(ray.getDirection(), ray.getDirection());
            float b = 2.0f * dotProduct(OC, ray.getDirection());
            float c = dotProduct(OC, OC) - radius * radius;
            float discriminant = b * b - 4 * a * c;

            if (discriminant < 0) {
                // No intersection
                return Hit();
            } else {

                //can get the intersection points if we need them later
                double t1 = (-b - std::sqrt(discriminant)) / (2.0f * a);
                double t2 = (-b + std::sqrt(discriminant)) / (2.0f * a);
                double t = t1;
                if ((t2 < t1 && t2 >= threshold) || (t1 < threshold)) {t = t2;}
                if (t < threshold) {return Hit();} //best intersection is behind the camera; nevermind we can't see this boy!

                //calculate normal vector
                vector3 norm = vectNormalize(ray.at(t) - center);
                vector3 colourNorm = 0.5 * (norm + vector3{1,1,1});
                Hit* hit = new Hit(t, ray.at(t), norm, vector3{0,0,0}, id, material.getReflectivity(), material.getRefIndex());
                vector3 textureMap = mapTexture(ray, norm);

                if (calcMaterial) {
                    if (material.exists()) {
                        hit->setColour(material.calculatePhongShading(hit, norm, ray.at(t), l, camLook, textureMap));
                    } else {
                        hit->setColour(colourNorm); //colour with normals
                    }
                }

                return *hit;
            }
        }

        //calculate position on texture file to get, if texture file is present in material
        vector3 mapTexture(Ray& ray, vector3 hit, vector3 hitNormal = {0,0,0}) override {
            if (!material.hasTexture()) { return {0,0,0}; }

            float scaleFactor = 0.5f; //2.0f / (1.0f + pos.z()); // Scaling factor
            vector3 pos = vectNormalize(hit); //ensure normalisation
            Image diffTex = material.getTexture();

            // Projected coordinates on the 2D image
            double xImage = ((clamp(pos.x(), -1, 1)  * scaleFactor) * diffTex.getWidth())  + (diffTex.getWidth() / 2);  //* 0.5f * diffTex.getWidth();
            double yImage = ((clamp(-pos.y(), -1, 1) * scaleFactor) * diffTex.getHeight()) + (diffTex.getHeight() / 2); //* 0.5f * diffTex.getHeight();

            return {xImage, yImage, 0};
        }

        //estimate the minimum / maximum points of a bounding box surrounding this geometry
        //used for acelleration hiercarchy box calculation
        vector3 getMinimums () override { return {center.x() - radius, center.y() - radius, center.z() - radius}; }
        vector3 getMaximums () override { return {center.x() + radius, center.y() + radius, center.z() + radius}; }
};

class Cylinder : public Shape {
    private:
        vector3 center;
        vector3 axis;
        double radius;
        double height;

    public:
        Cylinder (vector3 c, vector3 a, double r, double h, Material m, int id) : Shape(m, id), center(c), axis(a), radius(r), height(h) {}

        vector3 getCenter () override { return center; }
        std::string getType () override { return "cylinder"; }

        //After weeks this bad boy finally renders correctly
        Hit intersect(Ray& ray, std::list<LightSource*> l, vector3 camLook, bool calcMaterial = true) override {
            double threshold = 0;
            if (!calcMaterial) {threshold = 1e-4;}
            
            //i was having issues where the axis translated the cylinder
            //thus, this counteracts that by translating the cylinder by -axis prior to calculation
            vector3 tO = (ray.getOrigin() - axis);
            vector3 tC = (center - axis);

            vector3 OC = tO - tC;
            vector3 dir = ray.getDirection();
            
            vector3 p1 = dir - (axis * dotProduct(dir, axis));
            vector3 p2 = OC - (axis * dotProduct(OC, axis));
            double a = dotProduct(p1 , p1);
            double b = 2.0f * dotProduct(p2, p1);
            double c = dotProduct(p2, p2) - radius * radius;
        
            double discriminant = b * b - 4 * a * c;
        
            if (discriminant < 0) {
                // No intersection with the main body of the cylinder -> check the caps!
                return capCheck(ray, l, camLook);
            }
        
            // Calculate the t values for potential intersections
            double t1 = (-b - std::sqrt(discriminant)) / (2 * a);
            double t2 = (-b + std::sqrt(discriminant)) / (2 * a);
            double t = t1;
            if ((t2 < t1 && t2 >= threshold) || (t1 < threshold)) {t = t2;}

            //best intersection is behind the camera; nevermind we can't see this boy! 
            if (t < threshold) {
                return Hit();
            }

            //calculating normal vectors for a cylinder is a little different - thank u gpt :)
            vector3 vectorToSurface = ray.at(t) - center;
            vector3 axisComponent = dotProduct(vectorToSurface, axis) * axis;
            vector3 radialComponent = vectorToSurface - axisComponent;
            vector3 norm  = /*-*/vectNormalize(radialComponent); //gpt inverted this for some reason lol
            Hit* hit = new Hit(t, ray.at(t), norm, vector3{0,0,0}, id, material.getReflectivity(), material.getRefIndex());
            vector3 textureMap = mapTexture(ray, ray.at(t), norm);

            if (calcMaterial) {
                vector3 colourNorm = 0.5 * (norm + vector3{1,1,1});
                if (material.exists()) {
                    hit->setColour(material.calculatePhongShading(hit, norm, ray.at(t), l, camLook, textureMap));
                } else {
                    hit->setColour(colourNorm); //colour with normals
                }
            }
        
            // Check if the intersection point is within the height of the cylinder
            double z = dotProduct(tO - tC, axis) + t * dotProduct(dir, axis);
        
            //if these aren't divided by 2 I'm pretty sure height works the same way as the json files define it
            if (z >= -height && z <= height) {
                return *hit;
            }
            
            ray.setColour(vector3{1,0,0});
            return capCheck(ray, l, camLook);

        }

        //texture mapper for the main body of the cylinder
        vector3 mapTexture(Ray& ray, vector3 hit, vector3 hitNormal = {0,0,0}) override {
            if (!material.hasTexture()) { return {0,0,0}; }

            float scaleFactor = 0.5; //2.0f / (1.0f + pos.z()); // Scaling factor
            vector3 pos = hit - getCenter(); //ensure normalisation
            Image diffTex = material.getTexture();

            vector3 axisPair = {0,2,1}; //atr[] indexes relating to relative axis of the circular cap
            if (axis.x() == 1) { axisPair = {2,1,0}; }
            else if (axis.z() == 1) { axisPair = {0,1,2}; }
            int x = (int) axisPair.x();
            int y = (int) axisPair.y();
            int z = (int) axisPair.z();

            double theta = std::atan2(pos.atr[y], pos.atr[x]);
            double normalizedTheta = (theta + M_PI) / (2 * M_PI);  // Normalize theta to [0, 1]

            // Map cylindrical coordinates to texture coordinates
            double xImage = normalizedTheta * diffTex.getWidth();
            double yImage = ((-pos.atr[z] + height) / height / 2) * diffTex.getHeight();

            return {xImage, yImage, 0};
        }

        //if ray does not interact with the main part of the cylinder, check the caps!
        Hit capCheck(Ray& ray, std::list<LightSource*> l, vector3 camLook, bool calcMaterial = true) {
            vector3 origin = ray.getOrigin();
            vector3 direction = ray.getDirection();
            double threshold = 0;
            if (!calcMaterial) {threshold = 1e-4;}

            double top = dotProduct((center + (height) * axis - origin), axis) / dotProduct(direction,axis);
            double bot = dotProduct((center + (height) * -axis - origin), axis) / dotProduct(direction,axis);
            double t = -1;
            vector3 vec = {0,0,0};
            Hit* hit = new Hit(); 

            vector3 textureMap;
            if (std::pow((ray.at(top) - center - (height) * axis).magnitude(), 2) <= radius * radius + threshold) {
                hit = new Hit(top, ray.at(top), axis, vector3{0,0,0}, id, material.getReflectivity(), material.getRefIndex());
                textureMap = mapCapTexture(ray, ray.at(top), axis);
                vec = axis;
                t = top;
            }
            else if (std::pow((ray.at(bot) - center - (height) * -axis).magnitude(), 2) <= radius * radius + threshold) {
                hit = new Hit(bot, ray.at(bot), -axis, vector3{0,0,0}, id, material.getReflectivity(), material.getRefIndex());
                textureMap = mapCapTexture(ray, ray.at(bot), -axis);
                vec = -axis;
                t = bot;
            }
            else { return Hit(); }

            if (calcMaterial) {
                vector3 colourNorm = 0.5 * (axis + vector3{1,1,1});
                if (material.exists()) {
                    hit->setColour(material.calculatePhongShading(hit, vec, ray.at(t), l, camLook, textureMap));
                } else {
                    hit->setColour(colourNorm); //colour with normals
                }
            }

            //debugging bad shadows
            //std::cout << "Ray hit cap at: " << hit->getPoint() << std::endl;
            hit->setColour(vector3{1,0,0});
            hit->setID(99);

            return *hit;
        }

        //estimate the minimum / maximum points of a bounding box surrounding this geometry
        //used for acelleration hiercarchy box calculation
        vector3 getMinimums () override {
            double x = std::max(radius, axis.x() * (height));
            double y = std::max(radius, axis.y() * (height));
            double z = std::max(radius, axis.z() * (height));
            return {center.x() - x, center.y() - y, center.z() - z}; 
        }
        vector3 getMaximums () override {
            double x = std::max(radius, axis.x() * (height));
            double y = std::max(radius, axis.y() * (height));
            double z = std::max(radius, axis.z() * (height));
            return {center.x() + x, center.y() + y, center.z() + z}; 
        }

        //texture mapper for the caps of the cylinders; 
        vector3 mapCapTexture (Ray& ray, vector3 hit, vector3 normal) {
            if (!material.hasTexture()) { return {0,0,0}; }
            Image tex = material.getTexture();

            //Nothing fancy, I'm just gonna map the normalised distance from the centre
            vector3 axisPair = {0,1,-1}; //atr[] indexes relating to relative axis of the circular cap
            if (axis.x() == 1) { axisPair = {2,1,-1}; }
            else if (axis.y() == 1) { axisPair = {0,2,-1}; }
            int x = (int) axisPair.x();
            int y = (int) axisPair.y();
        
            vector3 circCenter = getCenter() + (normal * height / 2); //normal is just axis, pos or neg depending on cap hit
            vector3 relHit = vectNormalize(hit - circCenter);

            double xImage = fmod((relHit.atr[x] * 0.5) * tex.getWidth()  + (tex.getWidth() / 2), tex.getWidth());  
            double yImage = fmod((relHit.atr[y] * 0.5) * tex.getHeight() + (tex.getHeight() / 2), tex.getHeight()); 

            return {xImage,yImage,0};
        }
};

class Triangle : public Shape {
    private:
        vector3 v0;
        vector3 v1;
        vector3 v2;

    public:
        Triangle (vector3 z, vector3 o, vector3 t, Material m, int id) : Shape(m, id), v0(z), v1(o), v2(t) {}

        std::string getExistance();
        vector3 getCenter () override { 
            double x = (v0.x() + v1.x() + v2.x()) / 3;
            double y = (v0.y() + v1.y() + v2.y()) / 3;
            double z = (v0.z() + v1.z() + v2.z()) / 3;
            return {x,y,z};
        }
        std::string getType () override { return "tri"; }

        Hit intersect (Ray& ray, std::list<LightSource*> l, vector3 camLook, bool calcMaterial = true) override {
            double threshold = 0;
            if (!calcMaterial) {threshold = 1e-4;}

            // Calculate the normal to the triangle
            vector3 normal = crossProduct(v1 - v0, v2 - v0);
            //std::cout << normal.x() << " : " << normal.y() <<  " : "  << normal.z() << std::endl;

            // Check if the ray is parallel to the triangle (dot product with the normal is close to zero)
            double dotProd = dotProduct(ray.getDirection(), normal);
            if (std::abs(dotProd) < 1e-6) {
                // Ray is parallel to the triangle, no intersection
                return Hit();
            }

            // Calculate the distance along the ray where it intersects the plane of the triangle
            double t = dotProduct(v0 - ray.getOrigin(), normal) / dotProd;

            if (t < threshold) { return Hit(); } //intersection point is behind the camera! silly silly
            vector3 colour;

            // Calculate the intersection point
            vector3 intersectionPoint = ray.at(t);
            Hit* hit = new Hit(t, intersectionPoint, vectNormalize(normal), vector3{0,0,0}, id, material.getReflectivity(), material.getRefIndex());

            // Check if the intersection point is inside the triangle
            vector3 edge1 = v1 - v0;
            vector3 edge2 = v2 - v1;
            vector3 edge3 = v0 - v2;

            vector3 normal1 = crossProduct(edge1, intersectionPoint - v0);
            vector3 normal2 = crossProduct(edge2, intersectionPoint - v1);
            vector3 normal3 = crossProduct(edge3, intersectionPoint - v2);

            // Check if the intersection point is on the same side of each triangle edge
            if (dotProduct(normal1, normal) >= 0.0f && dotProduct(normal2, normal) >= 0.0f && dotProduct(normal3, normal) >= 0.0f) {
                vector3 textMap = mapTexture(ray, intersectionPoint, vectNormalize(normal));

                // Colour the hit now that we know the triangle has been hit
                // vector3 textureMap = mapTexture(ray, intersectionPoint, vectNormalize(normal));
                if (calcMaterial) {
                    vector3 colourNorm = 0.5 * (vectNormalize(normal) + vector3{1,1,1});
                    if (material.exists()) {
                        hit->setColour(material.calculatePhongShading(hit, vectNormalize(normal), intersectionPoint, l, camLook, textMap));
                    } else {
                        hit->setColour(colourNorm); //colour with normals
                    }
                }

                return *hit;
            }

            // Intersection point is outside the triangle
            return Hit();
        }

        //estimate the minimum / maximum points of a bounding box surrounding this geometry
        //used for acelleration hiercarchy box calculation
        vector3 getMinimums () override {
            double x = std::min(v0.x(), std::min(v1.x(), v2.x()));
            double y = std::min(v0.y(), std::min(v1.y(), v2.y()));
            double z = std::min(v0.z(), std::min(v1.z(), v2.z()));
            return {x, y, z}; 
        }
        vector3 getMaximums () override {
            double x = std::max(v0.x(), std::max(v1.x(), v2.x()));
            double y = std::max(v0.y(), std::max(v1.y(), v2.y()));
            double z = std::max(v0.z(), std::max(v1.z(), v2.z()));
            return {x, y, z}; 
        }

        //Triangle texture mapper -> maps texture to the plane of the triangle
        //GPT couldn't give me anything good, had to scour stack overflow for transformation maths for a while -
        //shout outs to valdo for the working code: https://stackoverflow.com/a/9605748
        vector3 mapTexture (Ray& ray, vector3 hit, vector3 normal) override {
            if (!material.hasTexture()) { return {0,0,0}; }
            Image tex = material.getTexture();

            vector3 min = getMinimums();
            vector3 max = getMaximums();
            vector3 xaxis = vectNormalize({max.x() - min.x(), min.y(), max.z() - min.z()});

            double localX = dotProduct((hit - getCenter()), xaxis);
            double localY = dotProduct((hit - getCenter()), crossProduct(normal, xaxis));
            double xImage = fmod((-localX * (1 / xaxis.magnitude())) * tex.getWidth()  + (tex.getWidth() / 2), tex.getWidth());  
            double yImage = fmod((localY * (1 / crossProduct(normal, xaxis).magnitude())) * tex.getHeight() + (tex.getHeight() / 2), tex.getHeight()); 

            return {xImage,yImage,0};
        }
};

/*
Cubes are currently only used for the acceleration hierarchy; they never concretely exist in the scene.
As such, it is impossible to instantiate one with a material or an ID.
*/
class Cube : public Shape {
    private:
        vector3 cubeMin;
        vector3 cubeMax;

    public:
        Cube () {};
        Cube (vector3 mi, vector3 ma) : Shape(Material(), -1), cubeMin(mi), cubeMax(ma) { }
        Cube (std::vector<Shape*> shapes, vector3 cam) {
            Shape(Material());
            cubeMax = {-INFINITY, -INFINITY, -INFINITY};
            cubeMin = {INFINITY, INFINITY, INFINITY};

            for (Shape* s : shapes) {
                vector3 mins = s->getMinimums();
                cubeMin = {std::min(cubeMin.x(), mins.x()), std::min(cubeMin.y(), mins.y()), std::min(cubeMin.z(), mins.z())};
                vector3 maxs = s->getMaximums();
                cubeMax = {std::max(cubeMax.x(), maxs.x()), std::max(cubeMax.y(), maxs.y()), std::max(cubeMax.z(), maxs.z())};
            }

            reshapeIfOnBoundary(cam);

            //std::cout << "New bounding box:" << std::endl;
            //std::cout << " * Min -> " << cubeMin.x() << " : " << cubeMin.y() <<  " : "  << cubeMin.z() << std::endl;
            //std::cout << " * Max -> " << cubeMax.x() << " : " << cubeMax.y() <<  " : "  << cubeMax.z() << std::endl;

        }
        Cube (Shape* s1, vector3 cam) {
            Shape(Material());

            cubeMin = {-99,-99,-99};
            cubeMax = {-99,-99,-99};
            if (s1->getType() != "null") {
                cubeMin = s1->getMinimums();
                cubeMax = s1->getMaximums();
            }

            reshapeIfOnBoundary(cam);

            //std::cout << "New bounding box [ contains " << s1->getType() << " ] :" << std::endl;
            //std::cout << " * Min -> " << cubeMin.x() << " : " << cubeMin.y() <<  " : "  << cubeMin.z() << std::endl;
            //std::cout << " * Max -> " << cubeMax.x() << " : " << cubeMax.y() <<  " : "  << cubeMax.z() << std::endl;
        }

        Hit intersect (Ray& ray, std::list<LightSource*> l, vector3 camLook, bool calcMaterial = false) override {
            // Calculate the inverse direction of the ray
            vector3 dir = ray.getDirection();
            vector3 origin = ray.getOrigin();
            vector3 invDirection(1.0f / dir.x(), 1.0f / dir.y(), 1.0f / dir.z());
            double rayMin = -INFINITY;
            double rayMax = INFINITY;

            //credit for this skin-saving optimised aabb loop is Pixar engineer Andrew Kensler
            //found in the book 'Raytracing: the next week' (Shirley, Black, Hollasch) - chapter 3.5
            for (int a = 0; a < 3; a++) {
                auto invD = 1 / ray.getDirection().atr[a];
                auto orig = ray.getOrigin().atr[a];

                auto t0 = (cubeMin.atr[a] - orig) * invD;
                auto t1 = (cubeMax.atr[a] - orig) * invD;

                if (invD < 0)
                    std::swap(t0, t1);

                if (t0 > rayMin) rayMin = t0;
                if (t1 < rayMax) rayMax = t1;

                if (rayMax <= rayMin)
                    return Hit();
            }

            // return a dummy hit; hopefully doesn't matter since cubes are just used as bounding volumes
            Hit h = Hit(1, {0,0,0}, {0,0,0}, {0,0,0}, 10001, false, false);
            h.setBounding(true);
            return h;
        }

        vector3 getMinimums () override { return cubeMin; }
        vector3 getMaximums () override { return cubeMax; }
        vector3 getCenter () override { 
            double x = (cubeMin.x() + cubeMax.x()) / 2;
            double y = (cubeMin.y() + cubeMax.y()) / 2;
            double z = (cubeMin.z() + cubeMax.z()) / 2;
            return {x,y,z};
        }
        std::string getType () override { return "cube"; }

        double x () const { return cubeMax.x() - cubeMin.x(); }
        double y () const { return cubeMax.y() - cubeMin.y(); }
        double z () const { return cubeMax.z() - cubeMin.z(); }

        int longestAxis () {
            double x = (cubeMax.x() - cubeMin.x());
            double y = (cubeMax.y() - cubeMin.y());
            double z = (cubeMax.z() - cubeMin.z());
            if (x >= y && x >= z) { return 0; }
            else if (y >= x && y >= z) { return 1; }
            return 2;
        }

        //slightly increase the size of the bounding box if the point in question is on an exact edge
        //being on the exact edge of a box leads to weird behaviour; camera won't render shapes within the boundary of the box,
        //and shadows will neither render if they are on a boundary (just a problem for triangles)
        void reshapeIfOnBoundary (vector3 camPos) {
            if (camPos.y() == cubeMin.y() || camPos.y() == cubeMax.y()) {
                cubeMin.atr[1] -= 0.001;
                cubeMax.atr[1] += 0.001;
            }
            if (camPos.z() == cubeMin.z() || camPos.z() == cubeMax.z()) {
                cubeMin.atr[2] -= 0.001;
                cubeMax.atr[2] += 0.001;
            }
            if (camPos.x() == cubeMin.x() || camPos.x() == cubeMax.x()) {
                cubeMin.atr[0] -= 0.001;
                cubeMax.atr[0] += 0.001;
            }
        }
};

std::string Triangle::getExistance()
{
    return "I'm alive babey!";
}

#endif