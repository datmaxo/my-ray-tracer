#ifndef COLOURS_H
#define COLOURS_H

#include <iostream>
#include <cmath>
#include "vector_helper.h"

/*
================================================================================================================
Scene was rendering pretty drably, so added this GPT generated script to boost the saturation of all colours!
Doesn't seem to hurt performance too badly either :)
================================================================================================================
*/

// RGB to HSL conversion
void rgbToHsl(const vector3& rgb, double& hue, double& saturation, double& lightness) {
    double maxColor = std::max({rgb.x(), rgb.y(), rgb.z()});
    double minColor = std::min({rgb.x(), rgb.y(), rgb.z()});

    lightness = (maxColor + minColor) / 2.0;

    if (maxColor == minColor) {
        hue = saturation = 0.0; // Grayscale
    } else {
        double delta = maxColor - minColor;

        saturation = (lightness > 0.5) ? (delta / (2.0 - maxColor - minColor)) : (delta / (maxColor + minColor));

        if (maxColor == rgb.x()) {
            hue = (rgb.y() - rgb.z()) / delta + ((rgb.y() < rgb.z()) ? 6.0 : 0.0);
        } else if (maxColor == rgb.y()) {
            hue = (rgb.z() - rgb.x()) / delta + 2.0;
        } else {
            hue = (rgb.x() - rgb.y()) / delta + 4.0;
        }

        hue /= 6.0;
    }
}

// Helper function for HSL to RGB conversion
double hueToRgb(double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;

    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;

    return p;
}

// HSL to RGB conversion
vector3 hslToRgb(double hue, double saturation, double lightness) {
    double q = (lightness < 0.5) ? (lightness * (1.0 + saturation)) : (lightness + saturation - lightness * saturation);
    double p = 2.0 * lightness - q;

    double r = hueToRgb(p, q, hue + 1.0 / 3.0);
    double g = hueToRgb(p, q, hue);
    double b = hueToRgb(p, q, hue - 1.0 / 3.0);

    return {r, g, b};
}

// Function to increase saturation
vector3 increaseSaturation(const vector3& rgb, double factor) {
    double hue, saturation, lightness;
    rgbToHsl(rgb, hue, saturation, lightness);

    saturation *= (1.0 + factor);
    saturation = std::min(1.0, saturation);

    return hslToRgb(hue, saturation, lightness);
}

#endif