#include "al/app/al_App.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Graphics.hpp"
#include "al/math/al_Random.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

using namespace al;

struct Dot {
  Vec3f position;
};

struct AnimatedLine {
  int a, b;           // indices of start and end dots
  float t = 0.0f;     // progress from 0 to 1
};

struct MyApp : App {
  std::vector<Dot> dots;
  std::vector<AnimatedLine> lines;

  void onCreate() override {
    // Initialize navigation
    nav().pos(0, 0, 10);
    nav().setHome();
    
    // Create dots
    for (int i = 0; i < 1000; ++i) {
      Dot d;
      d.position = Vec3f(rnd::uniformS(5.0f), rnd::uniformS(5.0f), rnd::uniformS(5.0f));
      dots.push_back(d);
    }

    // Create initial lines
    for (int i = 0; i < 100; ++i) {
      int a = rnd::uniform<int>(0, dots.size());
      int b = rnd::uniform<int>(0, dots.size());
      if (a != b) {
        AnimatedLine line;
        line.a = a;
        line.b = b;
        lines.push_back(line);
      }
    }
  }

  void onAnimate(double dt) override {
    for (auto& line : lines) {
      if (line.t < 1.0f) {
        line.t += dt * 0.1;
        if (line.t > 1.0f) line.t = 1.0f;
      }
    }
  }

  std::vector<int> findNearestNeighbors(int index, int k) {
    std::vector<std::pair<float, int>> distances;
    Vec3f p0 = dots[index].position;
    for (int i = 0; i < dots.size(); ++i) {
      if (i == index) continue;
      float d = (dots[i].position - p0).magSqr();
      distances.push_back({d, i});
    }
    std::sort(distances.begin(), distances.end());
    std::vector<int> neighbors;
    neighbors.push_back(index);
    for (int i = 0; i < k - 1 && i < distances.size(); ++i) {
      neighbors.push_back(distances[i].second);
    }
    return neighbors;
  }

  void drawConvexHullGroup(Graphics& g, const std::vector<int>& indices) {
    std::vector<Vec2f> proj;
    for (int i : indices) {
      proj.push_back(Vec2f(dots[i].position.x, dots[i].position.y));
    }
    auto leftmost = std::min_element(proj.begin(), proj.end(), [](Vec2f a, Vec2f b){ return a.x < b.x; });
    std::vector<int> hull;
    int current = int(leftmost - proj.begin());
    do {
      hull.push_back(current);
      int next = (current + 1) % proj.size();
      for (int i = 0; i < proj.size(); ++i) {
        Vec2f a = proj[current];
        Vec2f b = proj[next];
        Vec2f c = proj[i];
        float cross = (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
        if (cross < 0) next = i;
      }
      current = next;
    } while (current != hull[0]);

    Mesh fillMesh;
    fillMesh.primitive(Mesh::TRIANGLES);
    g.color(0.7, 0.9, 1.0, 0.4);
    Vec3f center(0);
    for (int i : hull) center += dots[indices[i]].position;
    center /= hull.size();
    for (int i = 0; i < hull.size(); ++i) {
      Vec3f a = dots[indices[hull[i]]].position;
      Vec3f b = dots[indices[hull[(i+1) % hull.size()]]].position;
      fillMesh.vertex(center);
      fillMesh.vertex(a);
      fillMesh.vertex(b);
    }
    g.draw(fillMesh);
  }

  void onDraw(Graphics& g) override {
    g.clear(0);
    g.pointSize(0.001);
    g.color(1.0, 0.0, 0.0, 1.0);

    // Draw dots
    Mesh dotMesh;
    dotMesh.primitive(Mesh::POINTS);
    for (auto& d : dots) dotMesh.vertex(d.position);
    g.draw(dotMesh);

    // Draw animated lines
    Mesh redLines;
    redLines.primitive(Mesh::LINES);
    g.color(1.0, 0.0, 0.0);
    
    for (auto& line : lines) {
      Vec3f p1 = dots[line.a].position;
      Vec3f p2 = dots[line.b].position;
      Vec3f ctrl1 = (p1 + p2) * 0.5f + Vec3f(0, 0.8f, 0);
      Vec3f ctrl2 = (p1 + p2) * 0.5f + Vec3f(0, -0.8f, 0);
      Vec3f last = p1;
      for (float t = 0.05f; t <= line.t; t += 0.05f) {
        float u = 1 - t;
        Vec3f pt = u*u*u * p1 + 3*u*u*t * ctrl1 + 3*u*t*t * ctrl2 + t*t*t * p2;
        redLines.vertex(last);
        redLines.vertex(pt);
        last = pt;
      }
    }
    g.draw(redLines);

    // Draw convex hulls
    for (int i = 0; i < dots.size(); i += 10) {
      std::vector<int> group = findNearestNeighbors(i, 5);
      drawConvexHullGroup(g, group);
    }
  }
};

int main() {
  MyApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1024, 768);
  app.title("Final Project");
  app.fps(60);
  app.displayMode(Window::DEFAULT_BUF);
  app.start();
  return 0;
}
