#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include "MyFunctions.h"

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


void ShowBar(const char* prefix, int pid)
{
    ImGui::PushID(pid);                // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
    ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.
    bool node_open = ImGui::TreeNode("Object", "%s_%u", prefix, pid);
    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("PPID");
    ImGui::NextColumn();
    if (node_open)
    {
        static float dummy_members[8] = { 0.0f,0.0f,1.0f,3.1416f,100.0f,999.0f };
        for (int thread_id = 0; thread_id < 8; thread_id++)
        {
            ImGui::PushID(thread_id); // Use field index as identifier.

            // Here we use a TreeNode to highlight on hover (we could use e.g. Selectable as well)
            ImGui::AlignTextToFramePadding();
            ImGui::TreeNodeEx("Field", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Field_%d", thread_id);
            ImGui::NextColumn();
            ImGui::SetNextItemWidth(-1);
            if (thread_id >= 5)
                ImGui::InputFloat("##value", &dummy_members[thread_id], 1.0f);
            else
                ImGui::DragFloat("##value", &dummy_members[thread_id], 0.01f);
            ImGui::NextColumn();

            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}
