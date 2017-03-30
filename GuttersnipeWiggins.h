#ifndef GUTTERSNIPEWIGGINS_H
#define GUTTERSNIPEWIGGINS_H
#include <vector>
#include <set>
#include <BWAPI.h>
#include "Utils.h"
#include "CmdRescuer.h"
#include "EcoBaseManager.h"
#include "SquadCommander.h"
#include "Cartographer.h"
#include "BuildingConstructer.h"

class GW : public BWAPI::AIModule
{
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

private:
    std::vector<std::pair<BWAPI::Position, BWAPI::Unitset>> getMapMinerals();
    CmdRescuer::Rescuer cmdRescuer;
    EcoBaseManager ecoBaseManager;
    Cartographer cartographer;
    SquadCommander squadCommander;
    BuildingConstructer buildingConstructer;
    void GW::manageProduction();
    void GW::manageBases();
    void GW::manageAttackGroups();
    std::vector<BWAPI::TilePosition> getMineralClusterLocations();
    int getAvailableSupply();
    int getUnitBuffer(BWAPI::UnitType);
    void scout(std::set<BWAPI::TilePosition> scoutLocations);
    // void constructExpansion();
    void displayUnitInfo();
    void displayStatus();
};

#endif
