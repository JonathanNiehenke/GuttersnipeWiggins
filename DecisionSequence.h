#pragma once
#include <map>
#include <vector>
#include "Production.h"

class DecisionSequence {
    private:
        struct ConditionalResponse {
            std::function<bool(void)> conditional;
            std::function<void(void)> response;
            ConditionalResponse();
            ConditionalResponse(
                std::function<bool(void)>, std::function<void(void)>);
        };
        std::map<std::string, ConditionalResponse> Objectives;
        std::vector<std::string> priorityList;
        Production* production = nullptr;
        bool needsSupply() const;
        void createSupply() const;
        bool canProgressFor(const BWAPI::UnitType& unitType) const; 
        bool canBegin(const BWAPI::UnitType& unitType) const;
        bool isIncomplete(const BWAPI::UnitType& unitType) const;
        bool enoughResources(const BWAPI::UnitType& unitType) const;
        void techTo(const BWAPI::UnitType& unitType) const;
        bool canExtend(const BWAPI::UnitType& unitType) const;
        void increaseFacilities(const BWAPI::UnitType& unitType) const;
    public:
        void onStart(Production* production);
        void update();
};
