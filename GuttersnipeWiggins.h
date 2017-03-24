#ifndef GUTTERSNIPEWIGGINS_H
#define GUTTERSNIPEWIGGINS_H
#include <vector>
#include <set>
#include <BWAPI.h>
#include "Utils.h"
#include "EcoBaseManager.h"

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
    EcoBaseManager ecoBaseManager;
    void GW::manageProduction();
    void GW::manageBases();
    void GW::manageAttackGroups();
    bool GW::needToGroup(BWAPI::Unitset Attackers);
    void GW::combatMicro();
    std::set<BWAPI::TilePosition> collectScoutingLocations();
    std::vector<BWAPI::TilePosition> getMineralClusterLocations();
    int getAvailableSupply();
    int getUnitBuffer(BWAPI::UnitType);
    void scout();
    void scoutLocations(
        std::vector<std::pair<BWAPI::Position, BWAPI::Unitset>> mapMinerals);
    void attackLocations(
        BWAPI::Unitset unitGroup,
        std::vector<std::pair<BWAPI::Position, BWAPI::Unitset>> mapMinerals);
    void appendAttackers();
    void attackEnemy(BWAPI::Unitset Attackers);
    void attackUnit(BWAPI::Unitset Attackers, BWAPI::Unit targetUnit);
    void GW::removeLocation(BWAPI::TilePosition Location);
    void removeLocation(BWAPI::Player Player, BWAPI::TilePosition Location);
    int getContractorTask(BWAPI::Unit contractorUnit);
    BWAPI::TilePosition getExpansionLocation(
        BWAPI::Unit centerContractor);
    void constructUnit(BWAPI::UnitType,
        BWAPI::TilePosition constructionLocation,
        BWAPI::Unit contractorUnit,
        int Task);
    void constructUnit(BWAPI::UnitType constructableType);
    void constructExpansion();
    void displayState();
};

#endif
