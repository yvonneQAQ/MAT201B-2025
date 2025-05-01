#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include "al/math/al_Vec.hpp"

using namespace al;

#include <fstream>
#include <vector>
using namespace std;

Vec3f randomVec3f(float scale) {
  return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * scale;
}
float flockCentering = 1.2;
float alignStrength = 0.4;
float cubeAvoidance = 0.1;
float goalTimer = 0;       // elapsed time since last update
float goalInterval = 3.0f; // update every 5 seconds

struct AlloApp : App {
  Parameter timeStep{"/timeStep", "", 0.1, 0.01, 0.6};
  Light light;
  Material material;  // Necessary for specular highlights
  Mesh cone;
  Mesh cube;

  // size, color, species, sex, age, etc.
  std::vector<Nav> agent;
  std::vector<float> size; // (0, 1]
  std::vector<int> interest; // (0, 1]
  std::vector<Vec3f> cubePos;
  std::vector<Vec3f> velocity;
  std::vector<Vec3f> target;

  void onInit() override {
    auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
    auto &gui = GUIdomain->newGUI();
    gui.add(timeStep);
  }

  void onCreate() override {
    nav().pos(0, 0, 10);
    // addSphere(mesh);
    // mesh.translate(0, 0, -0.1);
    addSphere(cone);
    cone.scale(1, 1, 1.5);
    cone.scale(0.08);

    cone.generateNormals();
    light.pos(0, 10, 10);

    for (int i = 0; i < 18; ++i) {
      Nav p;
      p.pos() = randomVec3f(5);
      p.quat()
          .set(rnd::uniformS(), rnd::uniformS(), rnd::uniformS(),
               rnd::uniformS())
          .normalize();
      velocity.push_back(Vec3f(0)); // 初始速度为0
      target.push_back(randomVec3f(5)); // 给每个agent一个初始目标
      agent.push_back(p);
      size.push_back(rnd::uniform(0.05, 1.0));
      interest.push_back(-1);
    }

    //we make obstacles here 
    addCube(cube);
    cube.scale(0.3);
    // Generate a random position vector
    float spacing = 2.0;  // distance between cubes
    Vec3f centerOffset = Vec3f(-spacing, -spacing, -spacing); // center the structure
    
    for (int x = 0; x < 3; ++x) {
      for (int y = 0; y < 3; ++y) {
        for (int z = 0; z < 3; ++z) {
          Vec3f pos = Vec3f(x * spacing, y * spacing, z * spacing) + centerOffset;
          cubePos.push_back(pos);
        }
      }
    }

  }
  // Translate the cube to that position

  void onAnimate(double dt) override {
    goalTimer += dt;
  if (goalTimer > goalInterval) {
  goalTimer = 0;

  // assign each agent a new target
  for (int i = 0; i < target.size(); ++i) {
    target[i] = randomVec3f(8); // or larger like randomVec3f(10)
  }
}

    for (int i = 0; i < agent.size(); ++i) {
      Vec3f pos = agent[i].pos();
      Vec3f dir(0);
  
      // ---------- Wander (seek random target) ----------
      Vec3f seek = target[i] - pos;
      float dist = seek.mag();
      if (dist < 0.1) {
        target[i] = randomVec3f(5);
      } else {
        seek.normalize();
        dir += seek;
      }
  
      // ---------- Flock Centering ----------
      Vec3f center(0);
      Vec3f align(0);
      Vec3f repel(0); // agent-agent avoidance
      int neighborCount = 0;
  
      for (int j = 0; j < agent.size(); ++j) {
        if (i == j) continue;
        Vec3f diff = agent[j].pos() - pos;
        float d = diff.mag();
  
        // Flock only if not too close
        if (d > 1.0 && d < 4.0) {
          center += agent[j].pos();
          align += velocity[j];
          neighborCount++;
        }
  
        // Repel if too close
        if (d < 1.0 && d > 0.001) {
          repel -= diff.normalize() / d; // repel direction: away
        }
      }
  
      if (neighborCount > 0) {
        center /= neighborCount;
        Vec3f centerDir = (center - pos).normalize();
        Vec3f alignDir = (align / neighborCount).normalize();
        dir += centerDir * flockCentering;
        dir += alignDir * alignStrength;
      }
  
      // Add agent-agent avoidance
      if (repel.mag() > 0.001) {
        repel.normalize();
        dir += repel * cubeAvoidance;
      }
  
      // ---------- Cube Avoidance ----------
      Vec3f cubeRepel(0);
      for (int c = 0; c < cubePos.size(); ++c) {
        Vec3f diff = pos - cubePos[c];
        float d = diff.mag();
        if (d < 1.5 && d > 0.001) {
          cubeRepel += diff.normalize() / d;
        }
      }
      if (cubeRepel.mag() > 0.001) {
        cubeRepel.normalize();
        dir += cubeRepel * cubeAvoidance;
      }
  
      // ---------- Final Move ----------
      if (dir.mag() > 0.001) {
        dir.normalize();
        velocity[i] = dir * 0.05;
        pos += velocity[i];
        agent[i].pos() = pos;
        agent[i].faceToward(pos + velocity[i]);
      }
    }
  }

  void onDraw(Graphics &g) override {
    g.clear(0.27);
    g.depthTesting(true);
    g.lighting(true);
    light.globalAmbient(RGB(0.1));
    light.ambient(RGB(0));
    light.diffuse(RGB(1, 1, 0.5));
    g.light(light);
    material.specular(light.diffuse() * 0.2);
    material.shininess(50);
    g.material(material);
  
      for (int i = 0; i < agent.size(); ++i) {
        g.pushMatrix();
        g.translate(agent[i].pos());
        g.rotate(agent[i].quat());
        g.scale(size[i]);
        g.draw(cone);
        g.popMatrix();
    }

    // Draw cube
    for (int i = 0; i < cubePos.size(); i++) {
      g.pushMatrix();
      g.translate(cubePos[i]);
      g.draw(cube);
      g.popMatrix();
  }
  }
};

int main() {
  AlloApp app;
  app.configureAudio(48000, 512, 2, 0);
  app.start();
}