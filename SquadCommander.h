#pragma once
#include <BWAPI.h>

class SquadCommander {
    private:
        class Squad;
        class TargetAssessor;
        Squad* rallyingSquad;
        std::vector<Squad*> deployedForce;
        bool isDeploymentReady();
        bool deploymentCondition(const BWAPI::Unitset);
        void uniteNearBySquads();
        void removeEmptySquads();
    public:
        void enlistForDeployment(const BWAPI::Unit& armyUnit);
        void removeFromDuty(const BWAPI::Unit& armyUnit);
        void updateGrouping();
        void updateTargeting() {
        void updateAttacking() {
};

class SquadCommander::Squad {
    private:
        BWAPI::Unitset members;
        BWAPI::Unitset enemyTargets;
    public:
        BWAPI::TilePosition aggresiveLocation, defensiveLocation;
        BWAPI::Position getAvgPosition() const;
        bool isEmpty() const;
        void assign(const BWAPI::Unit armyUnit);
        void remove(const BWAPI::Unit deadArmyUnit);
        void join(Squad& otherSquad);
        // void split(const int& newSquadAmount);
        void aquireTargets();
        void attack();
        void attackPosition();
        bool isAttackingPosition(const BWAPI::Unit& squdMember);
        void attackTargets();
        void isAttackingTarget(const BWAPI::Unit& squadMember);
        bool memberIsTargeting(const BWAPI::Unit&, const BWAPI::Unit&);
}
