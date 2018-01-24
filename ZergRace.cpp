#pragma once
#include "ZergRace.h"

void ZergRace::onUnitCreate(const BWAPI::Unit& createdUnit) {
    if (createdUnit->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
        includeLarvaProducers();
    Race::onCreate(createdUnit);
}

void ZergRace::includeLarvaProducers() {
    for (const BWAPI::Unit& unit: BWAPI::Broodwar->self()->getUnits()) {
        if (unit->getType().producesLarva())
            armyTrainer->includeFacility(unit);
    }
}

void ZergRace::onMorph(const BWAPI::Unit& morphedUnit) {
    onCreate(morphedUnit);
}

void ZergRace::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) {
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
        removeLarvaProducers();
}

void ZergRace::removeLarvaProducers() {
    for (const BWAPI::Unit& unit: BWAPI::Broodwar->self()->getUnits()) {
        if (unit->getType().producesLarva())
            armyTrainer->removeFacility(unit);
    }
}

void ZergRace::createSupply() {
    armyTrainer->trainUnits(supplyType);
}

bool ZergRace::doesTechExist(const BWAPI::UnitType& buildingType) {
    return BWAPI::Broodwar->self()->allUnitCount(buildingType) > 0;
}

void ZergRace::construct(const BWAPI::UnitType& buildingType)
{
    if (buildingType == centerType || doesTechExist(buildingType))
        buildingConstructor->request(centerType);
    else
        buildingConstructor->request(buildingType);
}

