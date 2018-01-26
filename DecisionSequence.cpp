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
    Objectives["IncreaseArmyTraining"] = ConditionalResponse(
        std::bind(&DecisionSequence::readyForArmyFacility, this),
        std::bind(&DecisionSequence::constructArmyFacility, this));
    priorityList.push_back("EnoughSupply");
    // priorityList.push_back("FullSaturation");
    priorityList.push_back("ArmyWarriors");
    priorityList.push_back("IncreaseArmyTraining");
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

bool DecisionSequence::readyForArmyFacility() const {
    const static BWAPI::UnitType armyType = race->getArmyUnitType();
    const static int armyUnitPrice = armyType.mineralPrice();
    const static BWAPI::UnitType productionType = armyType.whatBuilds().first;
    const static int facilityPrice = productionType.mineralPrice();
    const int productionCount = (
        BWAPI::Broodwar->self()->allUnitCount(productionType));
    const int armyPriceBuffer = armyUnitPrice * productionCount;
    const int mineralPrice = facilityPrice + armyPriceBuffer - 24;
    return BWAPI::Broodwar->self()->minerals() > mineralPrice;
}

void DecisionSequence::constructArmyFacility() const {
    race->construct(race->getArmyUnitType().whatBuilds().first);
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
