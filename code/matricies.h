#ifndef MATRICIES_H
#define MATRICIES_H

#include <iostream>
#include <vector>
#include <array>
#include "vector_helper.h"

/*
==============================================================================================================
Oh em gee more custom types! Life is crazy and wild man.
Defining matrix4 in terms of vector4s makes it much more managable imo.
Crucial to keep in mind that vectors correspond to ROWS of the matrix; made implementation slightly easier!

This script was created for persective mapping, which is unimplemented in the renderer.
Hence, this file is currently deprecated; should possibly be removed entirely?
==============================================================================================================
*/

class vector4 {
    public:
        double atr[4];
        vector4 () : atr{0,0,0,0} {}
        vector4 (double x, double y, double z, double w) : atr{x,y,z,w} {}
        vector4 (vector3 v, double w) : atr{v.x(),v.y(),v.z(), w} {}
        vector4 (double ds[4]) : atr{ds[0], ds[1], ds[2], ds[3]} {}

        double x () const { return atr[0]; }
        double y () const { return atr[1]; }
        double z () const { return atr[2]; }
        double w () const { return atr[3]; }
};

class matrix4 {
    public:
        vector4 atr[4];
        matrix4 () : atr{vector4(),vector4(),vector4(),vector4()} {}
        matrix4 (double d1[4], double d2[4], double d3[4], double d4[4]) : atr{vector4(d1),vector4(d2),vector4(d3),vector4(d4)} {}
        matrix4 (vector4 d1, vector4 d2, vector4 d3, vector4 d4) : atr{d1, d2, d3, d4} {}
};

//multiplies each dimension of input vector by a constant factor
vector4 operator *(const vector4& v1, const vector4& v2) {
    return vector4(v1.x() * v2.x(), v1.y() * v2.y(), v1.z() * v2.z(), v1.w() * v2.w());
}

//multiplies each dimension of input vector by a constant factor
vector4 operator *(const matrix4 m, const vector4& v) {
    double x = (m.atr[0].x() * v.x()) + (m.atr[0].y() * v.y()) + (m.atr[0].z() * v.z()) + (m.atr[0].w() * v.w());
    double y = (m.atr[1].x() * v.x()) + (m.atr[1].y() * v.y()) + (m.atr[1].z() * v.z()) + (m.atr[1].w() * v.w());
    double z = (m.atr[2].x() * v.x()) + (m.atr[2].y() * v.y()) + (m.atr[2].z() * v.z()) + (m.atr[2].w() * v.w());
    double w = (m.atr[3].x() * v.x()) + (m.atr[3].y() * v.y()) + (m.atr[3].z() * v.z()) + (m.atr[3].w() * v.w());
    return vector4(x,y,z,w);
}

/*
========================================================================================
And now for the funny functions we're all here for!
========================================================================================
*/

// Print a 4x4 matrix
void printMatrix(const matrix4& matrix) {
    for (const auto& row : matrix.atr) {
        for (float value : row.atr) {
            std::cout << value << ' ';
        }
        std::cout << '\n';
    }
};

// Returns the 4x4 identity matrix
matrix4 mat4Identity() {
    double v1[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    double v2[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    double v3[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    double v4[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    return matrix4({v1,v2,v3,v4});
};

// Create a perspective projection matrix
matrix4 createPerspectiveMatrix(float fov, float aspectRatio, float near, float far) {
    double f = 1.0 / std::tan(fov / 2.0);
    double range = near - far;

    return matrix4{{f / aspectRatio, 0.0f, 0.0f, 0.0f},
                   {0.0f, f, 0.0f, 0.0f},
                   {0.0f, 0.0f, (far + near) / range, -1.0f},
                   {0.0f, 0.0f, 2.0f * far * near / range, 0.0f}};
};

// Function to convert 3D point on a sphere to 2D image coordinates
// For ease of implementation, just returns a vector 3 object with values (x,y,0)
vector3 projectPointToImage (const vector3& point3D, const matrix4& viewProjectionMatrix, int screenWidth, int screenHeight) {
    vector4 homogeneousClip = viewProjectionMatrix * vector4(point3D, 1.0f);

    // Perspective division
    vector3 pointInClipSpace = vector3(homogeneousClip.x(), homogeneousClip.y(), homogeneousClip.z()) / homogeneousClip.w();

    // Map to screen coordinates
    int x = (pointInClipSpace.x() + 1.0f) * 0.5f * screenWidth;
    int y = (1.0f - pointInClipSpace.y()) * 0.5f * screenHeight;

    return vector3{x,y,0};
}

#endif