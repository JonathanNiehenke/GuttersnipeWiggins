#pragma once
#include <map>
#include <vector>
#include "Race.h"

class BaseSupport {
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
    public:
        void onStart(Race* race);
        void update();
};
