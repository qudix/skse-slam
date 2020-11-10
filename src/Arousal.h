#pragma once

#include "Serialization.h"
#include "Utils.h"

namespace slaModules
{
    uint32_t staticEffectCount = 0;
    std::unordered_map<std::string, uint32_t> staticEffectIds;

    float GetEffectLimitOffset(uint32_t effectIdx)
    {
        if (effectIdx == 1)
            return 0.5;
        return 0.0;
    }

    struct ArousalEffectGroup
    {
        ArousalEffectGroup() : value(0.f) {}
        std::vector<uint32_t> staticEffectIds;
        float value;
    };

    using ArousalEffectGroupPtr = std::shared_ptr<ArousalEffectGroup>;

    struct ArousalEffectData
    {
        ArousalEffectData() : value(0.f), function(0), param(0.f), limit(0.f), intAux(0) {}

        float value;
        int32_t function;
        float param;
        float limit;
        union {
            float floatAux;
            int32_t intAux;
        };

        void Set(int32_t a_functionId, float a_param, float a_limit, int32_t a_auxilliary)
        {
            function = a_functionId;
            param = a_param;
            limit = a_limit + GetEffectLimitOffset(a_functionId);
            intAux = a_auxilliary;
        }
    };

    class ArousalData
    {
    public:
        ArousalData() : staticEffects(staticEffectCount), staticEffectGroups(staticEffectCount), arousal(0.f), lastUpdate(0.f), lockedArousal(std::numeric_limits<float>::quiet_NaN()) {}
        ArousalData(SKSE::SerializationInterface* intfc, uint32_t& length) : ArousalData()
        {
            arousal = ReadDataHelper<float>(intfc, length);
            lastUpdate = ReadDataHelper<float>(intfc, length);
            uint32_t count = ReadDataHelper<uint32_t>(intfc, length);
            for (uint32_t j = 0; j < count; ++j)
                staticEffects[j] = ReadDataHelper<ArousalEffectData>(intfc, length);

            count = ReadDataHelper<uint8_t>(intfc, length);
            for (uint32_t j = 0; j < count; ++j)
            {
                auto grp = std::make_shared<ArousalEffectGroup>();
                uint32_t grpEntiryCount = ReadDataHelper<uint32_t>(intfc, length);
                for (uint32_t k = 0; k < grpEntiryCount; ++k)
                {
                    uint32_t effIdx = ReadDataHelper<uint32_t>(intfc, length);
                    grp->staticEffectIds.emplace_back(effIdx);
                    staticEffectGroups[effIdx] = grp;
                }
                grp->value = ReadDataHelper<float>(intfc, length);
                if (std::abs(grp->value) > 10000.f)
                {
                    logger::info("Possibly corrupted data reseting to zero");
                    grp->value = 0.f;
                }
                groupsToUpdate.emplace_back(std::move(grp));
            }
            count = ReadDataHelper<uint32_t>(intfc, length);
            for (uint32_t j = 0; j < count; ++j)
                staticEffectsToUpdate.insert(ReadDataHelper<uint32_t>(intfc, length));
            count = ReadDataHelper<uint32_t>(intfc, length);
            for (uint32_t j = 0; j < count; ++j) {
                std::string name = ReadString(intfc, length);
                dynamicEffects[name] = ReadDataHelper<ArousalEffectData>(intfc, length);
            }
            count = ReadDataHelper<uint32_t>(intfc, length);
            for (uint32_t j = 0; j < count; ++j)
                dynamicEffectsToUpdate.insert(ReadString(intfc, length));

            float recalculated = 0.f;
            for (uint32_t i = 0; i < staticEffects.size(); ++i)
            {
                if (!staticEffectGroups[i])
                    recalculated += staticEffects[i].value;
            }
            for (auto const& eff : dynamicEffects)
                recalculated += eff.second.value;
            for (auto const& grp : groupsToUpdate)
                recalculated += grp->value;
            if (std::abs(recalculated - arousal) > 0.5)
                logger::info("Arousal data mismatch: Expected: {} Got: {}", recalculated, arousal);
            arousal = recalculated;
        }
        ArousalData& operator=(ArousalData&& other) = default;

        void Serialize(SKSE::SerializationInterface* intfc) const
        {
            WriteData(intfc, &arousal);
            WriteData(intfc, &lastUpdate);
            WriteContainerData(intfc, staticEffects);
            uint8_t groupCount = static_cast<uint8_t>(groupsToUpdate.size());
            WriteData(intfc, &groupCount);
            for (auto& group : groupsToUpdate)
            {
                WriteContainerData(intfc, group->staticEffectIds);
                WriteData(intfc, &group->value);
            }
            WriteContainerData(intfc, staticEffectsToUpdate);
            uint32_t size = static_cast<uint32_t>(dynamicEffects.size());
            intfc->WriteRecordData(&size, sizeof(size));
            for (auto const& kvp : dynamicEffects)
            {
                WriteString(intfc, kvp.first);
                intfc->WriteRecordData(&kvp.second, sizeof(kvp.second));
            }
            size = static_cast<uint32_t>(dynamicEffectsToUpdate.size());
            intfc->WriteRecordData(&size, sizeof(size));
            for (auto const& toUpdate : dynamicEffectsToUpdate)
                WriteString(intfc, toUpdate);
        }

        void OnRegisterStaticEffect()
        {
            staticEffects.emplace_back();
            staticEffectGroups.emplace_back(nullptr);
        }

        void OnUnregisterStaticEffect(uint32_t id)
        {
            // staticEffects.erase(staticEffects.begin() + id);
            // staticEffectGroups.erase(staticEffectGroups.begin() + id);
            try
            {
                SetStaticArousalValue(id, 0.f);
                SetStaticArousalEffect(id, 0, 0.f, 0.f, 0);
                if (staticEffectGroups[id])
                    RemoveEffectGroup(id);
            }
            catch (std::exception ex)
            {
                logger::info("Unexpected exception in OnUnregisterStaticEffect: {}", ex.what());
            }
        }

        ArousalEffectGroupPtr GetEffectGroup(int32_t effectIdx)
        {
            if (effectIdx < 0 || effectIdx >= staticEffects.size())
                throw std::invalid_argument("Invalid static effect index");
            return staticEffectGroups[effectIdx];
        }

        ArousalEffectData& GetStaticArousalEffect(int32_t effectIdx)
        {
            if (effectIdx < 0 || effectIdx >= staticEffects.size())
                throw std::invalid_argument("Invalid static effect index");
            return staticEffects[effectIdx];
        }

        int32_t GetDynamicEffectCount() const
        {
            return static_cast<int32_t>(dynamicEffects.size());
        }

        RE::BSFixedString GetDynamicEffect(int32_t number) const
        {
            if (number >= dynamicEffects.size())
                return "";
            auto itr = dynamicEffects.begin();
            std::advance(itr, number);
            return itr->first.c_str();
        }

        float GetDynamicEffectValue(int32_t number) const
        {
            if (number >= dynamicEffects.size())
                return std::numeric_limits<float>::lowest();
            auto itr = dynamicEffects.begin();
            std::advance(itr, number);
            return itr->second.value;
        }

        float GetDynamicEffectValueByName(RE::BSFixedString effectId) const
        {
            std::string effectName(effectId.data());

            auto itr = dynamicEffects.find(effectName);
            if (itr != dynamicEffects.end())
                return itr->second.value;
            else
                return 0.f;
        }

        bool IsStaticEffectActive(int32_t effectIdx)
        {
            return staticEffectsToUpdate.find(effectIdx) != staticEffectsToUpdate.end();
        }

        void RemoveDynamicEffectIfNeeded(std::string effectName, ArousalEffectData& effect)
        {
            if (effect.function == 0 && effect.value == 0.f)
                dynamicEffects.erase(effectName);
        }

        void SetDynamicArousalEffect(RE::BSFixedString effectId, float initialValue, int32_t functionId, float param, float limit)
        {
            std::string effectName(effectId.data());
            ArousalEffectData& effect = dynamicEffects[effectName];

            if (functionId && !effect.function)
                dynamicEffectsToUpdate.insert(effectName);
            else if (!functionId && effect.function)
                dynamicEffectsToUpdate.erase(effectName);

            effect.Set(functionId, param, limit, 0);
            if (initialValue)
            {
                arousal += initialValue - effect.value;
                effect.value = initialValue;
            }
            RemoveDynamicEffectIfNeeded(std::move(effectName), effect);
        }

        void ModDynamicArousalEffect(RE::BSFixedString effectId, float modifier, float limit)
        {
            std::string effectName(effectId.data());
            ArousalEffectData& effect = dynamicEffects[effectName];

            float value = effect.value + modifier;
            float actualDiff = modifier;
            if ((modifier < 0 && limit > value) || (modifier > 0 && limit < value))
            {
                value = limit;
                actualDiff = limit - value;
            }
            arousal += actualDiff;
            effect.value = value;
            RemoveDynamicEffectIfNeeded(std::move(effectName), effect);
        }

        void SetStaticArousalEffect(int32_t effectIdx, int32_t functionId, float param, float limit, int32_t auxilliary)
        {
            ArousalEffectData& effect = GetStaticArousalEffect(effectIdx);

            if (functionId && !effect.function)
                staticEffectsToUpdate.insert(effectIdx);
            else if (!functionId && effect.function)
                staticEffectsToUpdate.erase(effectIdx);

            effect.Set(functionId, param, limit, auxilliary);
        }

        void SetStaticArousalValue(int32_t effectIdx, float value)
        {
            ArousalEffectData& effect = GetStaticArousalEffect(effectIdx);

            float diff = value - effect.value;
            effect.value = value;
            if (!staticEffectGroups[effectIdx])
                arousal += diff;
        }

        float ModStaticArousalValue(int32_t effectIdx, float diff, float limit)
        {
            ArousalEffectData& effect = GetStaticArousalEffect(effectIdx);

            float value = effect.value + diff;
            float actualDiff = diff;
            if ((diff < 0 && limit > value) || (diff > 0 && limit < value))
            {
                value = limit;
                actualDiff = limit - value;
            }
            effect.value = value;
            if (!staticEffectGroups[effectIdx])
                arousal += actualDiff;
            return actualDiff;
        }

        bool CalculateArousalEffect(ArousalEffectData& effect, float timeDiff, RE::Actor* who)
        {
            enum class LimitCheck
            {
                None,
                UpperBound,
                LowerBound
            };
            bool isDone = true;
            LimitCheck checkLimit = LimitCheck::None;
            float value;
            switch (effect.function)
            {
            case 1:
                value = effect.value * std::pow(0.5f, timeDiff / effect.param);
                checkLimit = effect.param * effect.value < 0.f ? LimitCheck::UpperBound : LimitCheck::LowerBound;
                break;
            case 2:
                value = effect.value + timeDiff * effect.param;
                checkLimit = effect.param >= 0.f ? LimitCheck::UpperBound : LimitCheck::LowerBound;
                break;
            case 3:
                value = (fastsin(float(who->formID % 7919) * 0.01f + lastUpdate * effect.param) + 1.f) * effect.limit;
                break;
            case 4:
                value = lastUpdate < effect.param ? 0.f : effect.limit;
                break;
            default:
                return true;
            }

            switch (checkLimit)
            {
            case LimitCheck::UpperBound:
                if (effect.limit < value)
                    value = effect.limit + GetEffectLimitOffset(effect.function);
                else
                    isDone = false;
                break;
            case LimitCheck::LowerBound:
                if (effect.limit > value)
                    value = effect.limit - GetEffectLimitOffset(effect.function);
                else
                    isDone = false;
                break;
            default: break;
            }

            effect.value = value;
            return isDone;
        }

        bool GroupEffects(RE::Actor* who, int32_t idx, int32_t idx2)
        {
            ArousalEffectData& first = GetStaticArousalEffect(idx);
            ArousalEffectData& second = GetStaticArousalEffect(idx2);
            ArousalEffectGroupPtr targetGrp = staticEffectGroups[idx];
            ArousalEffectGroupPtr otherGrp = staticEffectGroups[idx2];
            if (!targetGrp)
                targetGrp = otherGrp;
            else if (otherGrp)
                return targetGrp == otherGrp;
            if (!targetGrp)
            {
                targetGrp = std::make_shared<ArousalEffectGroup>();
                groupsToUpdate.push_back(targetGrp);
            }

            if (!staticEffectGroups[idx])
            {
                staticEffectGroups[idx] = targetGrp;
                targetGrp->staticEffectIds.push_back(idx);
                staticEffectsToUpdate.erase(idx);
                arousal -= first.value;
            }
            if (!staticEffectGroups[idx2])
            {
                staticEffectGroups[idx2] = targetGrp;
                targetGrp->staticEffectIds.push_back(idx2);
                staticEffectsToUpdate.erase(idx2);
                arousal -= second.value;
            }

            UpdateGroup(*targetGrp, 0.f, who);
            return true;
        }

        void RemoveEffectGroup(int32_t idx)
        {
            ArousalEffectGroupPtr group = staticEffectGroups[idx];
            auto itr = std::find(groupsToUpdate.begin(), groupsToUpdate.end(), group);
            if (itr == groupsToUpdate.end())
                throw std::logic_error("Error while removing group: group does not exist!");
            groupsToUpdate.erase(itr);
            arousal -= group->value;
            for (uint32_t id : group->staticEffectIds)
            {
                ArousalEffectData& eff = GetStaticArousalEffect(id);
                staticEffectGroups[id] = nullptr;
                arousal += eff.value;
                if (eff.function)
                    staticEffectsToUpdate.insert(id);
            }
        }

        void UpdateSingleActorArousal(RE::Actor* who, float GameDaysPassed)
        {
            if (!lastUpdate)
            {
                std::random_device rd;
                std::default_random_engine gen{ rd() };
                std::normal_distribution<> d{ 0.5, 2.0 };
                float randomTimeDiff = std::abs(float(d(gen)));
                lastUpdate = GameDaysPassed - randomTimeDiff;
            }

            float diff = GameDaysPassed - lastUpdate;
            lastUpdate = GameDaysPassed;

            for (auto group : groupsToUpdate)
                UpdateGroup(*group, diff, who);

            for (auto itr = staticEffectsToUpdate.begin(); itr != staticEffectsToUpdate.end();)
            {
                if (staticEffectGroups[*itr])
                    ++itr;
                else
                {
                    ArousalEffectData& effect = staticEffects[*itr];
                    if (UpdateArousalEffect(effect, diff, who))
                    {
                        effect.function = 0;
                        itr = staticEffectsToUpdate.erase(itr);
                    }
                    else
                        ++itr;
                }
            }

            for (auto itr = dynamicEffectsToUpdate.begin(); itr != dynamicEffectsToUpdate.end();)
            {
                ArousalEffectData& effect = dynamicEffects[*itr];
                if (UpdateArousalEffect(effect, diff, who))
                {
                    effect.function = 0;
                    RemoveDynamicEffectIfNeeded(*itr, effect);
                    itr = dynamicEffectsToUpdate.erase(itr);
                }
                else
                    ++itr;
            }
        }

        float GetArousal() const { return arousal; }
        float GetLastUpdate() const { return lastUpdate; }

    private:
        void UpdateGroupFactor(ArousalEffectGroup& group, float oldFactor, float newFactor)
        {
            float value = group.value / oldFactor * newFactor;
            float diff = value - group.value;
            arousal += diff;
            group.value = value;
        }

        void UpdateGroup(ArousalEffectGroup& group, float timeDiff, RE::Actor* who)
        {
            float value = 1.f;
            for (uint32_t id : group.staticEffectIds)
            {
                ArousalEffectData& eff = GetStaticArousalEffect(id);
                CalculateArousalEffect(eff, timeDiff, who);
                value *= eff.value;
            }
            float diff = value - group.value;
            arousal += diff;
            group.value = value;
        }

        // Should only be called for non-grouped effects
        bool UpdateArousalEffect(ArousalEffectData& effect, float timeDiff, RE::Actor* who)
        {
            float oldValue = effect.value;
            bool isDone = CalculateArousalEffect(effect, timeDiff, who);
            float diff = effect.value - oldValue;
            arousal += diff;
            return isDone;
        }

        ArousalData& operator=(const ArousalData&) = delete;
        ArousalData(const ArousalData&) = delete;

        std::unordered_set<int32_t> staticEffectsToUpdate;
        std::vector<ArousalEffectData> staticEffects;
        std::vector<ArousalEffectGroupPtr> staticEffectGroups;
        std::unordered_set<std::string> dynamicEffectsToUpdate;
        std::unordered_map<std::string, ArousalEffectData> dynamicEffects;
        std::vector<ArousalEffectGroupPtr> groupsToUpdate;
        float arousal;
        float lastUpdate;
        float lockedArousal;
    };
}