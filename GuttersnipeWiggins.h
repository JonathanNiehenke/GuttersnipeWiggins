#pragma once
#include <vector>
#include <set>
#include <BWAPI.h>
#include "Cartographer.h"
#include "SquadCommander.h"
#include "DecisionSequence.h"
#include "Production.h"

class GW : public BWAPI::AIModule
{
private:
    int availableSupply = 0, workerBuffer = 0, armyBuffer = 0, supplyCount = 0;
    BWAPI::Player Self;
    Cartographer* cartographer;
    Production* production;
    SquadCommander* squadCommander;
    DecisionSequence* decisionSequence;
    void drawStatus() const;
    void drawSelectedUnitInfo() const;

public:
    GW();
    ~GW();
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
