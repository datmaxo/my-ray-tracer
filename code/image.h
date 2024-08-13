#ifndef IMAGE_H
#define IMAGE_H
#include <algorithm>
#include <list>
#include "vector_helper.h"

//Don't want to have to load and maintain big vectors as they are stinky and slow and can't be indexed as easily as arrays!
//Hence; custom image type babey >:)

class Image {
    private:
        int width;
        int height;
        vector3** img;

    public:
        Image () : width(-1), height(-1) {}
        Image (int w, int h) : width(w), height(h) { //not the most efficient constructor but I split it apart for comprehensibility
            
            //create image dimensionality
            img = new vector3*[width];
            for (int i = 0; i < width; i++) {
                img[i] = new vector3[height];
            }

            //populate image with black pixels
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    img[x][y] = {0,0,0};
                }
            }
        }

        int getWidth () { return width; }
        int getHeight () { return height; }
        vector3 getPixel (int x, int y) {
            //std::cout << "Getting colour at pixel ["<< x << " , " << y << "]" << std::endl;
            x = std::max(0, std::min(x, width - 1));
            y = std::max(0, std::min(y, height - 1));
            return img[x][y];
        }

        void setPixel (int x, int y, vector3 colour) {
            img[x][y] = colour;
        }
};

#endif