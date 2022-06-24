#pragma once

#include "Panel.h"
#include "system/memory/MemoryPerformance.h"
#include "system/cpu/CPUPerformance.h"

//#include "helpers/Time.h"

namespace RESANA
{
	class PerformancePanel final : public Panel
	{
	public:
		PerformancePanel();
		~PerformancePanel() override;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(Timestep ts) override;
		void OnImGuiRender() override;
		void ShowPanel(bool* pOpen) override;

	private:
		void ShowPhysicalMemoryTable() const;
		void ShowVirtualMemoryTable() const;
		void ShowCPUTable();
	private:
		MemoryPerformance* mMemoryInfo = nullptr;
		CPUPerformance* mCPUInfo = nullptr;

		Timestep mTickRate{};
	};

} // RESANA
