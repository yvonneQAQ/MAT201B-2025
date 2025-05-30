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
  RtMidiIn midiIn;                     // MIDI input carrier

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
  navControl().active(navi);
  time += dt;  // <-- 记得加上这句，累积时间

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

if (time >=62.0f && time - lastMeteorTime >= 2.0f) {
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
  
    m.speed = al::rnd::uniform(0.5f, 1.0f);
   m.control = Vec3f(0.0f, y + al::rnd::uniform(-1.0f, 1.0f), -10.0f); // 中点弯曲
m.brightness = al::rnd::uniform(0.6f, 1.0f); // 用于亮度和透明度



;
    m.lifetime = 2.0f;

    meteorLines.push_back(m);
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

// 准备一个球体用于流星尾焰
Mesh stphere;
stphere.primitive(Mesh::TRIANGLES);
addSphere(stphere, 1.0f);

// Bézier 函数
auto bezier = [](const Vec3f &a, const Vec3f &b, const Vec3f &c, float t) -> Vec3f {
  return (1 - t) * (1 - t) * a + 2 * (1 - t) * t * b + t * t * c;
};

for (auto &m : meteorLines) {
  float t = m.progress / m.lifetime;
  if (t >= 1.0f) continue;

  // Bézier positions
  Vec3f pos = bezier(m.start, m.control, m.end, t);
  Vec3f nextPos = bezier(m.start, m.control, m.end, t + 0.25f);

  // direction and perpendicular vector for quad width
  Vec3f dir = (nextPos - pos).normalize();
  float taper = 1.0f - t;  // t 越大，尾部越细
  Vec3f perp = cross(dir, Vec3f(0, 1, 0)).normalize() * 0.1f * taper;


  float b = std::min(1.0f, m.brightness * 1.5f);


  // Construct quad (two triangles for gradient line)
  Mesh quad;
  quad.primitive(Mesh::TRIANGLES);

  Vec3f v1 = pos + perp;
  Vec3f v2 = pos - perp;
  Vec3f v3 = nextPos - perp;
  Vec3f v4 = nextPos + perp;

  // Gradient alpha trail
  quad.color(b, b, b, 1.0f); quad.vertex(v1);
  quad.color(b, b, b, 1.0f); quad.vertex(v2);
  quad.color(b, b, b, 0.0f); quad.vertex(v3);

  quad.color(b, b, b, 1.0f); quad.vertex(v1);
  quad.color(b, b, b, 0.0f); quad.vertex(v3);
  quad.color(b, b, b, 0.0f); quad.vertex(v4);

  g.draw(quad);

  // Glow sphere at nextPos
  g.pushMatrix();
  g.translate(nextPos);
  g.color(b, b, b, 0.7f);
  g.scale(0.15);
  g.draw(sphere);
  g.popMatrix();
}




    // Render the synth's graphics
    synthManager.render(g);
    // // Draw Spectrum
    mSpectrogram.reset();
    mSpectrogram.primitive(Mesh::LINE_STRIP);
    if (showSpectro)
    {
      for (int i = 0; i < FFT_SIZE / 2; i++)
      {
        mSpectrogram.color(HSV(0.5 - spectrum[i] * 100));
        mSpectrogram.vertex(i/2, spectrum[i]/20, 6.0);
      }
      g.meshColor(); // Use the color in the mesh
      g.pushMatrix();
      g.translate(-3.0, -3, -10);
      g.scale(10.0 / FFT_SIZE, 1000, 1.0);
      g.draw(mSpectrogram); g.translate(0, 0, -10);
      g.popMatrix();
    }
    // GUI is drawn here
    if (showGUI)
    {
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
