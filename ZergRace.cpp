#ifndef ZERGRACE_CPP
#define ZERGRACE_CPP
#include "ZergRace.h"

void ZergRace::handleEggType(BWAPI::Unit Unit)
{
    BWAPI::UnitType insideEggType = Unit->getBuildType();
    if (insideEggType == supplyType) {
        squadCommander->assembleSquad();  // Empty squads are Ok.
    }
    // Producting management must become an object.
    // PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
}


void ZergRace::onUnitCreate(BWAPI::Unit Unit)
{
    buildingConstructer->addProduct(Unit);
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Egg:
            handleEggType(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            unitTrainer->includeFacility(Unit);
            // Buffer management must be part of a shared object.
            // armyBuffer = getUnitBuffer(armyUnitType);
            break;
        default: 
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << "created!" << std::endl;
    }
}

void ZergRace::onUnitMorph(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            unitTrainer->includeFacility(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            // unitTrainer->includeFacility(baseCenter);
            // scout(cartographer->getStartingLocations());
            break;
        default: 
                BWAPI::Broodwar << "Unexpected Building created: "
                                << Unit->getType().c_str() << std::endl;
    }
    buildingConstructer->addProduct(Unit);
    // Previously: Incremented pending type.
    // Zerg workers become buildings, so recalculate supply.
}

void ZergRace::onCenterComplete(BWAPI::Unit Unit)
{
    // Prevent army hatcheries from becoming an EcoBases;
    if (!Unit->getClosestUnit(BWAPI::Filter::IsResourceDepot, 350)) // &&
        // Unit != baseCenter)
    {
        // Previously: Update worker buffer
        Race::onCenterComplete(Unit);
    }
}

void ZergRace::onUnitComplete(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
            try {
                ecoBaseManager->addWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            // Previously: Incremented supplyCount.
            buildingConstructer->removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            onCenterComplete(Unit);
            buildingConstructer->removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            buildingConstructer->removeConstruction(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << "completed!" << std::endl;
    }
}


void ZergRace::onUnitDestroy(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            squadCommander->removeWarrior(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
            try {
                ecoBaseManager->removeWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            if (Unit->isCompleted()) {
                // Previously: Decremented supplyCount.
            }
            buildingConstructer->removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            BWAPI::Broodwar->sendText("gg, you've proven more superior.");
            BWAPI::Broodwar->leaveGame();
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            ecoBaseManager->removeBase(Unit);
            buildingConstructer->removeConstruction(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << "destroyed!" << std::endl;
    }
}


#endif

