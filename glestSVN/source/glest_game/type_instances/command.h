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

#ifndef _GLEST_GAME_COMMAND_H_
#define _GLEST_GAME_COMMAND_H_

#include <cstdlib>

#include "unit.h"
#include "vec.h"

namespace Glest{ namespace Game{

using Shared::Graphics::Vec2i;

class CommandType;

// =====================================================
// 	class Command
//
///	A unit command
// =====================================================

class Command{
private:
    const CommandType *commandType;
    Vec2i pos;
	UnitReference unitRef;		//target unit, used to move and attack optinally
	const UnitType *unitType;	//used for build

public:
    //constructor
    Command(const CommandType *ct, const Vec2i &pos=Vec2i(0)); 
    Command(const CommandType *ct, Unit *unit); 
    Command(const CommandType *ct, const Vec2i &pos, const UnitType *unitType); 

    //get
	const CommandType *getCommandType() const	{return commandType;}
	Vec2i getPos() const						{return pos;}
	Unit* getUnit() const						{return unitRef.getUnit();}
	const UnitType* getUnitType() const			{return unitType;}

    //set 
    void setCommandType(const CommandType *commandType);
    void setPos(const Vec2i &pos);
    void setUnit(Unit *unit);
};   

}}//end namespace

#endif
