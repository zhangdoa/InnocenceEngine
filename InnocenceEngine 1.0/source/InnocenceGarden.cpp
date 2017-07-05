#include "stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}


InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::init()
{
	testTriangle.exec(INIT);
}

void InnocenceGarden::update()
{
	testTriangle.exec(UPDATE);
}

void InnocenceGarden::shutdown()
{	
	testTriangle.exec(SHUTDOWN);
}
