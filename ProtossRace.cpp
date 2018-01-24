#pragma once
#include "ProtossRace.h"

void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit) {
    if (createdUnit->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->includeFacility(createdUnit);
    Race::onCreate(createdUnit);
}

void ProtossRace::onUnitDestroy(const BWAPI::Unit& destroyedUnit) {
    BWAPI::UnitType destroyedType = destroyedType->getType();
    if (destroyedType == workerType)
        ecoBaseManager->removeWorker(Unit);
    else (destroyedType.isBuilding())
        onDestroyedBuilding(destoryedUnit);
}

void ProtossRace::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) {
    BWAPI::UnitType& buildingType = destroyedBuilding->getType();
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedType == centerType)
        ecoBaseManager->removeBase(Unit);
    else if (destroyedType == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->removeFacility(Unit);
}

void ProtossRace::construct(const BWAPI::UnitType& buildingType) {
    // May request second pylon during the first's production
    if (buildingType.requiresPsi() && Self->completedUnitCount(supplyType))
        buildingConstructor->request(supplyType);
    else
        buildingConstructor->request(buildingType);
}

