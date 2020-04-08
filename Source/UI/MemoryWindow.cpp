#include "GBC/Memory.h"

#include "UI/UI.h"

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

namespace
{

struct MemoryHelper
{
   GBC::Memory* memory = nullptr;
   uint16_t address = 0;
};

ImU8 readMemory(const ImU8* data, std::size_t offset)
{
   const MemoryHelper* memoryHelper = reinterpret_cast<const MemoryHelper*>(data);

   return memoryHelper->memory->readDirect(memoryHelper->address + static_cast<uint16_t>(offset));
}

void writeMemory(ImU8* data, std::size_t offset, ImU8 value)
{
   MemoryHelper* memoryHelper = reinterpret_cast<MemoryHelper*>(data);

   memoryHelper->memory->writeDirect(memoryHelper->address + static_cast<uint16_t>(offset), value);
}

MemoryEditor createMemoryEditor()
{
   MemoryEditor memoryEditor;

   memoryEditor.ReadFn = readMemory;
   memoryEditor.WriteFn = writeMemory;

   return memoryEditor;
}

void renderMemoryRegion(GBC::Memory& memory, const char* name, const char* description, uint16_t size, uint16_t& address)
{
   if (ImGui::BeginTabItem(name))
   {
      ImGui::Text("%s", description);
      ImGui::Separator();

      MemoryHelper memoryHelper;
      memoryHelper.memory = &memory;
      memoryHelper.address = address;

      static MemoryEditor memoryEditor = createMemoryEditor();
      memoryEditor.DrawContents(&memoryHelper, size, address);

      ImGui::EndTabItem();
   }

   address += size;
}

} // namespace

void UI::renderMemoryWindow(GBC::Memory& memory) const
{
   ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(560.0f, 312.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Memory");

   if (ImGui::BeginTabBar("MemoryTabBar"))
   {
      uint16_t address = 0x0000;

      renderMemoryRegion(memory, "romb", "Permanently-mapped ROM bank", 0x4000, address);
      renderMemoryRegion(memory, "roms", "Switchable ROM bank", 0x4000, address);
      renderMemoryRegion(memory, "vram", "Video RAM", 0x2000, address);
      renderMemoryRegion(memory, "eram", "Switchable external RAM bank", 0x2000, address);
      renderMemoryRegion(memory, "ram0", "Working RAM bank 0", 0x1000, address);
      renderMemoryRegion(memory, "ram1", "Working RAM bank 1", 0x1000, address);
      renderMemoryRegion(memory, "ramm", "Mirror of working ram", 0x1E00, address);
      renderMemoryRegion(memory, "oam", "Sprite attribute table", 0x0100, address);
      renderMemoryRegion(memory, "io", "I/O device mappings", 0x0080, address);
      renderMemoryRegion(memory, "ramh", "High RAM area and interrupt enable register", 0x0080, address);

      ImGui::EndTabBar();
   }

   ImGui::End();
}
