#include "rspch.h"
#include "ResourcePanel.h"

#include "perfdata/PerfManager.h"

#include <imgui.h>

namespace RESANA
{

	ResourcePanel::ResourcePanel()
	{
		mMemoryInfo = PerfManager::Get()->GetMemory();
		mCPUInfo = CPUPerformance::Get();
	};

	ResourcePanel::~ResourcePanel()
	{
		RS_CORE_TRACE("ResourcePanel::~ResourcePanel()");

		mMemoryInfo->Stop();
		CPUPerformance::Stop();

	}

	void ResourcePanel::ShowPanel(bool* pOpen)
	{
		if (*pOpen)
		{
			if (ImGui::Begin("System Resources", pOpen)) {
				mMemoryInfo->Start();
				CPUPerformance::Run();

				ImGui::TextUnformatted("Memory");
				ShowPhysicalMemoryTable();
				ShowVirtualMemoryTable();
				ShowCPUTable();
			}

			ImGui::End();
		}
		else
		{
			mMemoryInfo->Stop();
			CPUPerformance::Stop();
		}
	}

	void ResourcePanel::ShowPhysicalMemoryTable() const
	{
		ImGui::BeginTable("##Physical Memory", 2, ImGuiTableFlags_Borders);
		ImGui::TableSetupColumn("Physical");
		ImGui::TableSetupColumn("##values");
		ImGui::TableHeadersRow();
		ImGui::TableNextColumn();

		// TODO: Resize columns based on size
		ImGui::Text("Total");
		ImGui::Text("In use");
		ImGui::Text("Available");
		ImGui::Text("Used by process");
		ImGui::TableNextColumn();

		const auto totalMem = mMemoryInfo->GetTotalPhys() / BYTES_PER_MB;
		const auto usedMem = mMemoryInfo->GetUsedPhys() / BYTES_PER_MB;
		const float usedPercent = (float)usedMem / (float)totalMem * 100.0f;
		const auto availMem = mMemoryInfo->GetAvailPhys() / BYTES_PER_MB;
		const auto procMem = mMemoryInfo->GetCurrProcUsagePhys() / BYTES_PER_MB;

		ImGui::Text("%llu.%llu GB", totalMem / 1000, totalMem % 10);
		ImGui::Text("%llu.%llu GB (%.1f%%)", usedMem / 1000, usedMem % 10, usedPercent);
		ImGui::Text("%llu.%llu GB", availMem / 1000, availMem % 10);
		ImGui::Text("%lu MB", procMem);
		ImGui::EndTable();
	}

	void ResourcePanel::ShowVirtualMemoryTable() const
	{
		ImGui::BeginTable("##Virtual Memory", 2, ImGuiTableFlags_Borders);
		ImGui::TableSetupColumn("Virtual");
		ImGui::TableSetupColumn("##values");
		ImGui::TableHeadersRow();
		ImGui::TableNextColumn();

		ImGui::Text("Total");
		ImGui::Text("In use");
		ImGui::Text("Available");
		ImGui::Text("Used by process");
		ImGui::TableNextColumn();

		const auto totalMem = mMemoryInfo->GetTotalVirtual() / BYTES_PER_MB;
		const auto usedMem = mMemoryInfo->GetUsedVirtual() / BYTES_PER_MB;
		const float usedPercent = (float)usedMem / (float)totalMem * 100.0f;
		const auto availMem = mMemoryInfo->GetAvailVirtual() / BYTES_PER_MB;
		const auto procMem = mMemoryInfo->GetCurrProcUsageVirtual() / BYTES_PER_MB;

		ImGui::Text("%llu.%llu GB", totalMem / 1000, totalMem % 10);
		ImGui::Text("%llu.%llu GB (%.1f%%)", usedMem / 1000, usedMem % 10, usedPercent);
		ImGui::Text("%llu.%llu GB", availMem / 1000, availMem % 10);
		ImGui::Text("%lu MB", procMem);
		ImGui::EndTable();
	}

	void ResourcePanel::ShowCPUTable()
	{
		ImGui::BeginTable("##CPU", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable);
		ImGui::TableSetupColumn("CPU");
		ImGui::TableSetupColumn("##values");
		ImGui::TableHeadersRow();
		ImGui::TableNextColumn();

		mCPUInfo = CPUPerformance::Get();
		if (const auto &data = mCPUInfo->GetData())
		{
			std::scoped_lock slock(data->GetMutex());

			for (const auto& p : data->Processors) {
				ImGui::Text("cpu %s", p->szName);
			}

			ImGui::Text("Total");
			ImGui::Text("Used by process");
			ImGui::TableNextColumn();

			// Display values for all logical processors
			for (const auto& p : data->Processors) {
				ImGui::Text("%.1f%%", p->FmtValue.doubleValue);
			}

			// Display current CPU load and load in use by process
			const double currLoad = mCPUInfo->GetAverageLoad();
			const double procLoad = mCPUInfo->GetCurrentProcessLoad();
			ImGui::Text("%.1f%%", currLoad);
			ImGui::Text("%.1f%%", procLoad);

			mCPUInfo->ReleaseData();
		}

		ImGui::EndTable();
	}

} // RESANA