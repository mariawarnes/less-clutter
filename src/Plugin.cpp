#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace LC {
namespace {
constexpr auto iniPath = L"Data\\SKSE\\Plugins\\LessClutter.ini";

enum class Mode { Off, Remove, Fix };
Mode mode = Mode::Remove;
Mode containerMode = Mode::Off;
bool debug = false;
std::unordered_set<std::string> names;  // lower-case display names
std::vector<std::string> formSpecs;     // "Plugin|0xID"
std::unordered_set<RE::FormID> forms;   // resolved at data load

std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
std::string Trim(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

std::vector<std::string> ReadSection(const wchar_t* section)
{
    std::vector<wchar_t> buffer(32768);
    GetPrivateProfileSectionW(section, buffer.data(), static_cast<DWORD>(buffer.size()), iniPath);
    std::vector<std::string> values;
    for (const wchar_t* p = buffer.data(); *p; p += wcslen(p) + 1) {
        const std::wstring line = p;
        const auto eq = line.find(L'=');
        if (eq == std::wstring::npos || line.front() == L';') continue;
        const auto size = WideCharToMultiByte(CP_UTF8, 0, line.c_str() + eq + 1, -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(size > 0 ? size - 1 : 0, '\0');
        WideCharToMultiByte(CP_UTF8, 0, line.c_str() + eq + 1, -1, utf8.data(), size, nullptr, nullptr);
        utf8 = Trim(utf8);
        if (!utf8.empty()) values.push_back(utf8);
    }
    return values;
}

void ReadConfig()
{
    wchar_t buffer[32]{};
    GetPrivateProfileStringW(L"General", L"Mode", L"Remove", buffer, 32, iniPath);
    mode = (_wcsicmp(buffer, L"Fix") == 0) ? Mode::Fix : Mode::Remove;
    debug = GetPrivateProfileIntW(L"General", L"Debug", 0, iniPath) != 0;
    wchar_t cbuffer[32]{};
    GetPrivateProfileStringW(L"General", L"EmptyContainers", L"Off", cbuffer, 32, iniPath);
    containerMode = (_wcsicmp(cbuffer, L"Remove") == 0) ? Mode::Remove : (_wcsicmp(cbuffer, L"Fix") == 0) ? Mode::Fix : Mode::Off;
    for (auto& n : ReadSection(L"Names")) names.insert(Lower(n));
    formSpecs = ReadSection(L"Forms");
    spdlog::info("Mode={}, EmptyContainers={}, {} names, {} form entries", mode == Mode::Fix ? "Fix" : "Remove",
        containerMode == Mode::Remove ? "Remove" : containerMode == Mode::Fix ? "Fix" : "Off", names.size(), formSpecs.size());
}

void ResolveForms()
{
    auto* handler = RE::TESDataHandler::GetSingleton();
    for (const auto& spec : formSpecs) {
        const auto bar = spec.find('|');
        if (bar == std::string::npos) { spdlog::warn("Bad form entry '{}': expected Plugin|FormID", spec); continue; }
        const auto plugin = Trim(spec.substr(0, bar));
        try {
            const auto local = static_cast<RE::FormID>(std::stoul(Trim(spec.substr(bar + 1)), nullptr, 16));
            if (auto* form = handler->LookupForm(local & 0x00FFFFFF, plugin)) forms.insert(form->GetFormID());
            else spdlog::warn("Form '{}' not found (plugin not loaded?)", spec);
        } catch (...) { spdlog::warn("Bad form entry '{}'", spec); }
    }
}

bool IsUseless(RE::TESBoundObject* base)
{
    if (!base) return false;
    if (forms.contains(base->GetFormID())) return true;
    if (base->Is(RE::FormType::Misc) || base->Is(RE::FormType::KeyMaster)) {
        const char* name = base->GetName();
        if (name && *name) return names.contains(Lower(name));
    }
    return false;
}

bool InPlayerHome(RE::TESObjectREFR* ref)
{
    if (ref->GetOwner() == RE::PlayerCharacter::GetSingleton()->GetActorBase()) return true;
    if (auto* cell = ref->GetParentCell(); cell && cell->GetOwner() == RE::PlayerCharacter::GetSingleton()->GetActorBase()) return true;
    for (auto* loc = ref->GetCurrentLocation(); loc; loc = loc->parentLoc)
        if (loc->HasKeywordString("LocTypePlayerHome")) return true;
    return false;
}

bool IsEmptyContainerOutsideHome(RE::TESObjectREFR* ref)
{
    auto* base = ref->GetBaseObject();
    if (!base || !base->Is(RE::FormType::Container)) return false;
    if (InPlayerHome(ref)) return false;
    // Empty, or holding nothing but useless items.
    for (auto& [item, entry] : ref->GetInventory())
        if (!IsUseless(item)) return false;
    return true;
}

// Returns the action to take for a reference, or Off if it should be left alone.
// Skips anything the player dropped, scripts spawned, or a quest depends on.
Mode Decide(RE::TESObjectREFR* ref)
{
    if (!ref || ref->IsDeleted() || ref->IsDisabled()) return Mode::Off;
    if (ref->GetFormID() >= 0xFF000000) return Mode::Off;
    if (ref->extraList.HasType(RE::ExtraDataType::kAliasInstanceArray)) return Mode::Off;
    if (IsUseless(ref->GetBaseObject())) return mode;
    if (containerMode != Mode::Off && IsEmptyContainerOutsideHome(ref)) return containerMode;
    return Mode::Off;
}

void Process(RE::FormID id)
{
    SKSE::GetTaskInterface()->AddTask([id] {
        auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
        const auto action = Decide(ref);
        if (debug && action != Mode::Off) spdlog::info("Action {} on ref {:08X} '{}'", action == Mode::Remove ? "Remove" : "Fix", id, ref->GetBaseObject()->GetName());
        if (action == Mode::Remove) {
            ref->Disable();
            ref->SetDelete(true);
        } else if (action == Mode::Fix) {
            ref->SetActivationBlocked(true);
            ref->SetMotionType(RE::hkpMotion::MotionType::kKeyframed, false);
        }
    });
}

struct LoadSink : RE::BSTEventSink<RE::TESObjectLoadedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* e, RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override
    {
        if (e && e->loaded) Process(e->formID);
        return RE::BSEventNotifyControl::kContinue;
    }
} sink;

// The loaded event does not fire for every area the player walks into (e.g.
// dungeon interiors), so also act when references attach and cells finish loading.
struct AttachSink : RE::BSTEventSink<RE::TESCellAttachDetachEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellAttachDetachEvent* e, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override
    {
        if (e && e->attached && e->reference) Process(e->reference->GetFormID());
        return RE::BSEventNotifyControl::kContinue;
    }
} attachSink;

struct CellSink : RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* e, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override
    {
        if (!e || !e->cell) return RE::BSEventNotifyControl::kContinue;
        const auto cellId = e->cell->GetFormID();
        SKSE::GetTaskInterface()->AddTask([cellId] {
            auto* cell = RE::TESForm::LookupByID<RE::TESObjectCELL>(cellId);
            if (!cell) return;
            cell->ForEachReference([](RE::TESObjectREFR* ref) {
                if (ref && Decide(ref) != Mode::Off) Process(ref->GetFormID());
                return RE::BSContainer::ForEachResult::kContinue;
            });
        });
        return RE::BSEventNotifyControl::kContinue;
    }
} cellSink;

struct CrosshairSink : RE::BSTEventSink<SKSE::CrosshairRefEvent> {
    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* e, RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override
    {
        if (debug && e && e->crosshairRef) {
            auto* ref = e->crosshairRef.get();
            auto* base = ref->GetBaseObject();
            if (base) {
                const auto dec = Decide(ref);
                spdlog::info("Crosshair: ref {:08X} base {:08X} type {} name '{}' decision {} disabled={} deleted={} alias={}",
                    ref->GetFormID(), base->GetFormID(), static_cast<int>(base->GetFormType()), base->GetName(),
                    dec == Mode::Off ? "none" : dec == Mode::Remove ? "Remove" : "Fix", ref->IsDisabled(), ref->IsDeleted(),
                    ref->extraList.HasType(RE::ExtraDataType::kAliasInstanceArray));
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
} crosshairSink;

// Cover references that were already loaded before the sink saw them.
void Sweep()
{
    SKSE::GetTaskInterface()->AddTask([] {
        auto* tes = RE::TES::GetSingleton();
        if (!tes) return;
        tes->ForEachReference([](RE::TESObjectREFR* ref) {
            if (ref && Decide(ref) != Mode::Off) Process(ref->GetFormID());
            return RE::BSContainer::ForEachResult::kContinue;
        });
    });
}

void OnMessage(SKSE::MessagingInterface::Message* m)
{
    switch (m->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        ResolveForms();
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(&sink);
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(&attachSink);
        RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(&cellSink);
        if (debug) SKSE::GetCrosshairRefEventSource()->AddEventSink(&crosshairSink);
        spdlog::info("Ready: {} form ids resolved", forms.size());
        break;
    case SKSE::MessagingInterface::kNewGame:
    case SKSE::MessagingInterface::kPostLoadGame:
        Sweep();
        break;
    }
}
}
}

SKSE_PLUGIN_VERSION = [] {
    SKSE::PluginVersionData data;
    data.PluginVersion({1, 0, 0, 0});
    data.PluginName("Less Clutter");
    data.AuthorName("mlw05");
    data.UsesAddressLibrary();
    data.UsesUpdatedStructs();
    data.CompatibleVersions({REL::Version{1, 7, 104, 0}});
    return data;
}();

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    auto path = SKSE::log::log_directory();
    if (!path) return false;
    *path /= "LessClutter.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("LessClutter", sink));
    spdlog::flush_on(spdlog::level::info);
    LC::ReadConfig();
    return SKSE::GetMessagingInterface()->RegisterListener(LC::OnMessage);
}
