#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include "GUI_Processes_Functions.h"
#include <codecvt>

// convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}


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


void ShowProcessOnList(Process process)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::Columns(6);
    ImGui::Separator();

    ImGui::PushID((int)process.ProcessID());
    ImGui::AlignTextToFramePadding();

    
    // Process Time
    bool node_open = ImGui::TreeNode("","%02d:%02d:%02d.%03d",process.ST().wHour,
        process.ST().wMinute,
        process.ST().wSecond,
        process.ST().wMilliseconds);

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    // PID
    ImGui::Text("%X (%d)", process.ProcessID(), process.ProcessID());
    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    // PPID
    ImGui::Text("%X (%d)", process.ParentprocessID(), process.ParentprocessID());

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    // ImageFileName
    ImGui::Text("%s", wstring_to_utf8(process.ImageFileName()).c_str());

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    // CommandLine
    ImGui::Text("%s", wstring_to_utf8(process.Command()).c_str());

    ImGui::NextColumn();
    ImGui::AlignTextToFramePadding();
    // State
    ImGui::Text("%s", process.State() == ItemType::ProcessCreate ? "RUNNING" : "EXITED");

    if (node_open)
    {
        for (auto thread : process.ThreadList())
        {
            ShowThreadOnList(thread);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::PopStyleVar();
}


void ShowThreadOnList(Thread thread)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::Columns(3);
    ImGui::Separator();


    ImGui::PushID((int)thread.ThreadID());
    ImGui::AlignTextToFramePadding();

    ImGui::TreeNodeEx("Field", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%02d:%02d:%02d.%03d", thread.ST().wHour, thread.ST().wMinute, thread.ST().wSecond, thread.ST().wMilliseconds);
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::Text("%X (%d)", thread.ThreadID());
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1);
    ImGui::Text("%s", thread.State() == ItemType::ThreadCreate ? "RUNNING" : "EXITED");

    ImGui::PopID();

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::PopStyleVar();
}


void RemoveProcessFromList(Process process)
{}


void RemoveThreadFromList(Thread thread)
{}


void ShowBar(const char* prefix, int pid, bool show_less)
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

        size_t max = 8;
        if (show_less)
            max = 4;
        else
            max = 8;

        for (size_t thread_id = 0; thread_id < max; thread_id++)
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
