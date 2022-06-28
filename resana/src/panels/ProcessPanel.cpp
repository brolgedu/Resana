#include "ProcessPanel.h"
#include "rspch.h"

#include "core/Application.h"

#include "imgui/ImGuiHelpers.h"
#include <imgui.h>

#include <mutex>

namespace RESANA {

ProcessPanel::ProcessPanel() = default;

ProcessPanel::~ProcessPanel() = default;

void ProcessPanel::OnAttach()
{
    mUpdateInterval = TimeTick::Rate::Normal;
    mPanelOpen = false;

    mProcessManager = ProcessManager::Get();
    mProcessManager->SetUpdateInterval(mUpdateInterval);
}

void ProcessPanel::OnDetach()
{
    mPanelOpen = false;
    ProcessManager::Shutdown();
}

void ProcessPanel::OnUpdate(const Timestep ts)
{
    mUpdateInterval = (uint32_t)ts;

    if (IsPanelOpen()) {
        ProcessManager::Run();
        mProcessManager->SetUpdateInterval(mUpdateInterval);
    }

    GetProcesses(mDataCache);
}

void ProcessPanel::OnImGuiRender()
{
}

void ProcessPanel::GetProcesses(ProcessContainer& dest) const
{
    const auto& app = Application::Get();
    auto& threadPool = app.GetThreadPool();
    threadPool.Queue([&] {
        if (const auto& data = mProcessManager->GetData()) {
            if (data->GetNumEntries() > 0) {
                // Make a deep copy
                uint32_t backupId = -1;
                if (const auto selected = mDataCache.GetSelectedEntry()) {
                    backupId = selected->GetProcessId(); // Remember selected processId
                }

                dest.Copy(data.get());
                dest.SelectEntry(backupId); // Set selected process (if any)
            }
            mProcessManager->ReleaseData();
        }
    });
}

void ProcessPanel::ShowPanel(bool* pOpen)
{
    if ((mPanelOpen = *pOpen)) {
        if (ImGui::BeginChild("Details", ImGui::GetContentRegionAvail())) {
            ProcessManager::Run();
            ShowProcessTable();
        }
        ImGui::EndChild();

    } else {
        ProcessManager::Stop();
    }
}

void ProcessPanel::SetUpdateInterval(Timestep interval)
{
    mUpdateInterval = (uint32_t)interval;
}

void ProcessPanel::ShowProcessTable()
{
    static bool showProcessID = true;
    static bool showParentProcessID = false;
    static bool showModuleID = false;
    static bool showMemoryUsage = true;
    static bool showThreadCount = true;
    static bool showPriorityClass = false;

    ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.0f, 0.0f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, { 1.0f, 1.0f, 1.0f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, { 1.0f, 1.0f, 1.0f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, { 1.0f, 1.0f, 1.0f, 1.0f });

    if (ImGui::BeginMenu("View")) {
        ImGui::Checkbox("Process ID", &showProcessID);
        ImGui::Checkbox("Parent Process ID", &showParentProcessID);
        ImGui::Checkbox("Module ID", &showModuleID);
        ImGui::Checkbox("Memory Usage", &showMemoryUsage);
        ImGui::Checkbox("Thread Count", &showThreadCount);
        ImGui::Checkbox("Priority Class", &showPriorityClass);
        ImGui::EndMenu();
    }

    const bool viewOptions[] = {
        showProcessID,
        showParentProcessID,
        showModuleID,
        showMemoryUsage,
        showThreadCount,
        showPriorityClass
    };

    int numColumns = 1;
    for (const bool viewOption : viewOptions) {
        numColumns += viewOption ? 1 : 0;
    }

    static ImGuiTableFlags flags = ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_NoSavedSettings;

    const auto outerSize = ImVec2(-1.0f, ImGui::GetContentRegionAvail().y);

    if (ImGui::BeginTable("proc_table", numColumns, flags, outerSize)) {
        static bool showColWidthRow;
        showColWidthRow = true;

        constexpr int freezeCols = 0, freezeRows = 1;
        ImGui::TableSetupScrollFreeze(freezeCols, freezeRows);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoReorder, 160.0f);

        if (showProcessID) {
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        }
        if (showParentProcessID) {
            ImGui::TableSetupColumn("PPID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        }
        if (showModuleID) {
            ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        }
        if (showMemoryUsage) {
            ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed);
        }
        if (showPriorityClass) {
            ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed);
        }
        if (showThreadCount) {
            ImGui::TableSetupColumn("Threads");
        }

        ImGui::TableHeadersRow();

        // Lock the data and read the entries
        std::scoped_lock dataLock(mDataCache.GetMutex());

        for (const auto& entry : mDataCache.GetEntries()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            /* [DEBUG]
             * Show width of each column in the first row. */
            if (showColWidthRow) {
                ImGui::Text("%.2f", ImGui::GetContentRegionAvail().x);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", ImGui::GetContentRegionAvail().x);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", ImGui::GetContentRegionAvail().x);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                showColWidthRow = false; // Only called once (first row)
            }

            {
                // Lock the entry
                std::scoped_lock entryLock(entry->Mutex());

                static char uniqueId[64];
                sprintf_s(uniqueId, "##%lu", entry->GetProcessId());

                if (ImGui::Selectable(entry->GetName().c_str(), entry->IsSelected(),
                        ImGuiSelectableFlags_SpanAllColumns, ImGui::GetColumnWidth(-1), uniqueId)) {
                    mDataCache.SelectEntry(entry);
                }

                if (showProcessID) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%lu", entry->GetProcessId());
                }
                if (showParentProcessID) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%lu", entry->GetParentProcessId());
                }
                if (showModuleID) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%lu", entry->GetModuleId());
                }
                if (showMemoryUsage) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%lu", entry->GetMemoryUsage());
                }
                if (showPriorityClass) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%lu", entry->GetPriorityClass());
                }
                if (showThreadCount) {
                    ImGui::TableNextColumn();
                    ImGui::Text("%lu", entry->GetThreadCount());
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(4);
}

} // RESANA