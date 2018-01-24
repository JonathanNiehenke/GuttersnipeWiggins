#pragma once
#include "ArmyTrainer.h"

using namespace BWAPI::Filter;

void ArmyTrainer::includeFacility(const BWAPI::Unit& Facility) {
    productionFacilities.push_back(Facility);
}

void ArmyTrainer::removeFacility(const BWAPI::Unit& Facility) {
    auto endIt = productionFacilities.end(),
         foundIt = find(productionFacilities.begin(), endIt, Facility);
    if (foundIt != endIt)
        productionFacilities.erase(foundIt);
}

int ArmyTrainer::facilityCount() {
    return productionFacilities.size();
}

void ArmyTrainer::trainUnits(const BWAPI::UnitType& unitType) {
    for (BWAPI::Unit Facility: productionFacilities) {
        if (isTrainingQueueEmpty(Facility))
            Facility->train(unitType)
    }
}

bool ArmyTrainer::isTrainingQueueEmpty(BWAPI::Unit Facility) const {
    const BWAPI::Unitset& queue = (Facility->getType().producesLarva()
        ? Facility->getLarva() : Facility.getTrainingQueue());
    return queue.empty();
}
