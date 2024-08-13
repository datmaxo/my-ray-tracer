#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <array>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "raytracer.h"
#include "scene.h"
#include "shape.h"
#include "camera.h"
#include "material.h"
#include "lightsource.h"
#include "vector_helper.h"
#include "image.h"
#include "acceleration hierarchy.h"

using json = nlohmann::json;

/* 
==================================================================================================
This is the main program file!

A lot of this code will probably be removed at one stage or another now that I'm working on this
on my own time; I'd like to pivot away from a CLI interface but I don't know how probable such
a development is within the scope of macOS.

TODO: put the scene loading elements into their own script, as they clog this one up quite a bit.
==================================================================================================
*/

// Function to read and load a PPM image
Image readPPM(const std::string& filename, bool print = true) {
    std::ifstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return Image();
    }

    std::string magic;
    file >> magic;

    // Check for both P3 and P6 formats
    if (magic != "P3" && magic != "P6") {
        std::cerr << "Invalid PPM file format (must be P3 or P6)" << std::endl;
        return Image();
    }

    int width;
    int height;
    file >> width >> height;
    Image img = Image(width, height);
    if (print) std::cout << "Reading " << width << "x" << height << " texture image..." << std::endl;

    int maxColor;
    file >> maxColor;

    if (maxColor != 255) {
        std::cerr << "Unsupported color depth (must be 255)" << std::endl;
        return Image();
    }

    // Consume newline character after max color value
    file.get();

    // Read pixel data based on the format
    if (magic == "P6") {
        std::cout << "P6 reading currently unimplemented lol" << std::endl;
        // P6 format (binary)
        //file.read(reinterpret_cast<char*>(image.data()), width * height * sizeof(RGB));
    } else {
        // P3 format (ASCII)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                double r, g, b;
                file >> r >> g >> b;
                img.setPixel(x,y, vector3{r / 255, g / 255, b / 255});
            }
        }
    }

    if (!file) {
        std::cerr << "Error reading pixel data from file" << std::endl;
        return Image();
    }

    file.close();

    if (print) { std::cout << "Texture loaded sucessfully!" << std::endl; }
    return img;
}

//Reads JSON file with name 'filename' and initialises a complete scene & camera, which are entered into a new RayTracer instance and returned.
RayTracer loadScene (std::string& filename, bool print = true) {
    
    //read JSON
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening the JSON file." << std::endl;
        return RayTracer();
    }
    json jsonData;
    file >> jsonData;
    file.close();

    //create camera
    Camera cam;
    if (print) { std::cout << "Loading Camera...\n" << std::endl; }
    json camData = jsonData["camera"];
    std::vector<double> pos = camData["position"]; //casting json to vector3 class is hard, but we can cast std::vectors to vector 3 easy!
    std::vector<double> look = camData["lookAt"];
    std::vector<double> up = camData["upVector"];
    if (camData["type"] == "pinhole") {
        cam = PinholeCamera(camData["width"], camData["height"], vector3(pos), vector3(look), vector3(up), camData["fov"], camData["exposure"]);
    }
    else {
        cam = ThinLens(camData["width"], camData["height"], pos, look, up, camData["fov"], camData["exposure"]);
    }

    //create shapes
    std::vector<Shape*> shapes;
    int objID = 1;
    json sceneData = jsonData["scene"];
    json shapeData = sceneData["shapes"];
    for (const auto& item : shapeData.items() ) {
        if (print) {std::cout << "Loading " << item.value()["type"] << " [id: " << objID << "] ..." << std::endl;}
        
        json md = item.value()["material"];
        Material mat;
        if (!md.is_null()) {
            std::vector<double> difc = md["diffusecolor"];
            std::vector<double> spec = md["specularcolor"];
            bool hasTexture = false;
            Image texture;
            json jsonTex = md["diffusetexture"];
            if (!jsonTex.is_null()) {
                hasTexture = true;
                if (jsonTex == "null") { hasTexture = false; }
                else {
                    texture = readPPM(jsonTex, print);
                }
            }
            mat = Material(md["ks"], md["kd"], md["specularexponent"], vector3(difc), hasTexture, texture, vector3(spec), md["isreflective"], md["reflectivity"], md["isrefractive"], md["refractiveindex"]);
        } else { std::cout << "Material not found!" << std::endl; }

        if (item.value()["type"] == "triangle") {
            std::vector<double> v0 = item.value()["v0"];
            std::vector<double> v1 = item.value()["v1"];
            std::vector<double> v2 = item.value()["v2"];
            Shape* newShape = new Triangle(vector3(v0), vector3(v1), vector3(v2), mat, objID);
            shapes.push_back(newShape);
        } else if (item.value()["type"] == "cylinder") {
            std::vector<double> c = item.value()["center"];
            std::vector<double> a = item.value()["axis"];
            Shape* newShape = new Cylinder(vector3(c), vector3(a), item.value()["radius"], item.value()["height"], mat, objID);
            shapes.push_back(newShape);
        } else {
            std::vector<double> c = item.value()["center"];
            Shape* newShape = new Sphere(vector3(c), item.value()["radius"], mat, objID);
            shapes.push_back(newShape);
        }
        objID++;
    }
    if (print) { std::cout << "Loaded " << shapes.size() << " shapes.\n" << std::endl; }

    //Assemble acceleration hierarchy
    BVHNode* root = buildBVH(cam.getPosition(), shapes, 0, shapes.size() - 1);
    if (print) {
        std::cout << "\n=== ACCELERATION HIERARCHY ===\n" << std::endl;
        printBVH(root, "* "); //display heirarchy! 
        std::cout << "\n======== HIERARCHY END =======\n" << std::endl;
    }

    //create lights
    std::list<LightSource*> lights;
    json lightData = sceneData["lightsources"];
    for (const auto& item : lightData.items() ) {
        std::cout << "Loading " << item.value()["type"] << "..." << std::endl;
        if (item.value()["type"] == "pointlight") {
            std::vector<double> p = item.value()["position"];
            std::vector<double> i = item.value()["intensity"];
            LightSource* newLight = new PointLight(vector3(p), vector3(i));
            lights.push_back(newLight);
        } else {
            std::vector<double> p = item.value()["position"];
            std::vector<double> i = item.value()["intensity"];
            LightSource* newLight = new AreaLight(vector3(p), vector3(i));
            lights.push_back(newLight);
        }
    }
    if (print) { std::cout << "Loaded " << lights.size() << " lights.\n" << std::endl; }

    //if no lights have been specified, initialise a generic 'null' light -> ensures rendering will still work
    if (lights.size() == 0) {
        LightSource* emergencyLight = new PointLight(vector3{0,0,0}, vector3{0,0,0});
        lights.push_back(emergencyLight);
    }

    //create scene, create & return raytracer
    std::vector<double> bgcol = sceneData["backgroundcolor"];
    Scene scene = Scene(vector3(bgcol), lights, root);
    if (print) { std::cout << "Scene Loaded!" << std::endl; }

    std::list<Ray> rays;
    std::list<Hit> hits;
    int bounces = 8;
    if (!jsonData["nbounces"].is_null()) { bounces = jsonData["nbounces"]; }

    RayTracer r = RayTracer(cam, scene, rays, hits, bounces, jsonData["rendermode"], objID);
    std::cout << "Ray Tracer Loaded!" << std::endl;

    return r; 
}

//Save an image in PPM format
void exportPPMImage(const std::string& filename, Image image, Camera cam) {
    int width = image.getWidth();
    int height = image.getHeight();

    // Create and open the PPM file for writing
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open the output file." << std::endl;
        return;
    }

    // Write the PPM header (P3 format, width, height, and maximum color value)
    outputFile << "P3\n" << width << " " << height << "\n255\n";

    // Write the pixel data to the file
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            vector3 pixl = image.getPixel(x,y);
            outputFile << pixl.atr[0] << " " << pixl.atr[1] << " " << pixl.atr[2] << " ";
        }
        outputFile << "\n";
    }

    // Close the output file
    outputFile.close();

    std::cout << "Exported PPM image with dimensions " << width << "x" << height << " ..." << std::endl;
}

// Function to get a list of paths to JSON files in a given directory -> gpt generated
std::vector<std::string> getJsonFilePaths(const std::string& folderPath) {
    std::vector<std::string> jsonFilePaths;

    try {
        for (const auto& entry : std::__fs::filesystem::directory_iterator(folderPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                // Add the path to the list
                jsonFilePaths.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return jsonFilePaths;
}

int main () {
    std::cout << "Welcome to the raytracer!!\n" << std::endl;

    std::string bulkMode;
    std::cout << "Are you wanting to render multiple scenes ? [y/n] : ";
    std::cin >> bulkMode;

    if (bulkMode == "y") {
        std::string path;
        std::cout << "Chuck us a file path full of lovely little scene jsons brother! : ";
        std::cin >> path;
        path += "/";
        std::cout << "\nReading " << path << " ... " << std::endl;
        std::vector<std::string> files = getJsonFilePaths(path);
        std::sort(files.begin(), files.end());

        int samples;
        std::cout << "\nScene files loaded :) how many samples per pixel my man? : ";
        std::cin >> samples;

        int start = 0;
        std::cout << "\nWhat frame do we wanna start rendering from? : ";
        std::cin >> start;

        int print = 0;
        bool toPrint = false;
        std::cout << "\nDo you want the console to print stuff? This will cause a lot of clutter for busy scenes.\n[1: yes, 0:no] : ";
        std::cin >> print;
        if (print == 1) { toPrint = true; }

        std::cout << "Okay - this is gonna be a lot! Hold on to your butt :O" << std::endl;
        int i = 0;
        RayTracer tracer;

        for (std::string file : files) {
            //skip frames as desired! helps with not having to re-render stuff innit
            if (i >= start) {
                std::cout << std::endl;
                tracer = loadScene(file, toPrint);

                Image img = tracer.startThreadedRender(samples,20);
                std::string offset = ""; //used to order scenes manually -> keeps pics sorted in frame order for my video
                std::string num = std::to_string(i);
                int length = num.size() + offset.size();
                for (int j = 0; j < 5 - length; j++) {
                    num = "0" + num;
                }
                std::string title = "out/" + offset + num + ".ppm";
                exportPPMImage(title, img, tracer.getCamera());
            }
            i++;
        }
    }

    else {
        std::string file;
        std::cout << "Gizza a JSON scene file then chuck <3 : ";
        std::cin >> file;
        std::cout << "\nReading " << file << " ... " << std::endl;
        RayTracer tracer = loadScene(file);

        int samples;
        std::cout << "\nScene loaded :) how many samples per pixel my man? : ";
        std::cin >> samples;

        int threads;
        std::cout << "And how many threads? (default = 20) : ";
        std::cin >> threads;
        if (threads <= 0 || threads > 20) {threads = 20;}
        std::cout << "Rendering Scene [" << threads <<" threads] ... " << std::endl;

        //render and export
        //Image img = tracer.renderImage(samples);
        Image img = tracer.startThreadedRender(samples,threads);
        exportPPMImage("render.ppm", img, tracer.getCamera());

        std::cout << "the scene has " << tracer.totalObjs << " total objects btw.";
    }

    return 0;
}