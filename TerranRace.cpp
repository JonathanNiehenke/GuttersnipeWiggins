#pragma once
#include "TerranRace.h"

void TerranRace::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) const {
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Terran_Barracks)
        unitTrainer->removeFacility(destroyedBuilding);
}
