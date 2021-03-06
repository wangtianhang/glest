// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_WORLD_H_
#define _GLEST_GAME_WORLD_H_

#include "vec.h"
#include "math_util.h"
#include "resource.h"
#include "tech_tree.h"
#include "tileset.h"
#include "console.h"
#include "map.h"
#include "scenario.h"
#include "minimap.h"
#include "logger.h"
#include "stats.h"
#include "time_flow.h"
#include "upgrade.h"
#include "water_effects.h"
#include "faction.h"
#include "unit_updater.h"
#include "random.h"
#include "game_constants.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Quad2i;
using Shared::Graphics::Rect2i;
using Shared::Util::Random;

class Faction;
class Unit;
class Config;
class Game;
class GameSettings;
class ScriptManager;

// =====================================================
// 	class World
//
///	The game world: Map + Tileset + TechTree
// =====================================================

class World{
private:
	typedef vector<Faction> Factions;

public:
	static const int generationArea= 100;
	static const float airHeight;
	static const int indirectSightRange= 5;

private:

	Map map;
	Tileset tileset;
	TechTree techTree;
	TimeFlow timeFlow;
	Scenario scenario;

	UnitUpdater unitUpdater;
    WaterEffects waterEffects;
	Minimap minimap;
    Stats stats;	//BattleEnd will delete this object

	Factions factions;

	Random random;

	ScriptManager* scriptManager;

	int thisFactionIndex;
	int thisTeamIndex;
	int frameCount;
	int nextUnitId;

	//config
	bool fogOfWar;
	int fogOfWarSmoothingFrameSkip;
	bool fogOfWarSmoothing;
	
public:
	World();
	void end(); //to die before selection does

	//get			
	int getMaxPlayers() const						{return map.getMaxPlayers();}
	int getThisFactionIndex() const					{return thisFactionIndex;}
	int getThisTeamIndex() const					{return thisTeamIndex;}
	const Faction *getThisFaction() const			{return &factions[thisFactionIndex];}
	int getFactionCount() const						{return factions.size();}
	const Map *getMap() const 						{return &map;}
	const Tileset *getTileset() const 				{return &tileset;}
	const TechTree *getTechTree() const 			{return &techTree;}
	const Scenario *getScenario() const 			{return &scenario;}
	const TimeFlow *getTimeFlow() const				{return &timeFlow;}
	Tileset *getTileset() 							{return &tileset;}
	Map *getMap() 									{return &map;}
	const Faction *getFaction(int i) const			{return &factions[i];}
	Faction *getFaction(int i) 						{return &factions[i];}
	const Minimap *getMinimap() const				{return &minimap;}
	const Stats *getStats() const					{return &stats;};
	Stats *getStats()								{return &stats;};
	const WaterEffects *getWaterEffects() const		{return &waterEffects;}
	int getNextUnitId()								{return nextUnitId++;}
	int getFrameCount() const						{return frameCount;}

	//init & load
	void init(Game *game, bool createUnits);
	void loadTileset(const string &dir, Checksum* checksum);
	void loadTech(const string &dir, Checksum* checksum);
	void loadMap(const string &path, Checksum* checksum);
	void loadScenario(const string &path, Checksum* checksum);
	
	//misc
	void update();
	Unit* findUnitById(int id);
	const UnitType* findUnitTypeById(const FactionType* factionType, int id);
	bool placeUnit(const Vec2i &startLoc, int radius, Unit *unit, bool spaciated= false);
	void moveUnitCells(Unit *unit);
	bool toRenderUnit(const Unit *unit, const Quad2i &visibleQuad) const;
	bool toRenderUnit(const Unit *unit) const;
	Unit *nearestStore(const Vec2i &pos, int factionIndex, const ResourceType *rt);

	//scripting interface
	void createUnit(const string &unitName, int factionIndex, const Vec2i &pos);
	void givePositionCommand(int unitId, const string &commandName, const Vec2i &pos);
	void giveProductionCommand(int unitId, const string &producedName);
	void giveUpgradeCommand(int unitId, const string &upgradeName);
	void giveResource(const string &resourceName, int factionIndex, int amount);
	int getResourceAmount(const string &resourceName, int factionIndex);
	Vec2i getStartLocation(int factionIndex);
	Vec2i getUnitPosition(int unitId);
	int getUnitFactionIndex(int unitId);
	int getUnitCount(int factionIndex);
	int getUnitCountOfType(int factionIndex, const string &typeName);

private:
	
	void initCells();
	void initSplattedTextures();
	void initFactionTypes(GameSettings *gs);
	void initMinimap();
	void initUnits();
	void initMap();
	void initExplorationState();
	
	//misc
	void tick();
	void computeFow();
	void exploreCells(const Vec2i &newPos, int sightRange, int teamIndex);
};

}}//end namespace

#endif
