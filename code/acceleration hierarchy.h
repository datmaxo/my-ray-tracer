#ifndef ACCELERATION_HIERARCHY_H
#define ACCELERATION_HIERARCHY_H

#include <algorithm>
#include <iostream>
#include <cmath>
#include "vector_helper.h"
#include "shape.h"

/*
==================================================================================================
The acceleration hierarchy speeds up the rendering process by vastly reducing the number of
intersections required to test for as the number of shapes in a scene increases.
==================================================================================================
*/

// BVH Node
struct BVHNode {
    Cube bounds;
    BVHNode* left;
    BVHNode* right;
    Shape* shape;

    BVHNode(const Cube& _bounds) : bounds(_bounds), left(nullptr), right(nullptr), shape(nullptr) {}
};

// Function to traverse the BVH and find the closest intersection
Hit intersectBVH(vector3 camPos, Ray& ray, const std::list<LightSource*>& l, const vector3& look, BVHNode* node, bool calcMaterial = true, bool toplayer = false) {
    if (!node) {
        Hit h = Hit();
        h.setChecks(1);
        return h;
    }

    //check intersection with shape in bounding box, if it exists
    if (node->shape && node->shape->getType() != "null") {
        Hit h = node->shape->intersect(ray, l, look, calcMaterial);
        h.setChecks(1);
        return h;
    }

    //check intersection with bounding box
    if (node->bounds.intersect(ray, l, look, calcMaterial).getT() <= 0) { 
        //std::cout << "Didn't collide with this boundary! (" << node->bounds.getCenter().x() << "," << node->bounds.getCenter().y() << "," << node->bounds.getCenter().z() << "]" << std::endl;
        Hit h = Hit();
        h.setChecks(1);
        return h;
    }

    //std::cout << node->bounds.intersect(ray, l, look, calcMaterial).getT() << std::endl;
    //calculate sub-trees
    Hit hitLeft = intersectBVH(camPos, ray, l, look, node->left, calcMaterial);
    Hit hitRight = intersectBVH(camPos, ray, l, look, node->right, calcMaterial);
    
    //for debugginf -> set each subnode to have the same (total) checks
    int total = hitRight.getChecks() + hitLeft.getChecks();
    hitLeft.setChecks(total);
    hitRight.setChecks(total);
    //std::cout << hitLeft.getChecks() << std::endl;

    //reshape the node if either hit point was on the boundary of this bounding box
    //this is only necessary for getting shadows to appear on triangles -> was not a fun debugging process
    node->bounds.reshapeIfOnBoundary(hitLeft.getPoint());
    node->bounds.reshapeIfOnBoundary(hitRight.getPoint());

    if ((hitRight.getT() < hitLeft.getT() && hitRight.getT() >= 0) || (hitLeft.getT() < 0)) { return hitRight;}
    return hitLeft;
}

// Function to build a BVH from a list of spheres
BVHNode* buildBVH(vector3 camPos, std::vector<Shape*>& shapes, int start, int end) {
    if (start == end) {
        BVHNode* leafNode = new BVHNode(Cube(shapes[start], camPos));
        leafNode->shape = shapes[start];
        return leafNode;
    }
    //std::cout << end - start << std::endl;

    // Calculate bounding box for the current node
    std::vector<Shape*> partialShapeList = {};
    for (int i = start; i <= end; i++) {
        partialShapeList.push_back(shapes[i]);
    }
    Cube nodeBounds = Cube(partialShapeList, camPos);
    //partialShapeList = {};

    // Sort the spheres along the longest axis
    int splitAxis = nodeBounds.longestAxis();
    int mid = (start + end) / 2;
    std::nth_element(shapes.begin() + start, shapes.begin() + mid, shapes.begin() + end + 1, [splitAxis]( Shape* a, Shape* b) {
        return a->getCenter().atr[splitAxis] < b->getCenter().atr[splitAxis];
    });

    // Create the current BVH node
    BVHNode* node = new BVHNode(nodeBounds);
    node->left = buildBVH(camPos, shapes, start, mid);
    node->right = buildBVH(camPos, shapes, mid + 1, end);
    //TODO: cull nodes if they contain a shape of 'null' type -> might require a travsersal 'trim' function?

    return node;
}

// Prints the acceleration structure before rendering in a very neat and nice fashion!
bool printBVH(BVHNode* root, std::string lvl) {
    
    std::cout << lvl << "Node - " ;
    if (root -> shape) {
        std::cout << "contains: [" << root -> shape -> getType() << "]" << std::endl;
        std::cout << lvl << "Size -> " << root->bounds.x() << " : " << root->bounds.y()<<  " : "  << root->bounds.z() << std::endl;
        vector3 c = root-> bounds.getCenter();
        std::cout << lvl << "Center -> " << c.x() << " : " << c.y()<<  " : "  << c.z() << std::endl;
        return false;
    }
    else { std::cout << "nonleaf"; }
    std::cout << std::endl;
    std::string nextLvl = "    " + lvl;

    std::cout << lvl << "Size -> " << root->bounds.x() << " : " << root->bounds.y()<<  " : "  << root->bounds.z() << std::endl;
    vector3 c = root-> bounds.getCenter();
    std::cout << lvl << "Center -> " << c.x() << " : " << c.y()<<  " : "  << c.z() << std::endl;

    std::cout << lvl << "L: " << std::endl;
    printBVH(root->left, nextLvl);
    std::cout << lvl << "R: " << std::endl;
    printBVH(root->right, nextLvl);
    return true;
}

#endif