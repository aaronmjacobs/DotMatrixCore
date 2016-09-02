#include "Constants.h"
#include "GLIncludes.h"
#include "Log.h"

#include "gbc/Cartridge.h"
#include "gbc/Device.h"

#if defined(RUN_TESTS)
#  include "test/CPUTester.h"
#endif // defined(RUN_TESTS)

#include "wrapper/platform/IOUtils.h"
#include "wrapper/platform/OSUtils.h"
#include "wrapper/video/Renderer.h"

#include <cstdlib>
#include <functional>
#include <vector>

namespace {

std::function<void(int, int)> framebufferCallback;
std::function<void(int, bool)> keyCallback;

void onFramebufferSizeChange(GLFWwindow *window, int width, int height) {
   if (framebufferCallback) {
      framebufferCallback(width, height);
   }
}

void onKeyChanged(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (keyCallback) {
      keyCallback(key, action != GLFW_RELEASE);
   }
}

void updateJoypadState(GBC::Joypad& joypadState, int key, bool enabled) {
   switch (key) {
      case GLFW_KEY_LEFT:
         joypadState.left = enabled;
         break;
      case GLFW_KEY_RIGHT:
         joypadState.right = enabled;
         break;
      case GLFW_KEY_UP:
         joypadState.up = enabled;
         break;
      case GLFW_KEY_DOWN:
         joypadState.down = enabled;
         break;
      case GLFW_KEY_S:
         joypadState.a = enabled;
         break;
      case GLFW_KEY_A:
         joypadState.b = enabled;
         break;
      case GLFW_KEY_Z:
         joypadState.select = enabled;
         break;
      case GLFW_KEY_X:
         joypadState.start = enabled;
         break;
   }
}

GLFWwindow* init() {
   int glfwInitRes = glfwInit();
   if (!glfwInitRes) {
      LOG_ERROR_MSG_BOX("Unable to initialize GLFW");
      return nullptr;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

   GLFWwindow *window = glfwCreateWindow(GBC::kScreenWidth, GBC::kScreenHeight, kProjectDisplayName, nullptr, nullptr);
   if (!window) {
      glfwTerminate();
      LOG_ERROR_MSG_BOX("Unable to create GLFW window");
      return nullptr;
   }

   glfwSetFramebufferSizeCallback(window, onFramebufferSizeChange);
   glfwSetKeyCallback(window, onKeyChanged);
   glfwMakeContextCurrent(window);
   glfwSwapInterval(1); // VSYNC

   int gladInitRes = gladLoadGL();
   if (!gladInitRes) {
      glfwDestroyWindow(window);
      glfwTerminate();
      LOG_ERROR_MSG_BOX("Unable to initialize glad");
      return nullptr;
   }

   return window;
}

} // namespace

uint8_t onSerial(uint8_t receivedVal) {
   static std::vector<char> vals;

   vals.push_back((char)receivedVal);

   vals.push_back('\0');
   LOG_DEBUG("serial:\n" << vals.data());
   vals.pop_back();

   return 0xFF;
}

int main(int argc, char *argv[]) {
   LOG_INFO(kProjectName << " " << kVersionMajor << "." << kVersionMinor << "."
      << kVersionMicro << " (" << kVersionBuild << ")");

   if (!OSUtils::fixWorkingDirectory()) {
      LOG_ERROR_MSG_BOX("Unable to set working directory to executable directory");
   }

   GLFWwindow *window = init();
   if (!window) {
      return EXIT_FAILURE;
   }

   int framebufferWidth = 0, framebufferHeight = 0;
   glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

   Renderer renderer(framebufferWidth, framebufferHeight);
   framebufferCallback = [&renderer](int width, int height) {
      renderer.onFramebufferSizeChange(width, height);
   };

   GBC::Joypad joypadState{};
   keyCallback = [&joypadState](int key, bool enabled) {
      updateJoypadState(joypadState, key, enabled);
   };

#if defined(RUN_TESTS)
   GBC::CPUTester cpuTester;
   for (int i = 0; i < 10; ++i) {
      cpuTester.runTests(true);
   }
#endif // defined(RUN_TESTS)

   GBC::Device device;
   //device.setSerialCallback(onSerial);

   // Try to load a cartridge
   if (argc > 1) {
      const char* cartPath = argv[1];
      std::vector<uint8_t> cartData = IOUtils::readBinaryFile(cartPath);

      LOG_INFO("Loading cartridge: " << cartPath);
      UPtr<GBC::Cartridge> cartridge(GBC::Cartridge::fromData(std::move(cartData)));

      if (cartridge) {
         glfwSetWindowTitle(window, cartridge->title());
         device.setCartridge(std::move(cartridge));
      }
   }

   static const double kMaxFrameTime = 0.25;
   static const double kDt = 1.0 / 60.0;
   double lastTime = glfwGetTime();
   double accumulator = 0.0;

   while (!glfwWindowShouldClose(window)) {
      double now = glfwGetTime();
      double frameTime = std::min(now - lastTime, kMaxFrameTime); // Cap the frame time to prevent spiraling
      lastTime = now;

      accumulator += frameTime;
      while (accumulator >= kDt) {
         device.setJoypadState(joypadState);
         device.tick(static_cast<float>(kDt));
         accumulator -= kDt;
      }

      renderer.draw(device.getLCDController().getFramebuffer());

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   glfwDestroyWindow(window);
   glfwTerminate();

   return EXIT_SUCCESS;
}
