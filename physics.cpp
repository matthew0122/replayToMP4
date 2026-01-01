#include <box2d/box2d.h>
#include "constants.hpp"
#include <vector>
#include <string>
#include <assert.h>
#include <iostream>

Ball createBall(b2WorldId worldId, float x, float y, float radius, bool color){
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){x,y};
    bodyDef.linearDamping = 0.5f;
    bodyDef.angularDamping = 0.5f;
    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);

    b2Circle circle;
    circle.center = (b2Vec2){0,0}; //rel to body?
    circle.radius = radius;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = false;
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.5f;
    shapeDef.filter.categoryBits = REDBALL; //I am redball
    shapeDef.filter.maskBits = ENVIRONMENT; // I collide with
    
    b2ShapeId shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);
    b2Shape_SetRestitution(shapeId, 0.2f);


    Ball player = {true,{false, false, false, false}, bodyId, shapeId};

    return player;
}

void createWall(b2WorldId worldId, float x, float y, float length){
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = (b2Vec2){x,y};
    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = false;
    shapeDef.filter.categoryBits = ENVIRONMENT;

    b2Polygon square = b2MakeSquare(length/2);

    b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &square);
    assert(b2Shape_IsValid(shapeId));
}

void initializeWalls(b2WorldId worldId, std::vector<std::vector<std::string>> map){
    for(int i = 0; i < map.size(); i++){
        for(int j = 0; j < map[i].size(); j++){
            // j = x (column), i = y (row)
            if (map[i][j] == "1"){
                createWall(worldId, (j+1) * WALL_LENGTH, (i+1) * WALL_LENGTH, WALL_LENGTH);
            }
        }
    }
}

void updateVelocity(Ball player, float timeStep){
    b2Vec2 velocity = b2Body_GetLinearVelocity(player.bodyId);
    if (player.keys[0] > 0 && player.keys[0] > player.keys[1]){ //right
        velocity.x += ACCELERATION * TPU * timeStep;
    }
    if (player.keys[1] > 0 && player.keys[1] > player.keys[0]){ //left
        velocity.x -= ACCELERATION * TPU * timeStep;
    }
    if (player.keys[2] > 0 && player.keys[2] > player.keys[3]){ //up
        velocity.y -= ACCELERATION * TPU * timeStep;
    }
    if (player.keys[3] > 0 && player.keys[3] > player.keys[2]){ //down
        velocity.y += ACCELERATION * TPU * timeStep;
    }
    //apply drag
    // velocity.x -= velocity.x * DRAGCELERATION * timeStep;
    // velocity.y -= velocity.y * DRAGCELERATION * timeStep;
    //stop in place
    if (abs(velocity.x) < 0.3f){
        velocity.x = 0;
    }
    if (abs(velocity.y) < 0.3f){
        velocity.y = 0;
    }
    //clamp to max speed
    if (velocity.x > MAX_SPEED * TPU){
        velocity.x = MAX_SPEED * TPU;
    }
    else if (velocity.x < -MAX_SPEED * TPU){
        velocity.x = -MAX_SPEED * TPU;
    }
    if (velocity.y > MAX_SPEED * TPU){
        velocity.y = MAX_SPEED * TPU;
    }
    else if (velocity.y < -MAX_SPEED * TPU){
        velocity.y = -MAX_SPEED * TPU;
    }
    // std::cout << velocity.x << ", " << velocity.y << std::endl;
    b2Body_SetLinearVelocity(player.bodyId, velocity);
}
