#pragma once
#include <map>
#include <BWAPI.h>
#include "Utils.h"

typedef std::pair<BWAPI::Position, BWAPI::UnitType> PositionalType;
typedef std::map<BWAPI::Unit,PositionalType>::const_iterator CurIt;
typedef std::map<BWAPI::Position, BWAPI::UnitType>::const_iterator FogIt;
typedef std::function<bool(const BWAPI::UnitType&)> Pred;

class EnemyTracker {
    private:
        class Filter;
        class iterator;
        std::map<BWAPI::Unit, PositionalType> currentPositions;
        std::map<BWAPI::Position, BWAPI::UnitType> foggyPositions;
        void updateCurrentPositions();
        void updateFoggyPositions();
        static bool isInFog(const BWAPI::Position& position);
        void moveToFoggyPositions(
            const std::pair<BWAPI::Unit, PositionalType>& pair);
    public:
        Filter filter(Pred pred);
        iterator begin();
        iterator end();
        bool empty() const;
        void addUnit(const BWAPI::Unit& unit);
        void removeUnit(const BWAPI::Unit& unit);
        void update();
        BWAPI::Position getClosestEnemyPosition(
            const BWAPI::Position& sourcePosition) const;
};

class EnemyTracker::iterator {
    private:
        CurIt cIt, cEnd;
        FogIt fIt, fEnd;
    public:
        iterator(CurIt cIt, CurIt cEnd, FogIt fIt, FogIt fEnd) :
            cIt(cIt), cEnd(cEnd), fIt(fIt), fEnd(fEnd) {}
        iterator& operator++();
        bool operator==(iterator other) const;
        bool operator!=(iterator other) const;
        PositionalType operator*() const;
};

class EnemyTracker::Filter {
    private:
        class iterator;
        EnemyTracker* eTracker;
        Pred pred;
    public:
        Filter(EnemyTracker* eTracker, Pred pred) :
            eTracker(eTracker), pred(pred) {}
        iterator begin();
        iterator end();
};

class EnemyTracker::Filter::iterator {
    private:
        EnemyTracker::iterator It, itEnd;
        Pred pred;
        void toNextPostive();
    public:
        iterator(EnemyTracker::iterator It, EnemyTracker::iterator itEnd,
            Pred pred) : It(It), itEnd(itEnd), pred(pred) { toNextPostive(); }
        iterator& operator++();
        bool operator==(iterator other) const;
        bool operator!=(iterator other) const;
        PositionalType operator*() const;
};
