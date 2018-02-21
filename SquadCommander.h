#pragma once
#include <BWAPI.h>
#include "Combat.h"

namespace {

    class Squad {
        private:
            BWAPI::Unitset members;
            Combat combat;
            bool isJoinable(const Squad& otherSquad) const;
        public:
            Squad(const BWAPI::Unitset& units);
            BWAPI::Position getAvgPosition() const;
            bool isEmpty() const { return members.empty(); };
            void assign(const BWAPI::Unit& armyUnit);
            void remove(const BWAPI::Unit& deadArmyUnit);
            void join(Squad& otherSquad);
            // void split(const int& newSquadAmount);
            void attackPosition(const BWAPI::Position& position);
            void prepareCombat();
            void engageCombat() const;
    };
}

class SquadCommander {
    private:
        std::vector<Squad> deployedForces;
        void funnel();
        void terminate();
    public:
        void focus(const BWAPI::Position& attackPosition);
        void incorporate(const BWAPI::Unitset&);
        void deactivate(const BWAPI::Unit& armyUnit);
        void group();
        void prepare();
        void engage() const;
        void drawStatus(int& row) const;
};
