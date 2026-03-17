#include "/test/GUI.h"
#include "/test/Aimbot.h"
#include "/test/Movement.h"
#include "/test/RCS.h"
#include "/test/TriggerBot.h"
#include "/test/Radar.h"
#include "/test/SpectatorList.h"
#include "/test/BombTimer.h"
#include "/test/HitMarker.h"
#include "/test/Misc.h"
#include <Windows.h>
#include <fstream>
#include "config.h"

static void WriteINI(std::ofstream& f, const char* key, bool  val) { f << key << "=" << (val ? "1" : "0") << "\n"; }
static void WriteINI(std::ofstream& f, const char* key, int   val) { f << key << "=" << val << "\n"; }
static void WriteINI(std::ofstream& f, const char* key, float val) { f << key << "=" << val << "\n"; }

static std::string ReadValue(const std::string& line, const std::string& key) {
    if (line.rfind(key + "=", 0) == 0) return line.substr(key.size() + 1);
    return "";
}

void Config::Init(HMODULE hModule) {
    char path[MAX_PATH];
    GetModuleFileNameA(hModule, path, MAX_PATH);
    std::string s(path);
    auto pos = s.find_last_of("\\/");
    g_DllDir = (pos != std::string::npos) ? s.substr(0, pos + 1) : "";
}

void Config::Save(const std::string& name) {
    std::ofstream f(g_DllDir + name + ".ini");
    if (!f.is_open()) return;

    f << "[ESP]\n";
    WriteINI(f, "box", MyNamespace::visuals::box);
    WriteINI(f, "skeleton", MyNamespace::visuals::skeleton);
    WriteINI(f, "name", MyNamespace::visuals::name);
    WriteINI(f, "healthbar", MyNamespace::visuals::healthbar);
    WriteINI(f, "weapon", MyNamespace::visuals::weapon);
    WriteINI(f, "reload", MyNamespace::visuals::reload);
    WriteINI(f, "viewDirection", MyNamespace::visuals::viewDirection);

    f << "[Chams]\n";
    WriteINI(f, "chams", MyNamespace::visuals::chams);
    WriteINI(f, "teamchams", MyNamespace::visuals::teamchams);

    f << "[Aimbot]\n";
    WriteINI(f, "aimbot", MyNamespace::visuals::Aimbot);
    WriteINI(f, "aimKey", Aimbot::aimKey);
    WriteINI(f, "boneTarget", Aimbot::boneTarget);
    WriteINI(f, "aimFov", Aimbot::aimFov);
    WriteINI(f, "smooth", Aimbot::smooth);
    WriteINI(f, "showFov", Aimbot::showFov);
    WriteINI(f, "silentAim", Aimbot::silentAim);
    WriteINI(f, "damagePriority", Aimbot::damagePriority);
    WriteINI(f, "minDamage", Aimbot::minDamage);

    f << "[RCS]\n";
    WriteINI(f, "rcsEnabled", RCS::enabled);
    WriteINI(f, "rcsScaleX", RCS::scaleX);
    WriteINI(f, "rcsScaleY", RCS::scaleY);

    f << "[TriggerBot]\n";
    WriteINI(f, "triggerEnabled", TriggerBot::enabled);
    WriteINI(f, "triggerHotkey", TriggerBot::hotkey);
    WriteINI(f, "triggerDelayMs", TriggerBot::delayMs);
    WriteINI(f, "triggerHeadshotOnly", TriggerBot::headshotOnly);

    f << "[Radar]\n";
    WriteINI(f, "radarEnabled", Radar::enabled);
    WriteINI(f, "radarSize", Radar::size);
    WriteINI(f, "radarRange", Radar::range);
    WriteINI(f, "radarPosX", Radar::posX);
    WriteINI(f, "radarPosY", Radar::posY);
    WriteINI(f, "radarShowNames", Radar::showNames);

    f << "[Misc]\n";
    WriteINI(f, "spectator", SpectatorList::enabled);
    WriteINI(f, "bombTimer", BombTimer::enabled);
    WriteINI(f, "hitMarker", HitMarker::enabled);
    WriteINI(f, "hmDuration", HitMarker::duration);
    WriteINI(f, "hmSize", HitMarker::size);
    WriteINI(f, "watermark", Misc::watermark);
    WriteINI(f, "cornerBox", Misc::cornerBox);
    WriteINI(f, "panicEnabled", Misc::panicEnabled);

    f << "[Movement]\n";
    WriteINI(f, "bunnyHop", Movement::bunnyHop);
    WriteINI(f, "autoStrafe", Movement::autoStrafe);
}

void Config::Load(const std::string& name) {
    std::ifstream f(g_DllDir + name + ".ini");
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        std::string v;

        // ESP
        if (!(v = ReadValue(line, "box")).empty())           MyNamespace::visuals::box = v == "1";
        if (!(v = ReadValue(line, "skeleton")).empty())      MyNamespace::visuals::skeleton = v == "1";
        if (!(v = ReadValue(line, "name")).empty())          MyNamespace::visuals::name = v == "1";
        if (!(v = ReadValue(line, "healthbar")).empty())     MyNamespace::visuals::healthbar = v == "1";
        if (!(v = ReadValue(line, "weapon")).empty())        MyNamespace::visuals::weapon = v == "1";
        if (!(v = ReadValue(line, "reload")).empty())        MyNamespace::visuals::reload = v == "1";
        if (!(v = ReadValue(line, "viewDirection")).empty()) MyNamespace::visuals::viewDirection = v == "1";

        // Chams
        if (!(v = ReadValue(line, "chams")).empty())         MyNamespace::visuals::chams = v == "1";
        if (!(v = ReadValue(line, "teamchams")).empty())     MyNamespace::visuals::teamchams = v == "1";

        // Aimbot
        if (!(v = ReadValue(line, "aimbot")).empty())         MyNamespace::visuals::Aimbot = v == "1";
        if (!(v = ReadValue(line, "aimKey")).empty())         Aimbot::aimKey = std::stoi(v);
        if (!(v = ReadValue(line, "boneTarget")).empty())     Aimbot::boneTarget = std::stoi(v);
        if (!(v = ReadValue(line, "aimFov")).empty())         Aimbot::aimFov = std::stof(v);
        if (!(v = ReadValue(line, "smooth")).empty())         Aimbot::smooth = std::stof(v);
        if (!(v = ReadValue(line, "showFov")).empty())        Aimbot::showFov = v == "1";
        if (!(v = ReadValue(line, "silentAim")).empty())      Aimbot::silentAim = v == "1";
        if (!(v = ReadValue(line, "damagePriority")).empty()) Aimbot::damagePriority = v == "1";
        if (!(v = ReadValue(line, "minDamage")).empty())      Aimbot::minDamage = std::stof(v);

        // RCS
        if (!(v = ReadValue(line, "rcsEnabled")).empty()) RCS::enabled = v == "1";
        if (!(v = ReadValue(line, "rcsScaleX")).empty())  RCS::scaleX = std::stof(v);
        if (!(v = ReadValue(line, "rcsScaleY")).empty())  RCS::scaleY = std::stof(v);

        // TriggerBot
        if (!(v = ReadValue(line, "triggerEnabled")).empty())      TriggerBot::enabled = v == "1";
        if (!(v = ReadValue(line, "triggerHotkey")).empty())       TriggerBot::hotkey = std::stoi(v);
        if (!(v = ReadValue(line, "triggerDelayMs")).empty())      TriggerBot::delayMs = std::stoi(v);
        if (!(v = ReadValue(line, "triggerHeadshotOnly")).empty()) TriggerBot::headshotOnly = v == "1";

        // Radar
        if (!(v = ReadValue(line, "radarEnabled")).empty())   Radar::enabled = v == "1";
        if (!(v = ReadValue(line, "radarSize")).empty())      Radar::size = std::stof(v);
        if (!(v = ReadValue(line, "radarRange")).empty())     Radar::range = std::stof(v);
        if (!(v = ReadValue(line, "radarPosX")).empty())      Radar::posX = std::stof(v);
        if (!(v = ReadValue(line, "radarPosY")).empty())      Radar::posY = std::stof(v);
        if (!(v = ReadValue(line, "radarShowNames")).empty()) Radar::showNames = v == "1";

        // Misc
        if (!(v = ReadValue(line, "spectator")).empty())    SpectatorList::enabled = v == "1";
        if (!(v = ReadValue(line, "bombTimer")).empty())    BombTimer::enabled = v == "1";
        if (!(v = ReadValue(line, "hitMarker")).empty())    HitMarker::enabled = v == "1";
        if (!(v = ReadValue(line, "hmDuration")).empty())   HitMarker::duration = std::stof(v);
        if (!(v = ReadValue(line, "hmSize")).empty())       HitMarker::size = std::stoi(v);
        if (!(v = ReadValue(line, "watermark")).empty())    Misc::watermark = v == "1";
        if (!(v = ReadValue(line, "cornerBox")).empty())    Misc::cornerBox = v == "1";
        if (!(v = ReadValue(line, "panicEnabled")).empty()) Misc::panicEnabled = v == "1";

        // Movement
        if (!(v = ReadValue(line, "bunnyHop")).empty())   Movement::bunnyHop = v == "1";
        if (!(v = ReadValue(line, "autoStrafe")).empty()) Movement::autoStrafe = v == "1";
    }
}