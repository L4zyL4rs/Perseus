#pragma once
#include "AssetManager.h"

class SceneLoader {
	AssetManager* assetManager{};
	SceneLoader(AssetManager* aM) : assetManager(aM) {};
	// Dummy loader for now
	void load(std::string scene) {
		std::string cube = "cube";
		MeshHandle cubeMesh = assetManager->loadAsset(cube);

		assetManager->createBuffers();
	}
};