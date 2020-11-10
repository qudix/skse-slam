#pragma once

#include "Arousal.h"
#include "Serialization.h"

using VM = RE::BSScript::IVirtualMachine;

namespace slaModules
{
    uint32_t lastLookup;
    ArousalData* lastData = nullptr;
    std::unordered_map<uint32_t, ArousalData> arousalData;

    uint32_t GetStaticEffectCount(RE::StaticFunctionTag*)
    {
        return staticEffectCount;
    }

    std::string GetUnusedEffectId(int32_t id)
    {
        return "Unused" + std::to_string(id);
    }

    int32_t GetHighestUnusedEffectId()
    {
        int32_t result = -1;
        while (staticEffectIds.find(GetUnusedEffectId(result + 1)) != staticEffectIds.end())
            result += 1;
        return result;
    }

    uint32_t RegisterStaticEffect(RE::StaticFunctionTag*, RE::BSFixedString name)
    {
        auto itr = staticEffectIds.find(name.data());
        if (itr != staticEffectIds.end())
            return itr->second;

        int32_t unusedId = GetHighestUnusedEffectId();
        if (unusedId != -1)
        {
            itr = staticEffectIds.find(GetUnusedEffectId(unusedId));
            assert(itr != staticEffectIds.end());
            uint32_t effectId = itr->second;
            staticEffectIds.erase(itr);
            staticEffectIds[name.data()] = effectId;
            return effectId;
        }

        staticEffectIds[name.data()] = staticEffectCount;
        for (auto& data : arousalData)
            data.second.OnRegisterStaticEffect();
        const auto result = staticEffectCount;
        staticEffectCount++;
        return result;
    }

    bool UnregisterStaticEffect(RE::StaticFunctionTag*, RE::BSFixedString name)
    {
        auto itr = staticEffectIds.find(name.data());
        if (itr != staticEffectIds.end())
        {
            uint32_t id = itr->second;
            staticEffectIds.erase(itr);
            int32_t unusedId = GetHighestUnusedEffectId();
            staticEffectIds[GetUnusedEffectId(unusedId + 1)] = id;
            for (auto& data : arousalData)
                data.second.OnUnregisterStaticEffect(id);
            return true;
        }
        return false;
    }

    ArousalData& _GetOrCreateArousalData(uint32_t formId)
    {
        if (lastLookup == formId && lastData)
            return *lastData;
        auto& result = arousalData[formId];
        lastLookup = formId;
        lastData = &result;
        return result;
    }

    ArousalData& GetArousalData(RE::Actor* who)
    {
        if (!who)
            throw std::invalid_argument("Attempt to get arousal data for none actor");
        return _GetOrCreateArousalData(who->formID);
    }

    ArousalEffectData& GetStaticArousalEffect(RE::Actor* who, int32_t effectIdx)
    {
        ArousalData& data = GetArousalData(who);
        return data.GetStaticArousalEffect(effectIdx);
    }

    int32_t GetDynamicEffectCount(RE::StaticFunctionTag*, RE::Actor* who)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.GetDynamicEffectCount();
        }
        catch (std::exception) { return 0; }
    }

    RE::BSFixedString GetDynamicEffect(RE::StaticFunctionTag*, RE::Actor* who, int32_t number)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.GetDynamicEffect(number);
        }
        catch (std::exception) { return ""; }
    }

    float GetDynamicEffectValueByName(RE::StaticFunctionTag*, RE::Actor* who, RE::BSFixedString effectId)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.GetDynamicEffectValueByName(effectId);
        }
        catch (std::exception) { return 0.0; }
    }

    float GetDynamicEffectValue(RE::StaticFunctionTag*, RE::Actor* who, int32_t number)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.GetDynamicEffectValue(number);
        }
        catch (std::exception) { return std::numeric_limits<float>::lowest(); }
    }

    bool IsStaticEffectActive(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.IsStaticEffectActive(effectIdx);
        }
        catch (std::exception) { return false; }
    }

    float GetStaticEffectValue(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx)
    {
        try {
            ArousalData& data = GetArousalData(who);
            if (auto group = data.GetEffectGroup(effectIdx))
                return group->value;
            ArousalEffectData& effect = data.GetStaticArousalEffect(effectIdx);
            return effect.value;
        }
        catch (std::exception) { return 0.f; }
    }

    float GetStaticEffectParam(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx)
    {
        try {
            ArousalEffectData& effect = GetStaticArousalEffect(who, effectIdx);
            return effect.param;
        }
        catch (std::exception) { return 0.f; }
    }

    int32_t GetStaticEffectAux(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx)
    {
        try {
            ArousalEffectData& effect = GetStaticArousalEffect(who, effectIdx);
            return effect.intAux;
        }
        catch (std::exception) { return 0; }
    }

    void SetDynamicArousalEffect(RE::StaticFunctionTag*, RE::Actor* who, RE::BSFixedString effectId, float initialValue, int32_t functionId, float param, float limit)
    {
        try {
            ArousalData& data = GetArousalData(who);
            data.SetDynamicArousalEffect(effectId, initialValue, functionId, param, limit);
        }
        catch (std::exception) {}
    }

    void ModDynamicArousalEffect(RE::StaticFunctionTag*, RE::Actor* who, RE::BSFixedString effectId, float modifier, float limit)
    {
        try {
            ArousalData& data = GetArousalData(who);
            data.ModDynamicArousalEffect(effectId, modifier, limit);
        }
        catch (std::exception) {}
    }

    void SetStaticArousalEffect(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx, int32_t functionId, float param, float limit, int32_t auxilliary)
    {
        try {
            ArousalData& data = GetArousalData(who);
            data.SetStaticArousalEffect(effectIdx, functionId, param, limit, auxilliary);
        }
        catch (std::exception) {}
    }

    void SetStaticArousalValue(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx, float value)
    {
        try {
            ArousalData& data = GetArousalData(who);
            data.SetStaticArousalValue(effectIdx, value);
        }
        catch (std::exception) {}
    }

    float ModStaticArousalValue(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx, float diff, float limit)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.ModStaticArousalValue(effectIdx, diff, limit);
        }
        catch (std::exception) { return 0.f; }
    }

    void SetStaticAuxillaryFloat(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx, float value)
    {
        try {
            ArousalData& data = GetArousalData(who);
            ArousalEffectData& effect = data.GetStaticArousalEffect(effectIdx);

            effect.floatAux = value;
        }
        catch (std::exception) {}
    }

    void SetStaticAuxillaryInt(RE::StaticFunctionTag*, RE::Actor* who, int32_t effectIdx, int32_t value)
    {
        try {
            ArousalData& data = GetArousalData(who);
            ArousalEffectData& effect = data.GetStaticArousalEffect(effectIdx);

            effect.intAux = value;
        }
        catch (std::exception) {}
    }

    float GetArousal(RE::StaticFunctionTag*, RE::Actor* who)
    {
        try {
            return GetArousalData(who).GetArousal();
        }
        catch (std::exception) { return 0.f; }
    }

    bool GroupEffects(RE::StaticFunctionTag*, RE::Actor* who, int32_t idx, int32_t idx2)
    {
        try {
            ArousalData& data = GetArousalData(who);
            return data.GroupEffects(who, idx, idx2);
        }
        catch (std::exception) { return false; }
    }

    bool RemoveEffectGroup(RE::StaticFunctionTag*, RE::Actor* who, int32_t idx)
    {
        try {
            ArousalData& data = GetArousalData(who);
            data.RemoveEffectGroup(idx);
            return true;
        }
        catch (std::exception) { return false; }
    }

    int32_t CleanUpActors(RE::StaticFunctionTag*, float lastUpdateBefore)
    {
        int32_t removed = 0;
        for (auto itr = arousalData.begin(); itr != arousalData.end();)
        {
            if (itr->second.GetLastUpdate() < lastUpdateBefore)
            {
                itr = arousalData.erase(itr);
                ++removed;
            }
            else
                ++itr;
        }
        return removed;
    }

    void UpdateSingleActorArousal(RE::StaticFunctionTag*, RE::Actor* who, float GameDaysPassed)
    {
        try
        {
            ArousalData& data = GetArousalData(who);
            data.UpdateSingleActorArousal(who, GameDaysPassed);
        }
        catch (std::exception) {}
    }

    std::vector<RE::Actor*> GetActorList(RE::StaticFunctionTag*)
    {
        std::vector<RE::Actor*> result;
        for (auto& entry : arousalData)
            if (RE::Actor* actor = dynamic_cast<RE::Actor*>(RE::TESForm::LookupByID(entry.first)))
                result.push_back(actor);
        return result;
    }

    // Regular bool would be enough IF skyrim always uses the same thread for all papyrus scripts, but since I have no idea...
    std::array<std::atomic_flag, 3> locks;

    bool TryLock(RE::StaticFunctionTag*, int32_t lock)
    {
        if (lock < 0 || lock >= locks.size())
            return false;
        if (locks[lock].test_and_set())
            return false;
        return true;
    }

    void Unlock(RE::StaticFunctionTag*, int32_t lock)
    {
        if (lock < 0 || lock >= locks.size())
            return;

        locks[lock].clear();
    }

    std::vector<RE::Actor*> DuplicateActorArray(RE::StaticFunctionTag*, std::vector<RE::Actor*> arr, int32_t count)
    {
        std::vector<RE::Actor*> result;
        result.reserve(count);
        for (auto& actor : arr) {
            result.push_back(actor);
        }
        return result;
    }

    const uint32_t kSerializationDataVersion = 1;

    void Serialization_Revert(SKSE::SerializationInterface*)
    {
        logger::info("revert");

        staticEffectCount = 0;
        staticEffectIds.clear();

        lastLookup = 0;
        lastData = nullptr;
        arousalData.clear();

        for (auto& lock : locks)
            lock.clear();
    }

    void Serialization_Load(SKSE::SerializationInterface* intfc)
    {
        logger::info("load");

        uint32_t type;
        uint32_t version;
        uint32_t length;
        bool error = false;

        while (!error && intfc->GetNextRecordInfo(type, version, length))
        {
            switch (type)
            {
            case 'DATA':
            {
                if (version == kSerializationDataVersion)
                {
                    logger::info("Version correct");
                    try
                    {
                        staticEffectCount = ReadDataHelper<uint32_t>(intfc, length);
                        logger::info("Loading {} effects... ", staticEffectCount);

                        for (uint32_t i = 0; i < staticEffectCount; ++i)
                        {
                            std::string effect = ReadString(intfc, length);
                            uint32_t id = ReadDataHelper<uint32_t>(intfc, length);
                            staticEffectIds[effect] = id;
                            // logger::info("Added effect '{}' with id {}", effect.c_str(), id);
                        }

                        uint32_t entryCount = ReadDataHelper<uint32_t>(intfc, length);
                        logger::info("Loading {} data sets... ", entryCount);

                        for (uint32_t i = 0; i < entryCount; ++i)
                        {
                            uint32_t formId = ReadDataHelper<uint32_t>(intfc, length);
                            // logger::info("Loading data for actor {}...", formId);
                            ArousalData data(intfc, length);
                            uint32_t newFormId;
                            if (!intfc->ResolveFormID(formId, newFormId))
                                continue;
                            arousalData[newFormId] = std::move(data);
                        }
                    }
                    catch (std::exception)
                    {
                        error = true;
                    }
                }
                else
                    error = true;
            }
            break;

            default:
                logger::info("unhandled type {}", type);
                error = true;
                break;
            }
        }

        if (error)
            logger::info("Encountered error while loading data");
    }

    void Serialization_Save(SKSE::SerializationInterface* intfc)
    {
        logger::info("save");

        if (intfc->OpenRecord('DATA', kSerializationDataVersion))
        {
            intfc->WriteRecordData(&staticEffectCount, sizeof(staticEffectCount));
            for (auto const& kvp : staticEffectIds)
            {
                WriteString(intfc, kvp.first);
                int32_t id = kvp.second;
                intfc->WriteRecordData(&id, sizeof(id));
            }
            uint32_t entryCount = static_cast<uint32_t>(arousalData.size());
            intfc->WriteRecordData(&entryCount, sizeof(entryCount));
            for (auto const& entry : arousalData)
            {
                ArousalData const& data = entry.second;
                uint32_t formId = entry.first;
                intfc->WriteRecordData(&formId, sizeof(formId));
                data.Serialize(intfc);
            }
        }
    }

    static constexpr char CLASS_NAME[] = "slaInternalModules";

    bool RegisterFuncs(VM* a_vm)
    {
        BuildSinCosTable();

        a_vm->RegisterFunction("GetStaticEffectCount", CLASS_NAME, GetStaticEffectCount);
        a_vm->RegisterFunction("RegisterStaticEffect", CLASS_NAME, RegisterStaticEffect);
        a_vm->RegisterFunction("UnregisterStaticEffect", CLASS_NAME, UnregisterStaticEffect);
        a_vm->RegisterFunction("IsStaticEffectActive", CLASS_NAME, IsStaticEffectActive);
        a_vm->RegisterFunction("GetDynamicEffectCount", CLASS_NAME, GetDynamicEffectCount);
        a_vm->RegisterFunction("GetDynamicEffect", CLASS_NAME, GetDynamicEffect);
        a_vm->RegisterFunction("GetDynamicEffectValueByName", CLASS_NAME, GetDynamicEffectValueByName);
        a_vm->RegisterFunction("GetDynamicEffectValue", CLASS_NAME, GetDynamicEffectValue);
        a_vm->RegisterFunction("GetStaticEffectValue", CLASS_NAME, GetStaticEffectValue);
        a_vm->RegisterFunction("GetStaticEffectParam", CLASS_NAME, GetStaticEffectParam);
        a_vm->RegisterFunction("GetStaticEffectAux", CLASS_NAME, GetStaticEffectAux);
        a_vm->RegisterFunction("SetStaticArousalEffect", CLASS_NAME, SetStaticArousalEffect);
        a_vm->RegisterFunction("SetDynamicArousalEffect", CLASS_NAME, SetDynamicArousalEffect);
        a_vm->RegisterFunction("ModDynamicArousalEffect", CLASS_NAME, ModDynamicArousalEffect);
        a_vm->RegisterFunction("SetStaticArousalValue", CLASS_NAME, SetStaticArousalValue);
        a_vm->RegisterFunction("SetStaticAuxillaryFloat", CLASS_NAME, SetStaticAuxillaryFloat);
        a_vm->RegisterFunction("SetStaticAuxillaryInt", CLASS_NAME, SetStaticAuxillaryInt);
        a_vm->RegisterFunction("ModStaticArousalValue", CLASS_NAME, ModStaticArousalValue);
        a_vm->RegisterFunction("GetArousal", CLASS_NAME, GetArousal);
        a_vm->RegisterFunction("UpdateSingleActorArousal", CLASS_NAME, UpdateSingleActorArousal);

        a_vm->RegisterFunction("GroupEffects", CLASS_NAME, GroupEffects);
        a_vm->RegisterFunction("RemoveEffectGroup", CLASS_NAME, RemoveEffectGroup);

        a_vm->RegisterFunction("CleanUpActors", CLASS_NAME, CleanUpActors);

        a_vm->RegisterFunction("TryLock", CLASS_NAME, TryLock, true);
        a_vm->RegisterFunction("Unlock", CLASS_NAME, Unlock, true);
        a_vm->RegisterFunction("DuplicateActorArray", CLASS_NAME, DuplicateActorArray, true);

        return true;
    }

    void RegisterSerialization()
    {
        auto serialization = SKSE::GetSerializationInterface();
        serialization->SetUniqueID('SLAM'); // this must be a UNIQUE ID, change this and email me the ID so I can let you know if someone else has already taken it
        serialization->SetRevertCallback(Serialization_Revert);
        serialization->SetSaveCallback(Serialization_Save);
        serialization->SetLoadCallback(Serialization_Load);
    }
}