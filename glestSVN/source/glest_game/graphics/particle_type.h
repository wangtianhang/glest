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

#ifndef _GLEST_GAME_PARTICLETYPE_H_
#define _GLEST_GAME_PARTICLETYPE_H_

#include <string>

#include "particle.h"
#include "factory.h"
#include "texture.h"
#include "vec.h"
#include "xml_parser.h"

using std::string;

namespace Glest{ namespace Game{

using Shared::Graphics::ParticleSystem;
using Shared::Graphics::AttackParticleSystem;
using Shared::Graphics::ProjectileParticleSystem;
using Shared::Graphics::SplashParticleSystem;
using Shared::Graphics::Texture2D;
using Shared::Graphics::Vec3f;
using Shared::Graphics::Vec4f;
using Shared::Graphics::Model;
using Shared::Util::MultiFactory;
using Shared::Xml::XmlNode;

// ===========================================================
//	class ParticleSystemType 
//
///	A type of particle system
// ===========================================================

class ParticleSystemType{
protected:
	string type;
	Texture2D *texture;
	Model *model;
	string primitive;
	Vec3f offset;
	Vec4f color;
	Vec4f colorNoEnergy;
	float size;
	float sizeNoEnergy;
	float speed;
	float gravity;
	int emissionRate;
	int energyMax;
	int energyVar;

public:
	ParticleSystemType();
	void load(const XmlNode *particleSystemNode, const string &dir);

protected:
	void setValues(AttackParticleSystem *ats);
};

// ===========================================================
//	class ParticleSystemTypeProjectile
// ===========================================================

class ParticleSystemTypeProjectile: public ParticleSystemType{
private:
	string trajectory;
	float trajectorySpeed;
	float trajectoryScale;
	float trajectoryFrequency;

public:
	void load(const string &dir, const string &path);
	ProjectileParticleSystem *create();
};

// ===========================================================
//	class ParticleSystemTypeSplash
// ===========================================================

class ParticleSystemTypeSplash: public ParticleSystemType{
public:
	void load(const string &dir, const string &path);
	SplashParticleSystem *create();

private:
	int emissionRateFade;
	float verticalSpreadA;
	float verticalSpreadB;
	float horizontalSpreadA;
	float horizontalSpreadB;
};

}}//end namespace

#endif
