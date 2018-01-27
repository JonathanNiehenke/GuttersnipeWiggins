#pragma once
#include "DecisionSequence.h"

void DecisionSequence::onStart(Race* race) {
    this->race = race;
    Objectives["EnoughSupply"] = ConditionalResponse(
        std::bind(&DecisionSequence::needsSupply, this),
        std::bind(&Race::createSupply, race));
    // Objectives["FullSaturation"] = ConditionalResponse(
        // std::bind(&Race::canFillLackingMiners, race),
        // std::bind(&Race::createWorker, race));
    // Objectives["MinimalSaturation"] = ConditionalResponse(
        // std::bind(&Race::lackingMinimalMiners, race),
        // std::bind(&Race::createWorkers, race));
    // Objectives["EconomicExpansion"] = ConditionalResponse(
        // std::bind(&Race::lackingExpansion, race),
        // std::bind(&Race::constructExpansion, race));
    Objectives["ArmyWarriors"] = ConditionalResponse(
        std::bind(&Race::readyToTrainArmyUnit, race),
        std::bind(&Race::trainWarriors, race));
    Objectives["TechToArmy"] = ConditionalResponse(
        [this, race](){ return this->canProgressFor(race->getArmyUnitType()); },
        [this, race](){ this->techTo(race->getArmyUnitType()); });
    priorityList.push_back("EnoughSupply");
    // priorityList.push_back("FullSaturation");
    priorityList.push_back("ArmyWarriors");
    priorityList.push_back("TechToArmy");
}

void DecisionSequence::update() {
    for (const std::string& Key: priorityList) {
        const ConditionalResponse& logic = Objectives[Key];
        if (logic.conditional()) {
            logic.response();
            BWAPI::Broodwar->sendText(Key.c_str());
            break;
        }
    }
}

bool DecisionSequence::needsSupply() const {
    const int currentlyUsed = BWAPI::Broodwar->self()->supplyUsed(),
              workerBuffer = race->potentialSupplyUsed(race->getWorkerType()),
              armyBuffer = race->potentialSupplyUsed(race->getArmyUnitType());
    const int expectedSupplyUsed = currentlyUsed + workerBuffer + armyBuffer;
    return race->expectedSupplyProvided() <= expectedSupplyUsed;
}

bool DecisionSequence::canProgressFor(const BWAPI::UnitType& unitType) const {
    try {
        return enoughResources(race->getNextRequiredBuilding(unitType)); }
    catch (std::runtime_error) {
        return false; }
}

bool DecisionSequence::enoughResources(const BWAPI::UnitType& unitType) const {
    return (BWAPI::Broodwar->self()->minerals() >
            unitType.mineralPrice() - 24);
}

void DecisionSequence::techTo(const BWAPI::UnitType& unitType) const {
    try {
        race->construct(race->getNextRequiredBuilding(unitType)); }
    catch (const std::runtime_error& e) {
        BWAPI::Broodwar << "techTo: " << e.what() << "!!!!!" << std::endl; }
}

DecisionSequence::ConditionalResponse::ConditionalResponse() {
    this->conditional = nullptr;
    this->response = nullptr;
}

DecisionSequence::ConditionalResponse::ConditionalResponse(
    std::function<bool(void)> conditional, std::function<void(void)> response)
{
    this->conditional = conditional;
    this->response = response;
}
