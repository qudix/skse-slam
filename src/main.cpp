#include "Version.h"
#include "Papyrus.h"

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= "SLAM.log";
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info("SLAM v{}", Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "SLAM";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible");
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical("Unsupported runtime version {}", ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	auto papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus->Register(slaModules::RegisterFuncs)) {
		logger::critical("Failed to register papyrus callback");
		return false;
	}

	slaModules::RegisterSerialization();

	logger::info("SLAM loaded");

	return true;
}
