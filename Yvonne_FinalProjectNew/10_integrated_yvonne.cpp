// MUS109IA & MAT276IA.
// Spring 2022
// Course Instrument 10. Integrated Instrument from 01 to 09 Synth with Visaul (Meshs and Spectrum)
// Myungin Lee

// Press '[' or ']' to turn on & off GUI
// '=' to navigate pov
// Able to play with MIDI device
// To change the default instrument, change <FMWT> in line 43 to .. 
// <SineEnv>, <OscEnv>, <Vib>, <FM>, <OscAM>, <OscTrm>, <AddSyn>, <Sub>, or <PluckedString>

#include <cstdio> // for printing to stdout
#include <algorithm> // for std::clamp

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"
#include "_instrument_classes_Yvonne.cpp"

using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

class MyApp : public App, public MIDIMessageHandler
{
public:
  // To change the default instrument, change <FMWT> to .. 
  // <SineEnv>, <OscEnv>, <Vib>, <FM>, <OscAM>, <OscTrm>, <AddSyn>, <Sub>, or <PluckedString>
  // ***** This is the only line to change the default instrument
  SynthGUIManager<Sub> synthManager{"Integrated"}; 


  //you can actually 

  //    ParameterMIDI parameterMIDI;
  int midiNote;
  float harmonicSeriesScale[20];
  float halfStepScale[20];
  float halfStepInterval = 1.05946309; // 2^(1/12)
  RtMidiIn midiIn;    
  float zoomTime = 0.0f;
                 // MIDI input carrier

  Mesh mSpectrogram;
  vector<float> spectrum;
  bool showGUI = true;
  bool showSpectro = true;
  bool navi = false;
  float time;
  float lastMeteorTime = 0.0f;

  gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);

  // 定义结构体
struct LightDot {
  Vec3f pos;
  float brightness;
  float targetBrightness;
  float phase;
};

struct Raindrop {
    Vec3f pos;
    float speed;
};

std::vector<Raindrop> raindrops;
float rainSpawnTimer = 0.0f;
const float rainspawnInterval = 0.01f;
bool rainActive = false;


struct MeteorLine {
  Vec3f start, control, end;
  float progress = 0.0f;
  float speed = 0.3f;   // fast
  float lifetime = 3.0f;
  float brightness = 1.0f;
};
std::vector<MeteorLine> meteorLines;
double meteorSpawnTimer = 0.0;
double meteorSpawnInterval = 2.0;


template <typename T>
T clamp(T val, T minVal, T maxVal) {
  return std::max(minVal, std::min(val, maxVal));
}


std::vector<LightDot> lightDots;




  virtual void onInit() override
  {
    // Check for connected MIDI devices
    if (midiIn.getPortCount() > 0)
    {
      // Bind ourself to the RtMidiIn object, to have the onMidiMessage()
      // callback called whenever a MIDI message is received
      MIDIMessageHandler::bindTo(midiIn);

      // Open the last device founds
      unsigned int port = midiIn.getPortCount() - 1;
      midiIn.openPort(port);
      printf("Opened port to %s\n", midiIn.getPortName(port).c_str());
    }
    else
    {
      printf("Error: No MIDI devices found.\n");
    }
    // Declare the size of the spectrum
    spectrum.resize(FFT_SIZE / 2 + 1);

    imguiInit();
    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
  }

  void onCreate() override
  {
   synthManager.synthSequencer().playSequence("final_project_Yvonne.synthSequence");

    // Play example sequence. Comment this line to start from scratch
    synthManager.synthRecorder().verbose(true);
        // Add another class used
        synthManager.synth().registerSynthClass<SineEnv>();
        synthManager.synth().registerSynthClass<OscEnv>();
        synthManager.synth().registerSynthClass<Vib>();
        synthManager.synth().registerSynthClass<FM>();
        synthManager.synth().registerSynthClass<FMWT>();
        synthManager.synth().registerSynthClass<OscAM>();
        synthManager.synth().registerSynthClass<OscTrm>();
        synthManager.synth().registerSynthClass<AddSyn>();
        synthManager.synth().registerSynthClass<Sub>();
        synthManager.synth().registerSynthClass<PluckedString>();

lastMeteorTime = 0.0f;

  // 初始化 lightDots
  for (int i = 0; i < 600; i++) {
    float theta = al::rnd::uniform(0.0f, 6.28f);
    float phi = acos(al::rnd::uniform(-1.0f, 1.0f));
    float r = 3.0;

    float x = r * sin(phi) * cos(theta);
    float y = r * sin(phi) * sin(theta);
    float z = r * cos(phi);

    LightDot dot;
    dot.pos = Vec3f(x, y, z);
    dot.brightness = al::rnd::uniform(1.0f, 5.0f);
    dot.targetBrightness = dot.brightness;
    dot.phase = al::rnd::uniform(0.0f, 6.28f);

    lightDots.push_back(dot);
  }
}

  

  void onSound(AudioIOData &io) override
  {
    synthManager.render(io); // Render audio
    // STFT
    while (io())
    {
      if (stft(io.out(0)))
      { // Loop through all the frequency bins
        for (unsigned k = 0; k < stft.numBins(); ++k)
        {
          // Here we simply scale the complex sample
          spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
          // spectrum[k] = stft.bin(k).real();
        }
      }
    }
  }

void onAnimate(double dt) override {
    time += dt;

    // Rain logic - completely separate from meteor timing
    if (time > 102.0f) {  // Start rain after camera zoom
        if (!rainActive) {
            rainActive = true;
            rainSpawnTimer = 0.0f;  // Reset timer when rain first starts
            std::cout << "Rain started at time: " << time << std::endl;
        }

        // Spawn new drops at regular intervals
        rainSpawnTimer += dt;
        while (rainSpawnTimer >= rainspawnInterval) {
            rainSpawnTimer -= rainspawnInterval;

            // Add more drops per interval
            int count = al::rnd::uniformi(5, 10);  // Increased number of drops
            for (int i = 0; i < count; ++i) {
                Raindrop r;
                r.pos = Vec3f(
                    al::rnd::uniform(-8.0f, 8.0f),    // Wider X spread
                    al::rnd::uniform(6.0f, 8.0f),     // Start higher
                    al::rnd::uniform(-12.0f, -8.0f)   // Wider Z spread
                );
                r.speed = al::rnd::uniform(3.0f, 6.0f);  // Faster speed
                raindrops.push_back(r);
            }
            std::cout << "Spawned " << count << " raindrops. Total drops: " << raindrops.size() << std::endl;
        }

        // Update drop positions
        for (auto& r : raindrops) {
            r.pos.y -= r.speed * dt;
        }

        // Remove drops that fall below view
        raindrops.erase(
            std::remove_if(raindrops.begin(), raindrops.end(),
                          [](const Raindrop& r) { return r.pos.y < -6.0f; }),
            raindrops.end()
        );
    }

    // Meteor logic - completely separate
    if (time >= 62.0f && time - lastMeteorTime >= 2.0f) {
        lastMeteorTime = time;
        std::cout << "Meteor launched at time: " << time << std::endl;

        // 发射一个流星线
        MeteorLine m;
        float y = al::rnd::uniform(-2.0f, 2.0f);
        float z = al::rnd::uniform(-2.0f, 2.0f);
        float direction = (al::rnd::uniform() > 0.5f) ? 1.0f : -1.0f;

        m.start = Vec3f(-6.0f * direction, y, -10);
        m.end = Vec3f(6.0f * direction, y, -10);
        // Slightly curve the path in the Z direction or upward
        Vec3f mid = (m.start + m.end) * 0.5f;
      
        m.speed = al::rnd::uniform(1.8f, 3.0f);  // Slower speed

        m.control = Vec3f(
        0.0f,
        y + (al::rnd::uniform() > 0.5f) ? 1.0f : -1.0f * al::rnd::uniform(2.0f, 6.0f),  // lift/bend the curve more
        -10.0f);

        m.brightness = al::rnd::uniform(8.0f, 12.0f);  // Much brighter
        m.lifetime = 3.0f;  // Longer lifetime

        meteorLines.push_back(m);
    }

    // Camera zoom logic
    if (time > 62.0f && time <= 102.0f) {
        float t = (time - 62.0f) / 40.0f;   // Zoom in over 40 seconds
        t = std::min(t, 1.0f);             // Clamp to 1.0
        t = t * t * (3 - 2 * t);           // Smoothstep easing
        float z = 2.0f + (-20.0f * t);     // 2.0 → -18.0
        nav().pos(0, 0, z);
    }
    else if (time > 102.0f && time <= 142.0f) {  // Zoom out over 40 seconds
        float t = (time - 102.0f) / 40.0f;  // t goes from 0 to 1
        t = std::min(t, 1.0f);
        t = t * t * (3 - 2 * t);  // smoothstep
        float z = -18.0f + (20.0f * t);  // -18.0 → 2.0
        nav().pos(0, 0, z);
    }

  
 for (auto &dot : lightDots) {
  float accel = 1.0f;   // 加速度（用于位置 jitter）
  float phaseMult = 1.0f;  // 闪烁速度倍数

if (time < 40.0f) {
  accel = 1.0f;
  phaseMult = 1.0f;
}
else if (time < 60.0f) {
  float t = (time - 40.0f) / 20.0f;  // t 从 0 → 1，跨越 40 到 60 秒
  t = t * t;
  accel = 1.0f + t * (10.0f - 1.0f);       // 从 1 → 10
  phaseMult = 1.0f + t * (10.0f - 1.0f);   // 同上
}
else {
  accel = 10.0f;
  phaseMult = 10.0f;

}




  // Jitter 位置
  dot.pos += Vec3f(
    al::rnd::uniformS(0.005f),
    al::rnd::uniformS(0.005f),
    al::rnd::uniformS(0.005f)
  ) * accel;

  // 更新 phase
  dot.phase += dt * al::rnd::uniform(0.5f, 1.0f) * phaseMult;

  // 更新 brightness
  dot.brightness = 4.0f + 0.4f * sin(dot.phase) + al::rnd::uniformS(0.1f);
  dot.brightness = clamp(dot.brightness, 3.0f, 5.0f);
}
if (time >= 60.0f && !lightDots.empty()) {
  lightDots.clear();  // 删除所有点
}

// update progress
for (auto &m : meteorLines) {
  m.progress += m.speed * dt;
}

// erase finished meteors
meteorLines.erase(std::remove_if(meteorLines.begin(), meteorLines.end(),
  [](const MeteorLine &m) { return m.progress > m.lifetime; }),
  meteorLines.end());


    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
}


  void onDraw(Graphics &g) override
  {
    g.clear();
    // 绘制固定的闪烁球体
    g.pushMatrix();
    g.translate(0, 0, -10);

    Mesh sphere;
    sphere.primitive(Mesh::TRIANGLES);
    addSphere(sphere, 1.0);

    if (time < 60.0) {
      for (const auto &dot : lightDots) {
        g.pushMatrix();
        g.translate(dot.pos);
        g.color(dot.brightness, dot.brightness, dot.brightness);
        g.scale(0.004);
        g.draw(sphere);
        g.popMatrix();
      }
    }
    g.popMatrix();
    
    // 启用透明混合模式
    g.blending(true);
    g.blendAdd();

    // Bézier 函数
    auto bezier = [](const Vec3f &a, const Vec3f &b, const Vec3f &c, float t) -> Vec3f {
      return (1 - t) * (1 - t) * a + 2 * (1 - t) * t * b + t * t * c;
    };

    for (auto &m : meteorLines) {
      float t = m.progress / m.lifetime;
      if (t >= 1.0f) continue;

      // Calculate camera-relative position
      float zPos;
      if (time <= 142.0f) {
          float t = (time - 102.0f) / 40.0f;  // t goes from 0 to 1
          t = std::min(t, 1.0f);
          t = t * t * (3 - 2 * t);  // smoothstep
          zPos = -18.0f + (20.0f * t);  // -18.0 → 2.0
      } else {
          zPos = 2.0f;  // Final camera position
      }

      g.pushMatrix();
      g.translate(0, 0, zPos - 10);  // Match camera position

      // Set rendering states for maximum brightness
      g.lighting(false);  // Disable lighting
      g.depthTesting(false);  // Disable depth testing
      g.blending(true);
      g.blendAdd();  // Use additive blending for maximum brightness

      // Draw trail with multiple line segments
      int segments = 24;  // More segments for smoother line
      float step = 0.0008f;  // Smaller step for smoother line

      for (int i = 0; i < segments; ++i) {
        float ti = t - i * step;
        float tiNext = ti - step;

        if (ti < 0.0f || tiNext < 0.0f) continue;

        Vec3f p1 = bezier(m.start, m.control, m.end, ti);
        Vec3f p2 = bezier(m.start, m.control, m.end, tiNext);

        // Create a thin line segment
        Mesh line;
        line.primitive(Mesh::LINES);
        
        float brightness = m.brightness * (1.0f - (float)i / segments);  // Fade out along trail
        line.color(brightness, brightness, brightness, 1.0f);
        
        // Add the line segment
        line.vertex(p1);
        line.vertex(p2);

        g.draw(line);
      }

      // Restore rendering states
      g.lighting(true);
      g.depthTesting(true);
      g.popMatrix();
    }

    // Draw rain - moved outside meteor loop
    if (time > 102.0f) {  // Only draw rain after it starts
        g.pushMatrix();
        // Calculate z position based on camera zoom
        float zPos;
        if (time <= 142.0f) {
            float t = (time - 102.0f) / 40.0f;  // t goes from 0 to 1
            t = std::min(t, 1.0f);
            t = t * t * (3 - 2 * t);  // smoothstep
            zPos = -18.0f + (20.0f * t);  // -18.0 → 2.0
        } else {
            zPos = 2.0f;  // Final camera position
        }
        g.translate(0, 0, zPos - 10);  // Position rain relative to camera
        
        // Reset and set rendering states for maximum brightness
        g.lighting(false);  // Disable lighting
        g.depthTesting(false);  // Disable depth testing
        g.blending(true);
        g.blendAdd();  // Use additive blending for maximum brightness
        
        Mesh rainMesh;
        rainMesh.primitive(Mesh::POINTS);

        for (const auto& r : raindrops) {
            rainMesh.vertex(r.pos);
            // Super bright white color with even higher values
            rainMesh.color(6.0f, 6.0f, 6.0f, 1.0f);
        }

        g.pointSize(1.0f);
        g.draw(rainMesh);
        
        // Restore rendering states
        g.lighting(true);
        g.depthTesting(true);
        g.popMatrix();
    }

    // Render the synth's graphics
    synthManager.render(g);
    
    // Draw Spectrum
    //mSpectrogram.reset();
    //mSpectrogram.primitive(Mesh::LINE_STRIP);
    //if (showSpectro) {
      //for (int i = 0; i < FFT_SIZE / 2; i++) {
        //mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
        //mSpectrogram.vertex(i/2, spectrum[i]/20, 6.0);
      //}
      g.meshColor(); // Use the color in the mesh
      g.pushMatrix();
      g.translate(-3.0, -3, -10);
      g.scale(10.0 / FFT_SIZE, 1000, 1.0);
      g.draw(mSpectrogram);
      g.popMatrix();
    
    
    // GUI is drawn here
    if (showGUI) {
      imguiDraw();
    }
  }
  // This gets called whenever a MIDI message is received on the port
  void onMIDIMessage(const MIDIMessage &m)
  {
    switch (m.type())
    {
    case MIDIByte::NOTE_ON:
    {
      int midiNote = m.noteNumber();
      if (midiNote > 0 && m.velocity() > 0.001)
      {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
        synthManager.voice()->setInternalParameterValue(
            "attackTime", 0.01 / m.velocity());
        synthManager.triggerOn(midiNote);
      }
      else
      {
        synthManager.triggerOff(midiNote);
      }
      break;
    }
    case MIDIByte::NOTE_OFF:
    {
      int midiNote = m.noteNumber();
      synthManager.triggerOff(midiNote);
      break;
    }
    default:;
    }
  }
  bool onKeyDown(Keyboard const &k) override
  {
    if (ParameterGUI::usingKeyboard())
    { // Ignore keys if GUI is using them
      return true;
    }
    if (!navi)
    {
      if (k.shift())
      {
        // If shift pressed then keyboard sets preset
        int presetNumber = asciiToIndex(k.key());
        synthManager.recallPreset(presetNumber);
      }
      else
      {
        // Otherwise trigger note for polyphonic synth
        int midiNote = asciiToMIDI(k.key());
        if (midiNote > 0)
        {
          synthManager.voice()->setInternalParameterValue(
              "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
          synthManager.triggerOn(midiNote);
        }
      }
    }
    switch (k.key())
    {
    case ']':
      showGUI = !showGUI;
      break;
    case '[':
      showSpectro = !showSpectro;
      break;
    case '=':
      navi = !navi;
      break;
    }
    return true;
  }

  bool onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0)
    {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }
};

int main()
{
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
}
