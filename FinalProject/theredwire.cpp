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
  Vec2f centroid;  // Store centroid for quick distance checks
};

struct VoronoiApp : App {
  vector<Vec2f> points;
  vector<Color> colors;
  vector<vector<Vec2f>> cells;
  std::vector<Cell> voronoiCells;
  struct FragmentPair {
    int cell1;
    int cell2;
    int numLines;  // Random number between 50 and 300
  };
  std::vector<FragmentPair> furthestPairs;  // Store three pairs
  
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

  void updateFurthestPairs() {
    furthestPairs.clear();
    std::vector<FragmentPair> allPairs;
    
    // Collect all possible pairs
    for (int i = 0; i < voronoiCells.size(); ++i) {
      for (int j = i + 1; j < voronoiCells.size(); ++j) {
        allPairs.push_back({i, j, (int)rnd::uniform(10, 100)});
      }
    }
    
    // Randomly shuffle all pairs
    for (int i = allPairs.size() - 1; i > 0; i--) {
      int j = rnd::uniform(i + 1);
      std::swap(allPairs[i], allPairs[j]);
    }
    
    // Take first three pairs (or less if not enough pairs)
    int numPairs = std::min(3, (int)allPairs.size());
    for (int i = 0; i < numPairs; ++i) {
      furthestPairs.push_back(allPairs[i]);
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

      Cell newCell{cells[i], dir, colors[i], speedMult, centroid};
      voronoiCells.push_back(newCell);
    }
  }

  void onAnimate(double dt) override {
    for (auto& cell : voronoiCells) {
      for (auto& p : cell.points) {
        p += cell.velocity * cell.speedMultiplier;
      }
      cell.centroid += cell.velocity * cell.speedMultiplier;
    }
    updateFurthestPairs();
  }

  void drawConnectingLines(Graphics& g, const Cell& cell1, const Cell& cell2, int numLines) {
    g.color(1, 0, 0, 0.3);  // Red color with some transparency
    g.lineWidth(1);  // Thin lines

    float noise_scale1 = cell1.points.size() > 0 ? (cell1.points[0] - cell1.centroid).mag() * 0.3f : 0.1f;
    float noise_scale2 = cell2.points.size() > 0 ? (cell2.points[0] - cell2.centroid).mag() * 0.3f : 0.1f;

    for (int i = 0; i < numLines; ++i) {
      float t = float(i) / (numLines - 1);
      
      Vec2f noise_start(
        rnd::uniform(-noise_scale1, noise_scale1),
        rnd::uniform(-noise_scale1, noise_scale1)
      );
      Vec2f noise_end(
        rnd::uniform(-noise_scale2, noise_scale2),
        rnd::uniform(-noise_scale2, noise_scale2)
      );
      
      Vec2f start = cell1.centroid + noise_start;
      Vec2f end = cell2.centroid + noise_end;
      Vec2f mid = (start + end) * 0.5f;
      
      float offset = sin(t * M_PI) * 0.2f;
      mid += Vec2f(
        cos(t * 2 * M_PI + al::rnd::uniform()) * offset,
        sin(t * 2 * M_PI + al::rnd::uniform()) * offset
      );

      Mesh mesh;
      mesh.primitive(Mesh::LINE_STRIP);
      mesh.vertex(start);
      mesh.vertex(mid);
      mesh.vertex(end);
      
      g.draw(mesh);
    }
  }

  void onDraw(Graphics& g) override {
    g.clear(0);  // Black background to make ice colors pop

    g.blending(true);
    g.blendAdd();  // Use additive blending for ice effect
    g.depthTesting(true);

    // Draw all connecting lines first
    if (voronoiCells.size() >= 2) {
      for (const auto& pair : furthestPairs) {
        drawConnectingLines(g, voronoiCells[pair.cell1], voronoiCells[pair.cell2], pair.numLines);
      }
    }

    // Draw the cells
    for (auto& cell : voronoiCells) {
      if (cell.points.size() < 3) continue;

      Mesh mesh(Mesh::TRIANGLE_FAN);
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
