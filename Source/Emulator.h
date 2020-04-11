#pragma once

#include "Core/Archive.h"
#include "Core/Pointers.h"

#if GBC_WITH_AUDIO
#include "Platform/Audio/AudioManager.h"
#endif // GBC_WITH_AUDIO
#include "Platform/Input/ControllerInputDevice.h"
#include "Platform/Input/KeyboardInputDevice.h"

#include "UI/UIFriend.h"

#include <readerwriterqueue.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class Renderer;

namespace GBC
{
class Cartridge;
class GameBoy;
} // namespace GBC

struct GLFWwindow;

struct SaveData
{
   Archive archive;
   std::string gameTitle;
};

struct Bounds
{
   int x = 0;
   int y = 0;
   int width = 0;
   int height = 0;
};

class Emulator
{
public:
   Emulator();
   ~Emulator();

   bool init();
   void tick(double dt);
   void render();
   bool shouldExit() const;

   void setRom(const char* romPath);

   void onFramebufferSizeChanged(int width, int height);
   void onFilesDropped(int count, const char* paths[]);
   void onKeyChanged(int key, int scancode, int action, int mods);
   void onWindowRefreshRequested();

private:
   DECLARE_UI_FRIEND

   void resetGameBoy(UPtr<GBC::Cartridge> cartridge);
   void toggleFullScreen();
   void loadGame();
   void saveGameAsync();
   void saveThreadMain();

   GLFWwindow* window;
   UPtr<GBC::GameBoy> gameBoy;
   UPtr<Renderer> renderer;
#if GBC_WITH_AUDIO
   AudioManager audioManager;
#endif // GBC_WITH_AUDIO

#if GBC_WITH_BOOTSTRAP
   std::vector<uint8_t> bootstrap;
#endif // GBC_WITH_BOOTSTRAP

#if GBC_WITH_UI
   UPtr<UI> ui;
   double timeScale;
   bool renderUi;
   bool skipNextTick;
#endif // GBC_WITH_UI

   KeyboardInputDevice keyboardInputDevice;
   ControllerInputDevice controllerInputDevice;

   bool cartWroteToRamLastFrame;

   std::thread saveThread;
   std::mutex saveThreadMutex;
   std::condition_variable saveThreadConditionVariable;
   std::atomic_bool exiting;
   moodycamel::ReaderWriterQueue<SaveData> saveQueue;

   Bounds savedWindowBounds;
};
