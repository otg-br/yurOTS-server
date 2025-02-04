/*
 * YurOTS, a free game server emulator 
 * Official Repository on Github <https://github.com/rodolfoaugusto/yurOTS-server>
 * Copyright (C) 2020 - Rodi <https://github.com/rodolfoaugusto>
 * A fork of The Forgotten Server(Mark Samman) branch 1.2 and part of Nostalrius(Alejandro Mujica) repositories.
 *
 * The MIT License (MIT). Copyright © 2020 <YurOTS>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

#include "otpch.h"

#include "baseevents.h"

#include "pugicast.h"
#include "tools.h"

extern LuaEnvironment g_luaEnvironment;

bool BaseEvents::loadFromXml()
{
	if (loaded) {
		std::cout << "[Error - BaseEvents::loadFromXml] It's already loaded." << std::endl;
		return false;
	}

	std::string scriptsName = getScriptBaseName();
	std::string basePath = "data/" + scriptsName + "/";
	if (getScriptInterface().loadFile(basePath + "lib/" + scriptsName + ".lua") == -1) {
		std::cout << "[Warning - BaseEvents::loadFromXml] Can not load " << scriptsName << " lib/" << scriptsName << ".lua" << std::endl;
	}

	std::string filename = basePath + scriptsName + ".xml";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	if (!result) {
		printXMLError("Error - BaseEvents::loadFromXml", filename, result);
		return false;
	}

	loaded = true;

	for (auto node : doc.child(scriptsName.c_str()).children()) {
		Event* event = getEvent(node.name());
		if (!event) {
			continue;
		}

		if (!event->configureEvent(node)) {
			std::cout << "[Warning - BaseEvents::loadFromXml] Failed to configure event" << std::endl;
			delete event;
			continue;
		}

		bool success;

		pugi::xml_attribute scriptAttribute = node.attribute("script");
		if (scriptAttribute) {
			std::string scriptFile = "scripts/" + std::string(scriptAttribute.as_string());
			success = event->checkScript(basePath, scriptsName, scriptFile) && event->loadScript(basePath + scriptFile);
		} else {
			success = event->loadFunction(node.attribute("function"));
		}

		if (!success || !registerEvent(event, node)) {
			delete event;
		}
	}
	return true;
}

bool BaseEvents::reload()
{
	loaded = false;
	clear();
	return loadFromXml();
}

Event::Event(LuaScriptInterface* interface) : scriptInterface(interface) {}

Event::Event(const Event* copy) :
	scripted(copy->scripted), scriptId(copy->scriptId), scriptInterface(copy->scriptInterface) {}

bool Event::checkScript(const std::string& basePath, const std::string& scriptsName, const std::string& scriptFile) const
{
	LuaScriptInterface* testInterface = g_luaEnvironment.getTestInterface();
	testInterface->reInitState();

	if (testInterface->loadFile(std::string(basePath + "lib/" + scriptsName + ".lua")) == -1) {
		std::cout << "[Warning - Event::checkScript] Can not load " << scriptsName << " lib/" << scriptsName << ".lua" << std::endl;
	}

	if (scriptId != 0) {
		std::cout << "[Failure - Event::checkScript] scriptid = " << scriptId << std::endl;
		return false;
	}

	if (testInterface->loadFile(basePath + scriptFile) == -1) {
		std::cout << "[Warning - Event::checkScript] Can not load script: " << scriptFile << std::endl;
		std::cout << testInterface->getLastLuaError() << std::endl;
		return false;
	}

	int32_t id = testInterface->getEvent(getScriptEventName());
	if (id == -1) {
		std::cout << "[Warning - Event::checkScript] Event " << getScriptEventName() << " not found. " << scriptFile << std::endl;
		return false;
	}
	return true;
}

bool Event::loadScript(const std::string& scriptFile)
{
	if (!scriptInterface || scriptId != 0) {
		std::cout << "Failure: [Event::loadScript] scriptInterface == nullptr. scriptid = " << scriptId << std::endl;
		return false;
	}

	if (scriptInterface->loadFile(scriptFile) == -1) {
		std::cout << "[Warning - Event::loadScript] Can not load script. " << scriptFile << std::endl;
		std::cout << scriptInterface->getLastLuaError() << std::endl;
		return false;
	}

	int32_t id = scriptInterface->getEvent(getScriptEventName());
	if (id == -1) {
		std::cout << "[Warning - Event::loadScript] Event " << getScriptEventName() << " not found. " << scriptFile << std::endl;
		return false;
	}

	scripted = true;
	scriptId = id;
	return true;
}

bool CallBack::loadCallBack(LuaScriptInterface* interface, const std::string& name)
{
	if (!interface) {
		std::cout << "Failure: [CallBack::loadCallBack] scriptInterface == nullptr" << std::endl;
		return false;
	}

	scriptInterface = interface;

	int32_t id = scriptInterface->getEvent(name.c_str());
	if (id == -1) {
		std::cout << "[Warning - CallBack::loadCallBack] Event " << name << " not found." << std::endl;
		return false;
	}

	callbackName = name;
	scriptId = id;
	loaded = true;
	return true;
}
