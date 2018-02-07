#pragma once
#include "ZergRace.h"

void ZergRace::onUnitCreate(const BWAPI::Unit& createdUnit) const {
    if (createdUnit->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
        includeLarvaProducers();
    Race::onUnitCreate(createdUnit);
}

void ZergRace::includeLarvaProducers() const {
    for (const BWAPI::Unit& unit: BWAPI::Broodwar->self()->getUnits()) {
        if (unit->getType().producesLarva())
            armyTrainer->includeFacility(unit);
    }
}

void ZergRace::onUnitMorph(const BWAPI::Unit& morphedUnit) const {
    onUnitCreate(morphedUnit);
}

void ZergRace::onDestroyedBuilding(
    const BWAPI::Unit& destroyedBuilding) const
{
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
        removeLarvaProducers();
}

void ZergRace::removeLarvaProducers() const {
    for (const BWAPI::Unit& unit: BWAPI::Broodwar->self()->getUnits()) {
        if (unit->getType().producesLarva())
            armyTrainer->removeFacility(unit);
    }
}

void ZergRace::createSupply() const {
    resourceSupplier->createOverlord();
}

void ZergRace::construct(const BWAPI::UnitType& buildingType) const {
    if (buildingType == centerType || doesTechExist(buildingType))
        buildingConstructor->request(centerType);
    else
        buildingConstructor->request(buildingType);
}

bool ZergRace::doesTechExist(const BWAPI::UnitType& buildingType) const {
    return BWAPI::Broodwar->self()->allUnitCount(buildingType) > 0;
}
