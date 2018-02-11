#pragma once
#include "DecisionSequence.h"

void DecisionSequence::onStart(Production* production) {
    this->production = production;
    Objectives["EnoughSupply"] = ConditionalResponse(
        std::bind(&DecisionSequence::needsSupply, this),
        std::bind(&DecisionSequence::createSupply, this));
    Objectives["FullSaturation"] = ConditionalResponse(
        std::bind(&Production::canFillLackingMiners, production),
        std::bind(&Production::createWorker, production));
    // Objectives["MinimalSaturation"] = ConditionalResponse(
        // std::bind(&Production::lackingMinimalMiners, production),
        // std::bind(&Production::createWorkers, production));
    // Objectives["EconomicExpansion"] = ConditionalResponse(
        // std::bind(&Production::lackingExpansion, production),
        // std::bind(&Production::constructExpansion, production));
    Objectives["ArmyWarriors"] = ConditionalResponse(
        std::bind(&Production::readyToTrainArmyUnit, production),
        std::bind(&Production::trainWarriors, production));
    Objectives["TechToArmy"] = ConditionalResponse(
        [this, production](){
            return this->canProgressFor(production->getArmyUnitType()); },
        [this, production](){ this->techTo(production->getArmyUnitType()); });
    Objectives["IncreaseArmyFacilities"] = ConditionalResponse(
        [this, production](){
            return this->canExtend(production->getArmyUnitType()); },
        [this, production](){
            this->increaseFacilities(production->getArmyUnitType()); });
    priorityList.push_back("EnoughSupply");
    priorityList.push_back("FullSaturation");
    priorityList.push_back("ArmyWarriors");
    priorityList.push_back("TechToArmy");
    priorityList.push_back("IncreaseArmyFacilities");
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
    const int currentlyUsed = BWAPI::Broodwar->self()->supplyUsed();
    const int workerBuffer = production->potentialSupplyUsed(
        production->getWorkerType());
    const int armyBuffer = production->potentialSupplyUsed(
        production->getArmyUnitType());
    const int expectedSupplyUsed = currentlyUsed + workerBuffer + armyBuffer;
    return production->expectedSupplyProvided() <= expectedSupplyUsed;
}

void DecisionSequence::createSupply() const {
    if (enoughResources(production->getSupplyType()))
        production->createSupply();
}

bool DecisionSequence::canProgressFor(const BWAPI::UnitType& unitType) const {
    try {
        return canBegin(production->getNextRequiredBuilding(unitType)); }
    catch (std::runtime_error) {
        return false; }
}

bool DecisionSequence::canBegin(const BWAPI::UnitType& unitType)  const {
    return enoughResources(unitType) && !isIncomplete(unitType);
}

bool DecisionSequence::isIncomplete(const BWAPI::UnitType& unitType) const {
    return BWAPI::Broodwar->self()->incompleteUnitCount(unitType) > 0;
}

bool DecisionSequence::enoughResources(const BWAPI::UnitType& unitType) const {
    return (BWAPI::Broodwar->self()->minerals() >= unitType.mineralPrice() - (
        unitType.isBuilding() ? 24 : 0));
}

void DecisionSequence::techTo(const BWAPI::UnitType& unitType) const {
    try {
        production->construct(production->getNextRequiredBuilding(unitType)); }
    catch (const std::runtime_error& e) {
        BWAPI::Broodwar << "techTo: " << e.what() << "!!!!!" << std::endl; }
}

bool DecisionSequence::canExtend(const BWAPI::UnitType& unitType) const {
    const BWAPI::UnitType& facilityType = (production->facilityFor(unitType));
    const int facilityCount = BWAPI::Broodwar->self()->allUnitCount(
        facilityType);
    const int expenseBuffer = facilityCount * unitType.mineralPrice() + 100,
              expense = facilityType.mineralPrice() + expenseBuffer;
    return BWAPI::Broodwar->self()->minerals() >= expense;
}

void DecisionSequence::increaseFacilities(
    const BWAPI::UnitType& unitType) const
{
    production->construct(production->facilityFor(unitType));
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
