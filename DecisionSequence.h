#pragma once
#include <map>
#include <vector>
#include "Race.h"

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
        Race* race = nullptr;
        bool needsSupply() const;
        bool readyForArmyFacility() const;
        void constructArmyFacility() const;
    public:
        void onStart(Race* race);
        void update();
};
