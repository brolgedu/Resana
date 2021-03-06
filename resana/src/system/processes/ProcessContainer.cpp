#include "ProcessContainer.h"
#include "rspch.h"

#include "ProcessEntry.h"

namespace RESANA {

ProcessContainer::ProcessContainer() = default;

ProcessContainer::ProcessContainer(const ProcessContainer* other)
    : mEntries(other->mEntries)
    , mSelectedEntry(other->GetSelectedEntry())
{
}

ProcessContainer::~ProcessContainer()
{
    Clear(mMutex);
};

int ProcessContainer::GetNumEntries() const
{
    return (int)mEntries.size();
}

std::mutex& ProcessContainer::GetMutex()
{
    return mMutex;
}

std::vector<ProcessEntry*>& ProcessContainer::GetEntries()
{
    return mEntries;
}

ProcessEntry* ProcessContainer::GetSelectedEntry() const
{
    return mSelectedEntry;
}

ProcessEntry* ProcessContainer::FindEntry(const ProcessEntry* entry) const
{
    if (!entry) {
        return nullptr;
    }
    return FindEntry(entry->GetProcessId());
}

ProcessEntry* ProcessContainer::FindEntry(uint32_t procId) const
{
    for (auto* proc : mEntries) {
        if (proc->GetProcessId() == procId) {
            return proc;
        }
    }

    return nullptr;
}

void ProcessContainer::AddEntry(ProcessEntry* entry)
{
    mEntries.emplace_back(entry);
    SetDirty();
}

void ProcessContainer::SelectEntry(const uint32_t procId, bool preserve)
{
    auto* entry = FindEntry(procId);

    if (mSelectedEntry) {
        // Deselect the entry
        mSelectedEntry->Deselect();

        // Check if the entry is also the selected entry
        if (entry && (mSelectedEntry->GetProcessId() == entry->GetProcessId())) {
            if (preserve) {
                // Keep this selected
                mSelectedEntry->Select();
            } else {
                // There is no selected entry
                mSelectedEntry = nullptr;
            }
            return;
        }
    }

    if ((mSelectedEntry = entry)) {
        mSelectedEntry->Select();
    }
}

void ProcessContainer::SelectEntry(ProcessEntry* entry, bool preserve)
{
    if (entry) {
        SelectEntry(entry->GetProcessId(), preserve);
    } else {
        mSelectedEntry = nullptr;
    }
}

void ProcessContainer::EraseEntry(const ProcessEntry* entry)
{
    if (!entry) {
        return;
    }

    for (auto it = mEntries.begin(); it != mEntries.end(); ++it) {
        if (auto proc = mEntries.at(reinterpret_cast<std::vector<ProcessEntry*>::size_type>(&it));
            proc->GetProcessId() == entry->GetProcessId()) {
            delete proc;
            proc = nullptr;
            break;
        }
    }

    SetDirty();
}

void ProcessContainer::Copy(ProcessContainer* other)
{
    if (this == other) {
        return;
    }

    Clear(mMutex);
    mSelectedEntry = nullptr;

    std::scoped_lock lock1(mMutex);
    std::scoped_lock lock2(other->GetMutex());

    for (const auto* entry : other->GetEntries()) {
        mEntries.emplace_back(new ProcessEntry(entry));
    }

    SetDirty();
}
}
