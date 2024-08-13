#ifndef VECTORHELPER_H
#define VECTORHELPER_H

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

/*
========================================================================================
Rewriten to deal with a custom Vector3 class, rather than the std::vector<double> type.
Holy moly did this create some serious speedup! Way more than i expected lol.

TODO: move more funcs to be part of the vector class - single arg stuff like normalize
should be more intuitive this way?
========================================================================================
*/

class vector3 {
    public:
        double atr[3];
        vector3 () : atr{0,0,0} {}
        vector3 (double x, double y, double z) : atr{x,y,z} { }
        vector3 (double coords[3]) : atr{coords[0],coords[1],coords[2]} { }
        vector3 (std::vector<double> coords) : atr{coords[0],coords[1],coords[2]} { }

        double x () const { return atr[0]; }
        double y () const { return atr[1]; }
        double z () const { return atr[2]; }

        //negate the vector
        vector3 operator -() {
            return vector3 {-atr[0], -atr[1], -atr[2]};
        }

        //normal set equal to -> had to define after i defined eq and neq bool checks
        //void operator =(vector3 v) {
        //    atr[0] = v.x(); atr[1] = v.y(); atr[2] = v.z();
        //}

        //plus equals babey, you know how this one works
        vector3 operator +=(vector3 v) {
            return vector3 {atr[0] + v.x(), atr[1] + v.y(), atr[2] + v.z()};
        }

        //calculates the magnitude (length) of a vector
        double magnitude () const {
            double result = atr[0] * atr[0];
            result += atr[1] * atr[1];
            result += atr[2] * atr[2];
            return sqrt(result);
        }

        vector3 absolute () {
            return vector3 {abs(atr[0]), abs(atr[1]), abs(atr[2])};
        }

        //return the vector as a standard vector type; useful for converting normal vectors to colours, and probably not much else.
        std::vector<double> toStdVector () {
            return std::vector<double>{atr[0], atr[1], atr[2]};
        }
};

/* === Operators! === */

//eq
bool operator ==(const vector3& v1, const vector3& v2) {
    return (v1.x() == v2.x() && v1.y() == v2.y() && v1.z() == v2.z());
}

//neq
bool operator !=(const vector3& v1, const vector3& v2) {
    return !(v1 == v2);
}

//addition
vector3 operator +(const vector3& v1, const vector3& v2) {
    return vector3(v1.x() + v2.x(), v1.y() + v2.y(), v1.z() + v2.z());
}

//negation
vector3 operator -(const vector3& v1, const vector3& v2) {
    return vector3(v1.x() - v2.x(), v1.y() - v2.y(), v1.z() - v2.z());
}

//multiplies each dimension of input vector by a constant factor
vector3 operator *(const vector3& v, const double multiplier) {
    return vector3(v.x() * multiplier, v.y() * multiplier, v.z() * multiplier);
}

//constant multiplication but with the multiplier first - don't want to have to worry about order
vector3 operator *(const double multiplier, const vector3& v) {
    return vector3(v.x() * multiplier, v.y() * multiplier, v.z() * multiplier);
}

//calculate the standard multiplication of two vectors
vector3 operator *(const vector3& v1, const vector3& v2) {
    return vector3(v1.x() * v2.x(), v1.y() * v2.y(), v1.z() * v2.z());
}

//divison by a constant factor
vector3 operator /(const vector3& v, const double multiplier) {
    return vector3(v.x() / multiplier, v.y() / multiplier, v.z() / multiplier);
}

//division by another vector
vector3 operator /(const vector3& v1, const vector3& v2) {
    return vector3(v1.x() / v2.x(), v1.y() / v2.y(), v1.z() / v2.z());
}

//makes vectors printable in std::cout statements!
std::ostream& operator << (std::ostream &os, vector3 v) {
    return (os << "[" << v.x() << ", " << v.y() << ", " << v.z() << "]");
}

/* === Useful functions ! === */

// Function to calculate the dot product of two vectors - GPT Generated
double dotProduct(const vector3& v1, const vector3& v2) {
    double result = v1.x() * v2.x();
    result += v1.y() * v2.y();
    result += v1.z() * v2.z();
    return result;
}

//calculate the cross product between two vectors
vector3 crossProduct(const vector3& vector1, const vector3& vector2) {
    double resultX = vector1.y() * vector2.z() - vector1.z() * vector2.y();
    double resultY = vector1.z() * vector2.x() - vector1.x() * vector2.z();
    double resultZ = vector1.x() * vector2.y() - vector1.y() * vector2.x();
    vector3 result = {resultX, resultY, resultZ};
    return result;
}

//raise a vector to a power!
vector3 vectPow (vector3 v, double exponent) {
    return vector3 {pow(v.x(), exponent), pow(v.y(), exponent), pow(v.z(), exponent)};
}

//Normalizes an input vector; sets magnitude to 1 whilst maintaining relative directionality
vector3 vectNormalize (const vector3& v) {
    double mag = v.magnitude();
    if (mag > 0) {
        return vector3 (v.x() / mag, v.y() / mag, v.z() / mag);
    }
    return v; //if magnitude of vector is 0, just return the input vector
}

//transcribed from the glm geometric functions webpage
vector3 reflect (vector3 incident, vector3 normal) {
    return incident - 2.0 * dotProduct(normal, incident) * normal;
}

//transcribed from the glm geometric functions webpage
vector3 vectClamp (vector3 v, double min_d, double max_d) {
    double x = std::max(min_d, std::min(v.x(), max_d));
    double y = std::max(min_d, std::min(v.y(), max_d));;
    double z = std::max(min_d, std::min(v.z(), max_d));;
    return vector3(x,y,z);
}

//Converts a double vector to a vector of ints in the range of 0 to 255
//No checking for double in range of 0 to 1 -> unlikely to arise in errors due to structure of prototypical scene json
//New and improved to deal with vector3 types -> std::vector's have been more or less eliminated now!
vector3 convertColourVector(vector3 doublecolour) {
    vector3 colour;
    for (int i  = 0; i < 3; i++) {
        colour.atr[i] = ((int) (doublecolour.atr[i] * 255));
    }
    return colour;
}

vector3 gammaToneMap (vector3 v, double gamma) {
    double x = std::pow(v.x(), 1.0/gamma);
    double y = std::pow(v.y(), 1.0/gamma);
    double z = std::pow(v.z(), 1.0/gamma);
    return vector3(x,y,z);
}

//because it's mega useful and not in <algorithm> for this version of C++, here's clamp lol
double clamp(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}


//depreciated GPT-generated vector rotation function.
/* vector3 rotateVector (vector3 vector, vector3 axis, double angleDegrees) {

    double angleRadians = static_cast<double>(M_PI) * angleDegrees / 180.0f;

    // Calculate the rotation matrix
    double cosTheta = std::cos(angleRadians);
    double sinTheta = std::sin(angleRadians);
    double oneMinusCosTheta = 1.0f - cosTheta;

    vector3 normalizedAxis = vectNormalize(axis);

    double rotationMatrix[3][3] = {
        {cosTheta + normalizedAxis.x() * normalizedAxis.x() * oneMinusCosTheta, 
         normalizedAxis.x() * normalizedAxis.y() * oneMinusCosTheta - normalizedAxis.z() * sinTheta,
         normalizedAxis.x() * normalizedAxis.z() * oneMinusCosTheta + normalizedAxis.y() * sinTheta},

        {normalizedAxis.y() * normalizedAxis.x() * oneMinusCosTheta + normalizedAxis.z() * sinTheta,
         cosTheta + normalizedAxis.y() * normalizedAxis.y() * oneMinusCosTheta,
         normalizedAxis.y() * normalizedAxis.z() * oneMinusCosTheta - normalizedAxis.x() * sinTheta},

        {normalizedAxis.z() * normalizedAxis.x() * oneMinusCosTheta - normalizedAxis.y() * sinTheta,
         normalizedAxis.z() * normalizedAxis.y() * oneMinusCosTheta + normalizedAxis.x() * sinTheta,
         cosTheta + normalizedAxis.z() * normalizedAxis.z() * oneMinusCosTheta}
    };

    // Apply the rotation matrix to the vector
    return vector3{
        rotationMatrix[0][0] * vector.x() + rotationMatrix[0][1] * vector.y() + rotationMatrix[0][2] * vector.z(),
        rotationMatrix[1][0] * vector.x() + rotationMatrix[1][1] * vector.y() + rotationMatrix[1][2] * vector.z(),
        rotationMatrix[2][0] * vector.x() + rotationMatrix[2][1] * vector.y() + rotationMatrix[2][2] * vector.z()
    };
} */

#endif