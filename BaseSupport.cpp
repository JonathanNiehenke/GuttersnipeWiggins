#pragma once
#include "BaseSupport.h"

void BaseSupport::onStart(Race* race) {
    this->race = race;
    Objectives["EnoughSupply"] = ConditionalResponse(
        std::bind(&Race::needsSupply, race),
        std::bind(&Race::createSupply, race));
    Objectives["FullSaturation"] = ConditionalResponse(
        std::bind(&Race::canFillLackingMiners, race),
        std::bind(&Race::createWorkers, race));
    // Objectives["MinimalSaturation"] = ConditionalResponse(
        // std::bind(&Race::lackingMinimalMiners, race),
        // std::bind(&Race::createWorkers, race));
    // Objectives["EconomicExpansion"] = ConditionalResponse(
        // std::bind(&Race::lackingExpansion, race),
        // std::bind(&Race::constructExpansion, race));
    Objectives["ArmyWarriors"] = ConditionalResponse(
        std::bind(&Race::canTrainWarriors, race),
        std::bind(&Race::trainWarriors, race));
    Objectives["Teir1WarriorTech"] = ConditionalResponse(
        std::bind(&Race::readyForTeir1Tech, race),
        std::bind(&Race::createFacility, race));
    priorityList.push_back("EnoughSupply");
    priorityList.push_back("FullSaturation");
    priorityList.push_back("ArmyWarriors");
    priorityList.push_back("Teir1WarriorTech");
}

void BaseSupport::update() {
    for (const std::string& Key: priorityList) {
        const ConditionalResponse& logic = Objectives[Key];
        if (logic.conditional()) {
            logic.response();
            BWAPI::Broodwar->sendText(Key.c_str());
            break;
        }
    }
}

BaseSupport::ConditionalResponse::ConditionalResponse() {
    this->conditional = nullptr;
    this->response = nullptr;
}

BaseSupport::ConditionalResponse::ConditionalResponse(
    std::function<bool(void)> conditional, std::function<void(void)> response)
{
    this->conditional = conditional;
    this->response = response;
}
