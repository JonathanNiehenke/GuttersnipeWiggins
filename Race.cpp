#ifndef RACE_CPP
#define RACE_CPP
#include "Race.h"

// To reduce the parameters list of Race and derived constructors.
Core::Core(
    BuildingConstructer *buildingConstructer,
    Cartographer *cartographer,
    CmdRescuer::Rescuer *cmdRescuer,
    EcoBaseManager *ecoBaseManager,
    SquadCommander *squadCommander,
    UnitTrainer *unitTrainer)
{
    this->buildingConstructer = buildingConstructer;
    this->cartographer = cartographer;
    this->cmdRescuer = cmdRescuer;
    this->ecoBaseManager = ecoBaseManager;
    this->squadCommander = squadCommander;
    this->unitTrainer = unitTrainer;
}


Race::Race(
    BWAPI::UnitType centerType,
    BWAPI::UnitType workerType,
    BWAPI::UnitType supplyType,
    BWAPI::UnitType armyTechType,
    BWAPI::UnitType armyUnitType,
    Core &core)
{
    this->centerType = centerType;
    this->workerType = workerType;
    this->supplyType = supplyType;
    this->armyTechType = armyTechType;
    this->armyUnitType = armyUnitType;
    this->buildingConstructer = core.buildingConstructer;
    this->cartographer = core.cartographer;
    this->cmdRescuer = core.cmdRescuer;
    this->ecoBaseManager = core.ecoBaseManager;
    this->squadCommander = core.squadCommander;
    this->unitTrainer = core.unitTrainer;
}

void Race::onUnitCreate(BWAPI::Unit Unit)
{
    // Previously: Incremented pending type.
}

void Race::onCenterComplete(BWAPI::Unit Unit)
{
    // ToDo: Rewrite.
    for (BWAPI::Unitset mineralCluster: cartographer->getMinerals()) {
        BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
            mineralCluster.getPosition(), BWAPI::Filter::IsResourceDepot, 300);
        if (baseCenter == Unit) {
            BWAPI::Broodwar << "Creating EcoBase" << std::endl;
            ecoBaseManager->addBase(baseCenter, mineralCluster);
            break;
        }
    }
}

#endif
