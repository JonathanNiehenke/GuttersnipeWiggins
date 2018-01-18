#ifndef GUTTERSNIPEWIGGINS_H
#define GUTTERSNIPEWIGGINS_H
#include <vector>
#include <set>
#include <BWAPI.h>
#include "BuildingConstructor.h"
#include "Cartographer.h"
#include "CmdRescuer.h"
#include "EcoBaseManager.h"
#include "SquadCommander.h"
#include "UnitTrainer.h"
#include "Race.h"
#include "ProtossRace.h"
#include "TerranRace.h"
#include "ZergRace.h"

class GW : public BWAPI::AIModule
{
private:
    int availableSupply = 0, workerBuffer = 0, armyBuffer = 0, supplyCount = 0;
    BWAPI::Player Self;
    // Indicates number already in construction/training for UnitType.
    BuildingConstructor buildingConstructor;
    // Tracks resource, enemy and army facility locations.
    Cartographer cartographer;
    CmdRescuer::Rescuer cmdRescuer;
    EcoBaseManager ecoBaseManager;
    Race *race;  // onStart initializes with polymorphic intentions.
    SquadCommander squadCommander;
    UnitTrainer unitTrainer;
    void displayUnitInfo();
    void displayStatus();

public:
    virtual void onStart();
    virtual void onFrame();
    virtual void onUnitCreate(BWAPI::Unit unit);
    virtual void onUnitMorph(BWAPI::Unit unit);
    virtual void onUnitComplete(BWAPI::Unit unit);
    virtual void onUnitDestroy(BWAPI::Unit unit);
    virtual void onUnitDiscover(BWAPI::Unit unit);
    virtual void onUnitEvade(BWAPI::Unit unit);
    virtual void onUnitShow(BWAPI::Unit unit);
    virtual void onUnitHide(BWAPI::Unit unit);
    virtual void onUnitRenegade(BWAPI::Unit unit);
    virtual void onNukeDetect(BWAPI::Position target);
    virtual void onSendText(std::string text);
    virtual void onReceiveText(BWAPI::Player player, std::string text);
    virtual void onSaveGame(std::string gameName);
    virtual void onPlayerLeft(BWAPI::Player player);
    virtual void onEnd(bool isWinner);

};

#endif
