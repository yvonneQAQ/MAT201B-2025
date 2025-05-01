// Karl Yerkes
// 2022-01-20

#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/math/al_Random.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}
string slurp(string fileName);  // forward declaration

struct AlloApp : App {
  float sphereRadius = 5.0f;
  vector<Vec3f> anchorDirs;
  Parameter pointSize{"/pointSize", "", 3.0, 1.0, 3.0};
  Parameter timeStep{"/timeStep", "", 0.08, 0.02, 0.1};
  Parameter dragFactor{"/dragFactor", "", 0.90, 0.0, 0.99};
  Parameter springK{"/sprinK", "", 0.04, 0.01, 0.5};
  Parameter chargeK{"/chargeK", "", 0.00, 0.0, 0.9};
   
  //

  ShaderProgram pointShader;

  //  simulation state
  Mesh mesh;  // position *is inside the mesh* mesh.vertices() are the positions
  vector<Vec3f> velocity;
  vector<Vec3f> force;
  vector<float> mass;


  void onInit() override {
    // set up GUI
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(pointSize);  // add parameter to GUI
    gui.add(timeStep);   // add parameter to GUI
    gui.add(dragFactor);   // add parameter to GUI
    gui.add(springK);
    gui.add(chargeK);

    //
  }

  void onCreate() override {
    // compile shaders
    pointShader.compile(slurp("../point-vertex.glsl"),
                        slurp("../point-fragment.glsl"),
                        slurp("../point-geometry.glsl"));

    // set initial conditions of the simulation
    //

    // c++11 "lambda" function
    auto randomColor = []() { return HSV(rnd::uniform(), 1.0f, 1.0f); };

    mesh.primitive(Mesh::POINTS);
    // does 1000 work on your system? how many can you make before you get a low
    // frame rate? do you need to use <1000?
    for (int _ = 0; _ < 1000; _++) {
      Vec3f dir = randomVec3f(1.0f).normalize();
      anchorDirs.push_back(dir);
      mesh.vertex(dir * (sphereRadius + rnd::uniformS(1.0f)));
      mesh.color(randomColor());

      // float m = rnd::uniform(3.0, 0.5);
      float m = 3 + rnd::normal() / 2;
      if (m < 0.5) m = 0.5;
      mass.push_back(m);

      // using a simplified volume/size relationship
      mesh.texCoord(pow(m, 1.0f / 3), 0);  // s, t

      // separate state arrays
      velocity.push_back(randomVec3f(0.1));
      force.push_back(randomVec3f(1));
    }

    nav().pos(0, 0, 50);
  }

  bool freeze = false;

  void onAnimate(double dt) override {
    if (freeze) return;
  
    vector<Vec3f> &position(mesh.vertices());
  
    // Hooke's Law parameters
    float epsilon = 0.001f;  
    
    // 1. Hooke’s Law: pull particles to sphere surface
    for (int i = 0; i < position.size(); ++i) {
      Vec3f a = position[i];
      Vec3f b = anchorDirs[i] * sphereRadius;
      Vec3f displacement = b - a;
      force[i] += springK.get() * displacement;
    }
   
        // drag
        for (int i = 0; i < velocity.size(); i++) {
          force[i] += - velocity[i] * dragFactor;
        }

    // columbus Law
    for (int i = 0; i < position.size(); ++i) {
      for (int j = i + 1; j < position.size(); ++j) {
        Vec3f dir = position[i] - position[j];
        float distSqr = dir.magSqr() + epsilon;
        float strength = chargeK / distSqr;
        Vec3f repulsion = dir.normalize(strength);
  
        force[i] += repulsion;
        force[j] -= repulsion;  // Equal and opposite
      }


    // Integration
    //
    for (int i = 0; i < velocity.size(); i++) {
      // "semi-implicit" Euler integration
      velocity[i] += force[i] / mass[i] * timeStep;
      position[i] += velocity[i] * timeStep;
    }

    // 4. Clear all forces
    for (auto &f : force) f.set(0);
  }
  
};

    // 

    // XXX you put code here that calculates gravitational forces and sets
    // accelerations These are pair-wise. Each unique pairing of two particles
    // These are equal but opposite: A exerts a force on B while B exerts that
    // same amount of force on A (but in the opposite direction!) Use a nested
    // for loop to visit each pair once The time complexity is O(n*n)
    //
    // Vec3f has lots of operations you might use...
    // • +=
    // • -=
    // • +
    // • -
    // • .normalize() ~ Vec3f points in the direction as it did, but has length 1
    // • .normalize(float scale) ~ same but length `scale`
    // • .mag() ~ length of the Vec3f
    // • .magSqr() ~ squared length of the Vec3f
    // • .dot(Vec3f f) 
    // • .cross(Vec3f f)



  bool onKeyDown(const Keyboard &k) override {
    if (k.key() == ' ') {
      freeze = !freeze;
    }

    if (k.key() == '1') {
      // introduce some "random" forces
      for (int i = 0; i < velocity.size(); i++) {
        // F = ma
        force[i] += randomVec3f(1);
      }
      
    }

    return true;
  }

  void onDraw(Graphics &g) override {
    g.clear(0.3);
    g.shader(pointShader);
    g.shader().uniform("pointSize", pointSize / 100);
    g.blending(true);
    g.blendTrans();
    g.depthTesting(true);
    g.draw(mesh);
  }
};

int main() {
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}

string slurp(string fileName) {
  fstream file(fileName);
  string returnValue = "";
  while (file.good()) {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}
