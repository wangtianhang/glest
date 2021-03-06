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

#ifndef _GLEST_GAME_AIINTERFACE_H_
#define _GLEST_GAME_AIINTERFACE_H_

#include "world.h"
#include "commander.h"
#include "command.h"
#include "conversion.h"
#include "ai.h"

using Shared::Util::intToStr;

namespace Glest{ namespace Game{

// =====================================================
// 	class AiInterface  
//
///	The AI will interact with the game through this interface
// =====================================================

class AiInterface{
private:
    World *world;
    Commander *commander;
    Console *console;
    Ai ai;

    int timer;
    int factionIndex;
    int teamIndex;

	//config
	bool redir;
    int logLevel;

public:
    AiInterface(Game &game, int factionIndex, int teamIndex);

	//main
    void update();

	//get
	int getTimer() const		{return timer;}
	int getFactionIndex() const	{return factionIndex;}

    //misc
    void printLog(int logLevel, const string &s);
    
    //interact
    CommandResult giveCommand(int unitIndex, CommandClass commandClass, const Vec2i &pos=Vec2i(0));
    CommandResult giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos, const UnitType* unitType);
    CommandResult giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos);
    CommandResult giveCommand(int unitIndex, const CommandType *commandType, Unit *u= NULL);
    
    //get data
    int getMapMaxPlayers();
    Vec2i getHomeLocation();
    Vec2i getStartLocation(int locationIndex);
    int getFactionCount();
    int getMyUnitCount() const;
	int getMyUpgradeCount() const;
    int onSightUnitCount();
    const Resource *getResource(const ResourceType *rt);
    const Unit *getMyUnit(int unitIndex);
    const Unit *getOnSightUnit(int unitIndex);
    const FactionType *getMyFactionType();
    const TechTree *getTechTree(); 
    bool getNearestSightedResource(const ResourceType *rt, const Vec2i &pos, Vec2i &resultPos);
    bool isAlly(const Unit *unit) const;
	bool isAlly(int factionIndex) const;
	bool reqsOk(const RequirableType *rt); 
	bool reqsOk(const CommandType *ct); 
    bool checkCosts(const ProducibleType *pt);
	bool isFreeCells(const Vec2i &pos, int size, Field field);

private:
	string getLogFilename() const	{return "ai"+intToStr(factionIndex)+".log";}
};

}}//end namespace

#endif
