import json
import mp4toimages as mp

raydict = { "nbounces":10, "rendermode":"phong", "camera" : {}}
scenedict = { "backgroundcolor": [0.25, 0.25, 0.25], 
              "lightsources":[], "shapes":[] }
shapes = []
lights = []
mats = []

#class for handling linear change in vector or real values over specified frames
class change:
    def __init__ (self, obj, attrib, target, start, end):
        self.obj = obj
        self.attr = attrib
        self.targ = target
        self.start = start
        self.end = end
        self.initial = self.obj[self.attr] #the attribute as defined at frame 0
        self.init = self.initial #the attribute as defined at the start of the change

    def makeChange (self, i):
        if (i == self.start): self.init = self.obj[self.attr]
        if (i > self.start and i <= self.end): #change begins frame after start -> behave like keyframe
            self.obj[self.attr] = self.quickMaths(i)

    #figures out whether to alter as vector or as integer
    def quickMaths (self, i):
        try: #target is a vector
            len(self.obj[self.attr])
            vector = []
            for j in range(0, len(self.obj[self.attr])):
                dif = self.targ[j] - self.init[j]
                vector.append(dif * ((i - self.start) / (self.end - self.start)))
                
            final = []
            for j in range(0, len(vector)): final.append(self.init[j] + vector[j])
            return final
        
        except: #target is an integer
            dif = self.targ - self.init
            return self.init + (dif * ((i - self.start) / (self.end - self.start)))

#class for handling 1-frame changes, such as textures (never used nor tested)
class instantChange:
    def __init__ (self, obj, attrib, target, start, end):
        self.obj = obj
        self.attr = attrib
        self.targ = target
        self.start = start

    def makeChange (self, i):
        if (i == self.start): #change occurs ON start frame
            self.obj[self.attr] = self.self.targ

def camera (width = 1200, height = 800, pos = [0,0,0], look = [0,0,1], upVector = [0,1,0], fov = 45.0):
    camDit = {
    "type" : "pinhole",
    "width" : width, 
    "height" : height,
    "position" : pos,
    "lookAt" : look,
    "upVector" : upVector,
    "fov" : fov,
    "exposure" : 0.1 }
    return camDit

cam = camera()
changes = []

def addLight (pos, intensity) :
    lightDict = { "type":"pointlight", 
    "position":pos,
    "intensity":intensity}
    return lightDict;

def material (ks = 0.1, kd = 0.9, se = 10, dc = [1,0,0], dt = "null", sc = [1,1,1], ir=False, refl=1.0, ir2=False, refr=1.0):
    mat = {
    "ks":ks, 
    "kd":kd, 
    "specularexponent":se, 
    "diffusecolor":dc,
    "diffusetexture":dt,
    "specularcolor":sc,
    "isreflective":ir,
    "reflectivity":refl,
    "isrefractive":ir2,
    "refractiveindex":refr }
    return mat

def sphere (c, r, m=None):
    sphere = { "type": "sphere",
               "center" : c,
               "radius" : r,
               "material" : m }
    if (m == None): sphere["material"] = material()
    return sphere

def cylinder (c, a, r, h, m=None):
    sphere = { "type": "cylinder",
               "center" : c,
               "axis" : a,
               "radius" : r,
               "height" : h,
               "material" : m }
    if (m == None): sphere["material"] = material()
    return sphere

def triangle (v0, v1, v2, m=None):
    sphere = { "type": "triangle",
               "v0" : v0,
               "v1" : v1,
               "v2" : v2,
               "material" : m }
    if (m == None): sphere["material"] = material()
    return sphere

def intToLongString(i, size):
    out = ""
    i_str = str(i)
    for j in range(0, size- len(i_str)): out += "0"
    return out + i_str

def buildAndDump (iteration, r, s, lights, cam, shapes) :
    s['shapes'] = shapes
    s['lightsources'] = lights
    r['camera'] = cam
    r['scene'] = s
    
    json_object = json.dumps(r, indent=4)
    with open("gennedJSON/" + str(iteration) + ".json", "w+") as file:
        file.write(json_object)

def reset (changes):
    for c in changes:
        c.obj[c.attr] = c.initial

#iteratively generates json files -> applies changes as desired
def iterativeBuildDump (r, s, lights, cam, shapes, changes, start=0, end=25):
    for i in range(0, end):
        
        if (i >= start):
            print(i)
            fname = intToLongString(i,5)
            buildAndDump(fname, r,s,lights,cam, shapes)

        #thanks to shallow referencing this updates the json objects all fine and proper >:)
        for c in changes:
            c.makeChange(i)
            
    reset(changes)

#renders the video using a script I pulled from a personal python project lol
#think it's mostly stack-overflow and github code tho lol
def render (name):
    mp.render(name)

def loadShapes (file, t):
    f = open(file)
    d = json.load(f)['scene']['shapes']
    for s in d:
        if (s["type"] == "triangle"):
            s["v0"] = [s["v0"][0] + t[0], s["v0"][1] + t[1], s["v0"][2] + t[2]]
            s["v1"] = [s["v1"][0] + t[0], s["v1"][1] + t[1], s["v1"][2] + t[2]]
            s["v2"] = [s["v2"][0] + t[0], s["v2"][1] + t[1], s["v2"][2] + t[2]]
        else:
            x = s["position"]
            s["position"] = [x[0] + t[0], x[1] + t[1], x[2] + t[2]]
    return d


"""STATEMENTS : GIANT SLIME PLANET"""

"""
cam = camera(pos=[0,0.75, -0.25], look=[0,0.35,1.0])
mats.append(material(ir=True))
shapes.append(triangle([0, 0.0, 2.25],[0.75, 0.0, 2],[0, 0.75, 2.25], mats[0]))
shapes.append(triangle([0.75, 0.75, 2],[0.75, 0.0, 2],[0, 0.75, 2.25], mats[0]))
mats.append(material(dc=[0.5,1,0.5], dt='test.ppm'))
shapes.append(sphere([0,-25.0,0], 25.1, mats[1]))
lights.append(addLight([0,1,0.5],[0,0,0]))
lights.append(addLight([0,1,-0.5],[0,0,0]))
scenedict["backgroundcolor"] = [0.05,0.05,0.05]
changes.append(change(scenedict, "backgroundcolor", [0.25,0.25,0.25], 0, 15))
mats.append(material(dc=[0.5, 0.5, 0.8], se=20))
shapes.append(cylinder([-0.3, 0.19, 1], [0,1,0], 0.15, 0.2, mats[2]))
shapes.append(sphere([-0.3,0.59,1], 0.2, mats[0]))
shapes[-1] = sphere([0.3,0.29,1], 0.2, mats[0])
mats.append(material(dc = [0.8, 0.5, 0.5], se=20))
shapes.append(sphere([-0.3,0.59,1], 0.2, mats[-1]))
changes.append(change(lights[0], "intensity", [0.5,0.5,0.5], 0, 15))
changes.append(change(lights[1], "intensity", [0.5,0.5,0.5], 0, 15))
changes.append(change(cam, "position", [-12,2,-20], 25, 50))
changes[-1] = change(cam, "position", [-12,5,-50], 25, 50)
changes.append(change(cam, "lookAt", [0,-12.5,1], 25, 50))
changes[-2] = change(cam, "position", [-12,12,-50], 25, 50)
iterativeBuildDump(raydict, scenedict, lights, cam, shapes, changes,0, 50)
changes[-2] = change(cam, "position", [-12,12,-100], 25, 50)
iterativeBuildDump(raydict, scenedict, lights, cam, shapes, changes,0, 50)
changes[-1] = change(cam, "lookAt", [0,-25,1], 25, 50)
iterativeBuildDump(raydict, scenedict, lights, cam, shapes, changes,0, 50)
changes.append(change(cam, "position", [12,-12.5,-85], 50, 75))
changes.append(change(cam, "lookAt", [0,-31.5,0], 75, 90))
changes.append(change(cam, "position", [-2.5,-31.5,-25], 75, 90))
iterativeBuildDump(raydict, scenedict, lights, cam, shapes, changes,0, 100)
"""

""" STATEMENTS : FOREST """
"""
mats.append(material(dc=[74/255, 181/255, 103/255]))
shapes.append(triangle([0,-0.1,-10], [-20,0,12], [20,0,12], mats[0]))
cam = camera(pos=[0,2, -4.5], look=[0,0,-4.5])
scenedict["backgroundcolor"] = [45/255, 42/255, 51/255]
#shapes += loadShapes("tree.json", [-2,0,2])
shapes += loadShapes("tree.json", [3,0,5])
#shapes += loadShapes("tree.json", [1.25,-0.5,9])
shapes += loadShapes("fire.json", [0.1,0.1,1])

#targ look: [0,0.35,1.0]
#targ up: [0,1,0]

mats.append(material(dt="stone.ppm"))
shapes.append(cylinder([1.45,0,-0.7],[0,1,0],0.35,0.06,mats[-1]))
shapes.append(cylinder([-1.5,-0.02,-0.65],[0,1,0],0.4,0.05,mats[-1]))
lights.append(addLight([-5,2,15],[0.5,0.5,0.5]))
mats.append(material(dc = [227/255, 78/255, 48/255]))
shapes.append(sphere([-5,-2,15],1,mats[-1]))

#look up
changes.append(change(cam, "lookAt", [0,0.35,1.0], 10, 25))
#changes.append(change(cam, "position", [0,2,-3.5], 10, 25))

#sunrise - sun
changes.append(change(shapes[-1], "center", [-5,0,15], 10, 40))
changes.append(change(shapes[-1], "center", [-3,2,15], 40, 60))
changes.append(change(shapes[-1], "center", [0,6,15], 60, 100))

#sunrise - sunlight
changes.append(change(lights[-1], "position", [-3,2,12], 40, 60))
changes.append(change(lights[-1], "intensity", [0.65,0.65,0.65], 40, 60))
changes.append(change(lights[-1], "position", [-0.5,6,8], 60, 100))

#sun colour
changes.append(change(mats[-1], "diffusecolor", [235/255, 196/255, 23/255], 45, 75))

#sky colour
changes.append(change(scenedict, "backgroundcolor", [237/255, 178/255, 69/255], 10, 50))
changes.append(change(scenedict, "backgroundcolor", [163/255, 229/255, 240/255], 50, 100))

#spin camera                                 
changes.append(change(cam, "lookAt", [5.5,0.35,-4.5], 90, 107))
changes.append(change(cam, "lookAt", [0,2,-10], 107, 125))

iterativeBuildDump(raydict, scenedict, lights, cam, shapes, changes, 0, 125)
"""

"""STATEMENTS: FUNNY DOG"""
#"""

mats.append(material(dc=[1,0.3,0.4], ir=True, refl=0.0))
cam = camera(pos=[0,0.3, -1], look=[0,0.3,1])
scenedict["backgroundcolor"] = [163/255, 229/255, 240/255]
shapes.append(sphere([-0.75,0.3,1], 0.2, mats[0]))
shapes.append(sphere([0.75,0.3,1], 0.2, mats[0]))
lights.append(addLight([2,1,-2.5],[0.5,0.5,0.5]))
lights.append(addLight([-2,1,-2.5],[0.5,0.5,0.5]))

mats.append(material(dt="dog.ppm"))
shapes.append(sphere([0,7,1], 0.25, mats[-1]))

changes.append(change(mats[0], "reflectivity", 0.9, 0, 20))
changes.append(change(shapes[-1], "center", [0,0.3,1], 0, 12))

iterativeBuildDump(raydict, scenedict, lights, cam, shapes, changes, 0, 25)
#"""
