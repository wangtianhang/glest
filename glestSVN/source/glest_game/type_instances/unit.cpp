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
#include "faction.h" 

#include <cassert>

#include "unit.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class UnitPath
// =====================================================

const int UnitPath::maxBlockCount= 10;

bool UnitPath::isEmpty(){
	return pathQueue.empty();
}

bool UnitPath::isBlocked(){
	return blockCount>=maxBlockCount;
}

void UnitPath::clear(){
	pathQueue.clear();
	blockCount= 0;
}

void UnitPath::incBlockCount(){
	pathQueue.clear();
	blockCount++;
}

void UnitPath::push(const Vec2i &path){
	pathQueue.push_back(path);
}

Vec2i UnitPath::pop(){
	Vec2i p= pathQueue.front();
	pathQueue.erase(pathQueue.begin());
	return p;
}

// =====================================================
// 	class UnitReference
// =====================================================

UnitReference::UnitReference(){
	id= -1;
	faction= NULL;
}

void UnitReference::operator=(const Unit *unit){
	if(unit==NULL){
		id= -1;
		faction= NULL;
	}
	else{
		id= unit->getId();
		faction= unit->getFaction();
	}
}

Unit *UnitReference::getUnit() const{
	if(faction!=NULL){
		return faction->findUnit(id);
	}
	return NULL;
}

// =====================================================
// 	class Unit
// =====================================================

const float Unit::speedDivider= 100.f;
const int Unit::maxDeadCount= 500;	//time in until the corpse disapears
const float Unit::highlightTime= 0.5f;
const int Unit::invalidId= -1;

// ============================ Constructor & destructor =============================

Unit::Unit(int id, const Vec2i &pos, const UnitType *type, Faction *faction, Map *map){
    Random random;
	
	this->pos=pos;
	this->type=type;
    this->faction=faction;
	this->map= map;

	this->id= id;
	lastPos= pos;
    progress= 0;
	lastAnimProgress= 0;
    animProgress= 0;
    progress2= 0;
	kills= 0;
	loadCount= 0;
    ep= 0;
	deadCount= 0;
	hp= type->getMaxHp()/20;
	toBeUndertaken= false;
	level= NULL;
	highlight= 0.f;
	meetingPos= pos;
	alive= true;

	float rot= 0.f;

	random.init(id);
	rot+= random.randRange(-5, 5);
	
	rotation= rot;
	lastRotation= rot;
	targetRotation= rot;

    if(getType()->getField(fAir)) currField=fAir;
    if(getType()->getField(fLand)) currField=fLand;

    fire= NULL;

	computeTotalUpgrade();

	//starting skill
	currSkill= getType()->getFirstStOfClass(scStop);
}

Unit::~Unit(){
	//remove commands
	while(!commands.empty()){
		delete commands.back();
		commands.pop_back();
	}
}

// ====================================== get ======================================

int Unit::getFactionIndex() const{
	return faction->getIndex();
}

int Unit::getTeam() const{
	return faction->getTeam();
}

Vec2i Unit::getCenteredPos() const{
    return pos + Vec2i(type->getSize()/2, type->getSize()/2);
}

Vec2f Unit::getFloatCenteredPos() const{
	return Vec2f(pos.x-0.5f+type->getSize()/2.f, pos.y-0.5f+type->getSize()/2.f);
}

Vec2i Unit::getCellPos() const{

	if(type->hasCellMap()){
		
		//find nearest pos to center that is free
		Vec2i centeredPos= getCenteredPos();
		float nearestDist= -1.f;
		Vec2i nearestPos= pos;

		for(int i=0; i<type->getSize(); ++i){
			for(int j=0; j<type->getSize(); ++j){
				if(type->getCellMapCell(i, j)){
					Vec2i currPos= pos + Vec2i(i, j);
					float dist= currPos.dist(centeredPos);
					if(nearestDist==-1.f || dist<nearestDist){
						nearestDist= dist;
						nearestPos= currPos;
					}
				}
			}
		}
		return nearestPos;
	}
	return pos;
}

float Unit::getVerticalRotation() const{
	/*if(type->getProperty(UnitType::pRotatedClimb) && currSkill->getClass()==scMove){
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		float dist= pos.dist(targetPos);
		return radToDeg(atan2(heightDiff, dist));
	}*/
	return 0.f;
}

int Unit::getProductionPercent() const{
	if(anyCommand()){
		const ProducibleType *produced= commands.front()->getCommandType()->getProduced();
		if(produced!=NULL){
			return clamp(progress2*100/produced->getProductionTime(), 0, 100);
		}
	}
	return -1;
}

float Unit::getHpRatio() const{
	return clamp(static_cast<float>(hp)/type->getTotalMaxHp(&totalUpgrade), 0.f, 1.f);
}

float Unit::getEpRatio() const{
	if(type->getMaxHp()==0){ 
		return 0.f;
	}
	else{
		return clamp(static_cast<float>(ep)/type->getTotalMaxEp(&totalUpgrade), 0.f, 1.f);
	}
}

const Level *Unit::getNextLevel() const{
	if(level==NULL && type->getLevelCount()>0){
		return type->getLevel(0);
	}
	else{
		for(int i=1; i<type->getLevelCount(); ++i){
			if(type->getLevel(i-1)==level){
				return type->getLevel(i);
			}
		}
	}
	return NULL;
}

string Unit::getFullName() const{
	string str;
	if(level!=NULL){
		str+= level->getName() + " ";
	}
	str+= type->getName();
	return str;
}

// ====================================== is ======================================

bool Unit::isOperative() const{
    return isAlive() && isBuilt();
}

bool Unit::isBeingBuilt() const{
    return currSkill->getClass()==scBeBuilt;
}
        
bool Unit::isBuilt() const{
    return !isBeingBuilt();
}

bool Unit::isPutrefacting() const{
	return deadCount!=0;
}

bool Unit::isAlly(const Unit *unit) const{
	return faction->isAlly(unit->getFaction());
}

bool Unit::isDamaged() const{
	return hp < type->getTotalMaxHp(&totalUpgrade);
}

bool Unit::isInteresting(InterestingUnitType iut) const{
	switch(iut){
	case iutIdleHarvester: 
		if(type->hasCommandClass(ccHarvest)){
			if(!commands.empty()){
				const CommandType *ct= commands.front()->getCommandType();
				if(ct!=NULL){
					return ct->getClass()==ccStop;
				}
			}
		}
		return false;

	case iutBuiltBuilding: 
		return type->hasSkillClass(scBeBuilt) && isBuilt();
	case iutProducer: 
		return type->hasSkillClass(scProduce);
	case iutDamaged: 
		return isDamaged();
	case iutStore:
		return type->getStoredResourceCount()>0;
	default:
		return false;
	}
}

// ====================================== set ======================================

void Unit::setCurrSkill(const SkillType *currSkill){
	if(currSkill->getClass()!=this->currSkill->getClass()){
		animProgress= 0;
		lastAnimProgress= 0;
	}
	progress2= 0;
	this->currSkill= currSkill;
}

void Unit::setCurrSkill(SkillClass sc){
    setCurrSkill(getType()->getFirstStOfClass(sc));
}

void Unit::setTarget(const Unit *unit){

	//find a free pos in cellmap
	setTargetPos(unit->getCellPos());

	//ser field and vector
	targetField= unit->getCurrField();
	targetVec= unit->getCurrVector();
	targetRef= unit;
}

void Unit::setPos(const Vec2i &pos){
	this->lastPos= this->pos;
	this->pos= pos;
	this->meetingPos= pos - Vec2i(1);
}

void Unit::setTargetPos(const Vec2i &targetPos){

	Vec2i relPos= targetPos - pos;
	Vec2f relPosf= Vec2f(relPos.x, relPos.y);
	targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));
	targetRef= NULL;

	this->targetPos= targetPos;
}	

// =============================== Render related ==================================

const Model *Unit::getCurrentModel() const{
    return currSkill->getAnimation(); 
}

Vec3f Unit::getCurrVector() const{
	return getCurrVectorFlat() + Vec3f(0.f, type->getHeight()/2.f, 0.f);
}

Vec3f Unit::getCurrVectorFlat() const{
    Vec3f v;

	float y1= computeHeight(lastPos);
	float y2= computeHeight(pos);
				
    if(currSkill->getClass()==scMove){
        v.x= lastPos.x + progress * (pos.x-lastPos.x);
        v.z= lastPos.y + progress * (pos.y-lastPos.y);
		v.y= y1+progress*(y2-y1); 
    }
    else{
        v.x= static_cast<float>(pos.x);
        v.z= static_cast<float>(pos.y);
        v.y= y2;
    }
    v.x+= type->getSize()/2.f-0.5f;
    v.z+= type->getSize()/2.f-0.5f;

    return v;
}

// =================== Command list related ===================

//any command
bool Unit::anyCommand() const{
	return !commands.empty();
}		

//return current command, assert that there is always one command
Command *Unit::getCurrCommand() const{
	assert(!commands.empty());
	return commands.front();
}

//give one command (clear, and push back)
CommandResult Unit::giveCommand(Command *command){
	
	if(command->getCommandType()->isQueuable()){
		//cancel current command if it is not queuable
		if(!commands.empty() && !commands.front()->getCommandType()->isQueuable()){
			cancelCommand();
		}
	}
	else{
		//empty command queue
		clearCommands();
		unitPath.clear();
	}
	
	//check command
	CommandResult result= checkCommand(command);
	if(result==crSuccess){
		applyCommand(command);
	}

	//push back command
	if(result== crSuccess){
		commands.push_back(command);
	}
	else{
		delete command;
	}

	return result;
} 

//pop front (used when order is done)
CommandResult Unit::finishCommand(){

	//is empty?
	if(commands.empty()){
		return crFailUndefined;
	}

	//pop front
	delete commands.front();
	commands.erase(commands.begin());
	unitPath.clear();

	return crSuccess;
}

//to cancel a command
CommandResult Unit::cancelCommand(){
 
	//is empty?
	if(commands.empty()){
		return crFailUndefined;
	}

	//undo command
	undoCommand(commands.back());

	//delete ans pop command
	delete commands.back();
	commands.pop_back();

	//clear routes
	unitPath.clear();

	return crSuccess;
}

// =================== route stack ===================

void Unit::create(bool startingUnit){
	faction->addUnit(this);
	map->putUnitCells(this, pos);
	if(startingUnit){
		faction->applyStaticCosts(type);
	}
}

void Unit::born(){
	faction->addStore(type);
	faction->applyStaticProduction(type);	
	setCurrSkill(scStop);
	hp= type->getMaxHp();
}

void Unit::kill(){
	
	//no longer needs static resources
	if(isBeingBuilt()){
		faction->deApplyStaticConsumption(type);
	}
	else{
		faction->deApplyStaticCosts(type);
	}

	//do the cleaning
	map->clearUnitCells(this, pos);
	if(!isBeingBuilt()){
		faction->removeStore(type);
	}
    setCurrSkill(scDie);

	notifyObservers(UnitObserver::eKill);

	//clear commands
	clearCommands();
}

void Unit::undertake(){
	faction->removeUnit(this);
}

// =================== Referencers ===================

void Unit::addObserver(UnitObserver *unitObserver){
	observers.push_back(unitObserver);
}

void Unit::removeObserver(UnitObserver *unitObserver){
	observers.remove(unitObserver);
}

void Unit::notifyObservers(UnitObserver::Event event){
	for(Observers::iterator it= observers.begin(); it!=observers.end(); ++it){
		(*it)->unitEvent(event, this);
	}
}

// =================== Other ===================

void Unit::resetHighlight(){
	highlight= 1.f;
}

const CommandType *Unit::computeCommandType(const Vec2i &pos, const Unit *targetUnit) const{
	const CommandType *commandType= NULL;
	SurfaceCell *sc= map->getSurfaceCell(Map::toSurfCoords(pos));

	if(targetUnit!=NULL){
		//attack enemies
		if(!isAlly(targetUnit)){
			commandType= type->getFirstAttackCommand(targetUnit->getCurrField());
		}	

		//repair allies
		else{
			commandType= type->getFirstRepairCommand(targetUnit->getType());
		}
	}
	else{
		//check harvest command
		Resource *resource= sc->getResource();
		if(resource!=NULL){
			commandType= type->getFirstHarvestCommand(resource->getType());
		}
	}

	//default command is move command
	if(commandType==NULL){
		commandType= type->getFirstCtOfClass(ccMove);
	}

	return commandType;
}

bool Unit::update(){
	assert(progress<=1.f);

	//highlight
	if(highlight>0.f){
		highlight-= 1.f/(highlightTime*GameConstants::updateFps);
	}

	//speed
	int speed= currSkill->getTotalSpeed(&totalUpgrade);
	
	//speed modifier
	float diagonalFactor= 1.f;
	float heightFactor= 1.f;
	if(currSkill->getClass()==scMove){
		
		//if moving in diagonal move slower
		Vec2i dest= pos-lastPos;
		if(abs(dest.x)+abs(dest.y) == 2){
			diagonalFactor= 0.71f;
		}

		//if movig to an higher cell move slower else move faster
		float heightDiff= map->getCell(pos)->getHeight() - map->getCell(targetPos)->getHeight();
		heightFactor= clamp(1.f+heightDiff/5.f, 0.2f, 5.f);
	}

	//update progresses
	lastAnimProgress= animProgress;
	progress+= (speed*diagonalFactor*heightFactor)/(speedDivider*GameConstants::updateFps);
	animProgress+= (currSkill->getAnimSpeed()*heightFactor)/(speedDivider*GameConstants::updateFps);

	//update target
	updateTarget();

	//rotation
	if(currSkill->getClass()!=scStop){
		const int rotFactor= 2;
		if(progress<1.f/rotFactor){
			if(type->getFirstStOfClass(scMove)){
				if(abs(lastRotation-targetRotation)<180)
					rotation= lastRotation+(targetRotation-lastRotation)*progress*rotFactor;
				else{
					float rotationTerm= targetRotation>lastRotation? -360.f: +360.f;
					rotation= lastRotation+(targetRotation-lastRotation+rotationTerm)*progress*rotFactor;
				}
			}
		}
	}

	//checks
	if(animProgress>1.f){
		animProgress= currSkill->getClass()==scDie? 1.f: 0.f;
	}

	//checks
	if(progress>=1.f){ 
		lastRotation= targetRotation;
		if(currSkill->getClass()!=scDie){
			progress= 0.f;
			return true;
		}
		else{
			progress= 1.f;
			deadCount++;
			if(deadCount>=maxDeadCount){
				toBeUndertaken= true;			
				return false;
			}
		}
	}

	return false;
}

void Unit::tick(){

	if(isAlive()){
		//regenerate hp
		hp+= type->getHpRegeneration();
		if(hp>type->getTotalMaxHp(&totalUpgrade)){
			hp= type->getTotalMaxHp(&totalUpgrade);
		}

		//regenerate ep
		ep+= type->getEpRegeneration();
		if(ep>type->getTotalMaxEp(&totalUpgrade)){
			ep= type->getTotalMaxEp(&totalUpgrade);
		}
	}
}

int Unit::update2(){
     progress2++;
     return progress2;
}

bool Unit::computeEp(){
    
	//if not enough ep
    if(ep-currSkill->getEpCost() < 0){ 
        return true;
    }

	//decrease ep
    ep-= currSkill->getEpCost();
	if(ep>getType()->getTotalMaxEp(&totalUpgrade)){
        ep= getType()->getTotalMaxEp(&totalUpgrade); 
	}

    return false;
}

bool Unit::repair(){
	
	//increase hp
	hp+= getType()->getMaxHp()/type->getProductionTime() + 1;
    if(hp>(getType()->getTotalMaxHp(&totalUpgrade))){
        hp= getType()->getTotalMaxHp(&totalUpgrade);
        return true;
    }

	//stop fire
	if(hp>type->getMaxHp()/2 && fire!=NULL){
		fire->fade();
		fire= NULL;
	}
    return false;
}

//decrements HP and returns if dead
bool Unit::decHp(int i){
	if(hp==0){
		return false;
	}
	
	hp-=i;

	//fire
	if(type->getProperty(UnitType::pBurnable) && hp<type->getMaxHp()/2 && fire==NULL){
		FireParticleSystem *fps;
		fps= new FireParticleSystem(200);
		fps->setSpeed(2.5f/GameConstants::updateFps);
		fps->setPos(getCurrVector());
		fps->setRadius(type->getSize()/3.f);
		fps->setTexture(CoreData::getInstance().getFireTexture());
		fps->setParticleSize(type->getSize()/3.f);
		fire= fps;
		Renderer::getInstance().manageParticleSystem(fps, rsGame);
	}

	//stop fire on death
    if(hp<=0){
		alive= false;
        hp=0;
		if(fire!=NULL){
			fire->fade();
			fire= NULL;
		}
		return true;
    }
    return false;
}

string Unit::getDesc() const{

    Lang &lang= Lang::getInstance();

    //pos
    //str+="Pos: "+v2iToStr(pos)+"\n";

	//hp
	string str= "\n" + lang.get("Hp")+ ": " + intToStr(hp) + "/" + intToStr(type->getTotalMaxHp(&totalUpgrade));
	if(type->getHpRegeneration()!=0){
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(type->getHpRegeneration()) + ")";
	}

	//ep
	if(getType()->getMaxEp()!=0){
		str+= "\n" + lang.get("Ep")+ ": " + intToStr(ep) + "/" + intToStr(type->getTotalMaxEp(&totalUpgrade));
	}
	if(type->getEpRegeneration()!=0){
		str+= " (" + lang.get("Regeneration") + ": " + intToStr(type->getEpRegeneration()) + ")";
	}

	//armor
	str+= "\n" + lang.get("Armor")+ ": " + intToStr(getType()->getArmor());
	if(totalUpgrade.getArmor()!=0){
		str+="+"+intToStr(totalUpgrade.getArmor());
	}
	str+= " ("+getType()->getArmorType()->getName()+")";

	//sight
	str+="\n"+ lang.get("Sight")+ ": " + intToStr(getType()->getSight());
	if(totalUpgrade.getSight()!=0){
		str+="+"+intToStr(totalUpgrade.getSight());
	}

	//kills
	const Level *nextLevel= getNextLevel();
	if(kills>0 || nextLevel!=NULL){
		str+= "\n" + lang.get("Kills") +": " + intToStr(kills);
		if(nextLevel!=NULL){
			str+= " (" + nextLevel->getName() + ": " + intToStr(nextLevel->getKills()) + ")";
		}
	}

	//str+= "\nskl: "+scToStr(currSkill->getClass());

	//load
	if(loadCount!=0){
		str+= "\n" + lang.get("Load")+ ": " + intToStr(loadCount) +"  " + loadType->getName();
	}

	//consumable production
	for(int i=0; i<getType()->getCostCount(); ++i){
		const Resource *r= getType()->getCost(i);
		if(r->getType()->getClass()==rcConsumable){
			str+= "\n";
			str+= r->getAmount()<0? lang.get("Produce")+": ": lang.get("Consume")+": ";
			str+= intToStr(abs(r->getAmount())) + " " + r->getType()->getName();
		}
	}

	//command info
	if(!commands.empty()){
		str+= "\n" + commands.front()->getCommandType()->getName();
		if(commands.size()>1){
			str+="\n"+lang.get("OrdersOnQueue")+": "+intToStr(commands.size());
		}
	}
	else{
		//can store
		if(getType()->getStoredResourceCount()>0){
			for(int i=0; i<getType()->getStoredResourceCount(); ++i){
				const Resource *r= getType()->getStoredResource(i);
				str+= "\n"+lang.get("Store")+": ";
				str+= intToStr(r->getAmount()) + " " + r->getType()->getName();
			}
		}
	}

    return str;
}

void Unit::applyUpgrade(const UpgradeType *upgradeType){
	if(upgradeType->isAffected(type)){
		totalUpgrade.sum(upgradeType);
		hp+= upgradeType->getMaxHp();
	}
}

void Unit::computeTotalUpgrade(){
	faction->getUpgradeManager()->computeTotalUpgrade(this, &totalUpgrade);
}

void Unit::incKills(){
	++kills;

	const Level *nextLevel= getNextLevel();
	if(nextLevel!=NULL && kills>= nextLevel->getKills()){
		level= nextLevel;
		int maxHp= totalUpgrade.getMaxHp();
		totalUpgrade.incLevel(type);
		hp+= totalUpgrade.getMaxHp()-maxHp;
	}
}

bool Unit::morph(const MorphCommandType *mct){
	const UnitType *morphUnitType= mct->getMorphUnit();

	if(map->isFreeCellsOrHasUnit(pos, morphUnitType->getSize(), currField, this)){
		map->clearUnitCells(this, pos);
		faction->deApplyStaticCosts(type);
		hp+= morphUnitType->getMaxHp() - type->getMaxHp();
		type= morphUnitType;
		level= NULL;
		computeTotalUpgrade();
		map->putUnitCells(this, pos);
		faction->applyDiscount(morphUnitType, mct->getDiscount());	
		return true;
	}
	else{
		return false;
	}
}


// ==================== PRIVATE ====================

float Unit::computeHeight(const Vec2i &pos) const{
	float height= map->getCell(pos)->getHeight();
	
	if(currField==fAir){
		height+= World::airHeight;
	}

	return height;
}

void Unit::updateTarget(){
	Unit *target= targetRef.getUnit();
	if(target!=NULL){

		//update target pos
		targetPos= target->getCellPos();
		Vec2i relPos= targetPos - pos;
		Vec2f relPosf= Vec2f(relPos.x, relPos.y);
		targetRotation= radToDeg(atan2(relPosf.x, relPosf.y));

		//update target vec
		targetVec= target->getCurrVector();
	}
}

void Unit::clearCommands(){
	while(!commands.empty()){
		undoCommand(commands.back());
		delete commands.back();
		commands.pop_back();
	}
}

CommandResult Unit::checkCommand(Command *command) const{
	
	//if not operative or has not command type => fail
	if(!isOperative() || command->getUnit()==this || !getType()->hasCommandType(command->getCommandType())){
        return crFailUndefined;
	}

	//if pos is not inside the world (if comand has not a pos, pos is (0, 0) and is inside world
	if(!map->isInside(command->getPos())){
        return crFailUndefined;
	}

	//check produced
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		if(!faction->reqsOk(produced)){
            return crFailReqs;
		}
		if(!faction->checkCosts(produced)){
			return crFailRes;
		}
	}

    //build command specific, check resources and requirements for building
    if(command->getCommandType()->getClass()==ccBuild){
		const UnitType *builtUnit= command->getUnitType();
		if(!faction->reqsOk(builtUnit)){
            return crFailReqs;
		}
		if(!faction->checkCosts(builtUnit)){
			return crFailRes;
		}
    }

    //upgrade command specific, check that upgrade is not upgraded
    else if(command->getCommandType()->getClass()==ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());  
		if(faction->getUpgradeManager()->isUpgradingOrUpgraded(uct->getProducedUpgrade())){
            return crFailUndefined;
		}
	}

    return crSuccess; 
}

void Unit::applyCommand(Command *command){

	//check produced
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->applyCosts(produced);
	}

    //build command specific
    if(command->getCommandType()->getClass()==ccBuild){
		faction->applyCosts(command->getUnitType());
    }

    //upgrade command specific
    else if(command->getCommandType()->getClass()==ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());  
		faction->startUpgrade(uct->getProducedUpgrade());
	}
}

CommandResult Unit::undoCommand(Command *command){

	//return cost
	const ProducibleType *produced= command->getCommandType()->getProduced();
	if(produced!=NULL){
		faction->deApplyCosts(produced);
	}

	//return building cost if not already building it or dead 
	if(command->getCommandType()->getClass() == ccBuild){
		if(currSkill->getClass()!=scBuild && currSkill->getClass()!=scDie){
			faction->deApplyCosts(command->getUnitType());
		}
	}

	//upgrade command cancel from list
	if(command->getCommandType()->getClass() == ccUpgrade){
        const UpgradeCommandType *uct= static_cast<const UpgradeCommandType*>(command->getCommandType());
		faction->cancelUpgrade(uct->getProducedUpgrade());
	}

	return crSuccess;
}


}}//end namespace
