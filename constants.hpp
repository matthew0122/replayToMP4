#ifndef TAGPROC_CONSTANTS_H
#define TAGPROC_CONSTANTS_H

const int TILE_SIZE = 40;
const int BALL_RADIUS = 18;
const int WALL_LENGTH = 40;


//Colors
const int WALL_R = 64;
const int WALL_G = 64;
const int WALL_B = 64;
const int FLOOR_R = 164;
const int FLOOR_G = 164;
const int FLOOR_B = 164;
    //Balls
const int RED_R = 255;
const int RED_G = 50;
const int RED_B = 50;
const int BLUE_R = 50;
const int BLUE_G = 50;
const int BLUE_B = 255;


//struct/enum
struct Ball {
    bool red; //true for red, false for blue
    int keys[4] = {0, 0, 0, 0}; //Right, Left, Up, Down
    b2BodyId bodyId;
    b2ShapeId shapeId;
};
enum CollisionCategories
{
    REDBALL = 0x00000002,
    ENVIRONMENT = 0x00000004,
};

//physics constants
const int TPU = 100;
const float ACCELERATION = 1.5f;
const float DRAGCELERATION = 0.5f;
const float MAINTENANCEDRAG = 0.6f;
const float SUPERDRAG = 0.7f;
const int PIXELS_PER_METER = 40;
const float WALL_FRICTION = 0.5f;
const float MAX_SPEED = 2.5f;
const float BOOST_SPEED = 0.3f;


#endif