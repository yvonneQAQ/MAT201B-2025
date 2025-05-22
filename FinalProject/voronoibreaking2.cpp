#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/math/al_Random.hpp"
#include <vector>
#include <map>
#include <limits>
#include <cmath>
#include <algorithm>

using namespace al;
using namespace std;

const int NUM_POINTS = 40;
const int NUM_RELAXATIONS = 5;
const int GRID_RES = 800;  // resolution of the approximation grid
const float DOMAIN_SIZE = 4.0f;  // [-2,2] space
const float BASE_DRIFT_SPEED = 0.001f;  // Base speed of drift
const float SPEED_VARIATION = 0.0005f;  // Speed variation range

// Ice color parameters
const float BASE_HUE = 0.55f;  // Base blue hue
const float HUE_VARIATION = 0.05f;  // Slight variation in blue
const float MIN_SAT = 0.1f;  // Lower saturation for whiter appearance
const float MAX_SAT = 0.3f;  // Lower max saturation for whiter appearance
const float MIN_VAL = 0.85f;  // Very bright minimum
const float MAX_VAL = 0.98f;  // Almost white maximum
const float MIN_ALPHA = 0.2f;  // Slightly more transparent
const float MAX_ALPHA = 0.5f;  // Maximum transparency

struct Cell {
  std::vector<Vec2f> points;
  Vec2f velocity;  // direction and speed of drift
  Color color;
  float speedMultiplier;  // Individual speed variation
};

struct VoronoiApp : App {
  vector<Vec2f> points;
  vector<Color> colors;
  vector<vector<Vec2f>> cells;
  std::vector<Cell> voronoiCells;
  
  Color generateIceColor() {
    float hue = BASE_HUE + rnd::uniform(-HUE_VARIATION, HUE_VARIATION);
    float sat = rnd::uniform(MIN_SAT, MAX_SAT);
    float val = rnd::uniform(MIN_VAL, MAX_VAL);
    float alpha = rnd::uniform(MIN_ALPHA, MAX_ALPHA);
    
    Color c = HSV(hue, sat, val);
    c.a = alpha;
    return c;
  }

  void onCreate() override {
    // Generate random points
    for (int i = 0; i < NUM_POINTS; ++i) {
      points.push_back(Vec2f(rnd::uniform(-1.0f, 1.0f), rnd::uniform(-1.0f, 1.0f)));
      colors.push_back(generateIceColor());
    }

    for (int i = 0; i < NUM_RELAXATIONS; ++i) {
      relaxPoints();
    }

    computeVoronoiCells();
  }

  void relaxPoints() {
    vector<Vec2f> newPoints(NUM_POINTS);
    vector<int> counts(NUM_POINTS, 0);

    for (int x = 0; x < GRID_RES; ++x) {
      for (int y = 0; y < GRID_RES; ++y) {
        float fx = (float)x / (GRID_RES - 1) * DOMAIN_SIZE - DOMAIN_SIZE/2;
        float fy = (float)y / (GRID_RES - 1) * DOMAIN_SIZE - DOMAIN_SIZE/2;
        Vec2f pos(fx, fy);

        int nearestIdx = 0;
        float minDist = std::numeric_limits<float>::max();
        for (int i = 0; i < points.size(); ++i) {
          float d = (pos - points[i]).magSqr();
          if (d < minDist) {
            minDist = d;
            nearestIdx = i;
          }
        }

        newPoints[nearestIdx] += pos;
        counts[nearestIdx]++;
      }
    }

    for (int i = 0; i < NUM_POINTS; ++i) {
      if (counts[i] > 0)
        points[i] = newPoints[i] / float(counts[i]);
    }
  }

  void computeVoronoiCells() {
    cells.clear();
    cells.resize(NUM_POINTS);

    for (int x = 0; x < GRID_RES; ++x) {
      for (int y = 0; y < GRID_RES; ++y) {
        float fx = (float)x / (GRID_RES - 1) * DOMAIN_SIZE - DOMAIN_SIZE/2;
        float fy = (float)y / (GRID_RES - 1) * DOMAIN_SIZE - DOMAIN_SIZE/2;
        Vec2f pos(fx, fy);

        int nearestIdx = 0;
        float minDist = std::numeric_limits<float>::max();
        for (int i = 0; i < points.size(); ++i) {
          float d = (pos - points[i]).magSqr();
          if (d < minDist) {
            minDist = d;
            nearestIdx = i;
          }
        }
        cells[nearestIdx].push_back(pos);
      }
    }

    // Create voronoi cells with velocities
    voronoiCells.clear();
    for (int i = 0; i < NUM_POINTS; ++i) {
      if (cells[i].empty()) continue;

      Vec2f centroid(0);
      for (auto& p : cells[i]) centroid += p;
      centroid /= (float)cells[i].size();

      Vec2f dir = centroid;  // vector from center to centroid
      dir.normalize();
      
      // Calculate individual speed for this cell
      float speedMult = 1.0f + rnd::uniform(-SPEED_VARIATION, SPEED_VARIATION) / BASE_DRIFT_SPEED;
      dir *= BASE_DRIFT_SPEED;

      voronoiCells.push_back(Cell{cells[i], dir, colors[i], speedMult});
    }
  }

  void onAnimate(double dt) override {
    for (auto& cell : voronoiCells) {
      for (auto& p : cell.points) {
        // Apply individual speed multiplier
        p += cell.velocity * cell.speedMultiplier;
      }
    }
  }

  void onDraw(Graphics& g) override {
    g.clear(0);  // Black background to make ice colors pop

    g.blending(true);
    g.blendAdd();  // Use additive blending for ice effect
    g.depthTesting(true);

    for (auto& cell : voronoiCells) {
      if (cell.points.size() < 3) continue;

      Mesh mesh(Mesh::POINTS);
      Vec2f center(0);
      for (auto& p : cell.points) center += p;
      center /= (float)cell.points.size();
      mesh.vertex(center);

      // sort points by angle for cleaner rendering
      std::vector<Vec2f> sorted = cell.points;
      std::sort(sorted.begin(), sorted.end(), [&](Vec2f a, Vec2f b) {
        return atan2(a.y - center.y, a.x - center.x) < atan2(b.y - center.y, b.x - center.x);
      });

      for (auto& p : sorted) mesh.vertex(p);
      mesh.vertex(sorted[0]); // close the fan
      g.color(cell.color);
      g.draw(mesh);
    }
  }

  bool onKeyDown(Keyboard const& k) override {
    return false;
  }
};

int main() {
  VoronoiApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.dimensions(1024, 768);
  app.title("Breaking Ice");
  app.fps(60);
  app.displayMode(Window::DEFAULT_BUF);
  app.start();
}
