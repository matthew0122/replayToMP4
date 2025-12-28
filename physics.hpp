#include <box2d/box2d.h>
#include "constants.hpp"
#include <vector>
#include <string>
#include <assert.h>

Ball createBall(b2WorldId worldId, float x, float y, float radius, bool color);

void createWall(b2WorldId worldId, float x, float y, float length);

void initializeWalls(b2WorldId worldId, std::vector<std::vector<std::string>> map);

void updateVelocity(Ball player, float timeStep);

