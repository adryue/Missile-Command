#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>

const int WIN_X_LEN = 1280;
const int WIN_Y_LEN = 720;

const int FRAMERATE = 60;

const int TEXT_SIZE = 40;

const float SCORE_MULTIPLIER = 0.1;

const int RELOAD_DURATION_FRAMES = 30;
const float RELOAD_BAR_SIZE = 100.0;
const float RELOAD_TICK_SIZE = RELOAD_BAR_SIZE / RELOAD_DURATION_FRAMES;
const sf::Color RELOADING_COLOR = sf::Color::Yellow;

const int BULLET_RADIUS = 3.0;
const float BULLET_RELATIVE_SPEED = 9.0;
const unsigned int BULLET_LIMIT = 2;

const int EXPLOSION_TICK_COUNT = 60;

const float MISSILE_RADIUS = 5.0;
const int MISSILE_INIT_COUNT = 1;
const int MISSILE_DURATION_MIN_FRAMES = 400;
const int MISSILE_DURATION_VARIATION_FRAMES = 120;
const int MISSILE_RANDOM_SPAWN_CHANCE = 100;

const sf::Vector2f BUILDING_SIZE(100, 25);
const int BUILDING_COUNT = 6;

sf::Font font;

class Bullet: public sf::CircleShape
{
public:
    sf::Vector2f velocity;
    float travelDistance;
    float distanceTravelled;
    float tickDistance;

    Bullet(sf::Vector2f pos, sf::Vector2f vel, float travelDis)
    {
        setRadius(BULLET_RADIUS);
        setFillColor(sf::Color::White);
        setOrigin(BULLET_RADIUS, BULLET_RADIUS);
        travelDistance = std::sqrt(travelDis);
        velocity = vel;
        tickDistance = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
        setPosition(pos);
        distanceTravelled = 0.0;
    }
};
class Explosion: public sf::CircleShape
{
public:
    sf::Color explodeColor[3];
    int explodeColorKey;
    int explodeTick;
    bool fromBullet;

    Explosion(sf::Vector2f pos, bool fromBul)
    {
        setRadius(BULLET_RADIUS * 1.5);
        setOrigin(BULLET_RADIUS * 1.5, BULLET_RADIUS * 1.5);
        setPosition(pos);
        setFillColor(sf::Color::Red);
        explodeColor[0] = sf::Color::Red;
        explodeColor[1] = sf::Color::Yellow;
        explodeColor[2] = sf::Color(255, 165, 0); // orange
        explodeColorKey = 0;
        explodeTick = 0;
        fromBullet = fromBul;
    }
    void Expand()
    {
        setRadius(getRadius() + 1.0);
        setOrigin(getRadius(), getRadius());
    }
    void ChangeColor()
    {
        explodeColorKey++;
        setFillColor(explodeColor[explodeColorKey % 3]);
    }
};
class Missile: public sf::CircleShape
{
public:
    sf::Vector2f velocity;
    sf::VertexArray trail;
    int BuildingIndex;

    Missile(sf::Vector2f vel, sf::Vector2f pos, int BuildingNum)
    {
        setRadius(MISSILE_RADIUS);
        setOrigin(MISSILE_RADIUS, MISSILE_RADIUS);
        setFillColor(sf::Color::Yellow);
        setPosition(pos);
        velocity = vel;
        trail.setPrimitiveType(sf::LineStrip);
        trail.append(sf::Vertex(pos));
        trail.append(sf::Vertex(pos));
        BuildingIndex = BuildingNum;
    }
};
class Building: public sf::RectangleShape
{
public:
    bool destroyed;
    Building(sf::Vector2f pos)
    {
        setSize(BUILDING_SIZE);
        setFillColor(sf::Color(100, 100, 100));
        setOutlineThickness(-3.0);
        setOutlineColor(sf::Color::White);
        setOrigin(BUILDING_SIZE.x / 2, 0);
        setPosition(pos);
        destroyed = false;
    }
};
class ScoreText: public sf::Text
{
public:
    int scoreValue;
    sf::Vector2f position;
    ScoreText(sf::Vector2f pos)
    {
        font.loadFromFile("Hyperspace-JvEM.ttf");
        setFont(font);
        setCharacterSize(TEXT_SIZE);
        setFillColor(sf::Color::White);
        position = pos;
        scoreValue = 0;
        setValue(0);
    }
    void addValue(int value)
    {
        setValue(scoreValue + value);
        return;
    }
    void setValue(int value)
    {
        scoreValue = value;
        setString(std::to_string(scoreValue));
        setOrigin(getLocalBounds().width + getLocalBounds().left, 0.0);
        setPosition(position);
        return;
    }
};
bool CheckCollision(Explosion explosion, sf::Vector2f position)
{
    float distance = ((explosion.getPosition().x - position.x) * (explosion.getPosition().x - position.x)) + ((explosion.getPosition().y - position.y) * (explosion.getPosition().y - position.y));
    if (distance <= explosion.getRadius() * explosion.getRadius())
    {
        return true;
    }
    return false;
}
int main()
{
    // Create the main window

    sf::RenderWindow window(sf::VideoMode(WIN_X_LEN, WIN_Y_LEN), "Missile Command");
    window.setFramerateLimit(FRAMERATE);

    std::srand(std::time(nullptr));

    //play a sound when a building is destroyed
    sf::SoundBuffer buildingDestroySoundBuffer;
    buildingDestroySoundBuffer.loadFromFile("buildingDestroy.wav");
    sf::Sound buildingDestroySound;
    buildingDestroySound.setBuffer(buildingDestroySoundBuffer);
    //play a sound when the turret shoots
    sf::SoundBuffer turretFireSoundBuffer;
    turretFireSoundBuffer.loadFromFile("turretFire.wav");
    sf::Sound turretFireSound;
    turretFireSound.setBuffer(turretFireSoundBuffer);
    //play a sound when a bullet explodes
    sf::SoundBuffer missileDestroySoundBuffer;
    missileDestroySoundBuffer.loadFromFile("missileDestroy.wav");
    sf::Sound missileDestroySound;
    missileDestroySound.setBuffer(missileDestroySoundBuffer);


    font.loadFromFile("Hyperspace-JvEM.ttf");

    //set up the lose screen text
    sf::Text loseScreen;
    loseScreen.setFont(font);
    loseScreen.setCharacterSize(150);
    loseScreen.setFillColor(sf::Color::White);
    loseScreen.setString("GAME OVER");
    loseScreen.setOrigin(loseScreen.getLocalBounds().width / 2.0 + loseScreen.getLocalBounds().left, loseScreen.getLocalBounds().height / 2.0 + loseScreen.getLocalBounds().top);
    loseScreen.setPosition((float)WIN_X_LEN / 2, (float)WIN_Y_LEN / 2);
    bool gameLost = false;

    ScoreText score(sf::Vector2f(WIN_X_LEN - 6.0, 0.0));

    //custom cursors
    sf::Cursor normalCursor;
    normalCursor.loadFromSystem(sf::Cursor::Cross);
    sf::Cursor noFireCursor;
    noFireCursor.loadFromSystem(sf::Cursor::NotAllowed);
    window.setMouseCursor(normalCursor);

    //the turret
    sf::RectangleShape rectangle;
    rectangle.setSize(sf::Vector2f(100.0, 10));
    rectangle.setOrigin(10.0, 5.0);
    rectangle.setPosition(WIN_X_LEN / 2, WIN_Y_LEN);
    rectangle.setFillColor(sf::Color(50, 50, 50));
    rectangle.setOutlineThickness(2.0);
    rectangle.setOutlineColor(sf::Color::White);
    bool fireKeyPressed = false;

    //reload things
    int reloadTimeRemaining = 0;
    sf::RectangleShape reloadBar;
    reloadBar.setSize(sf::Vector2f(100.0, 10));
    reloadBar.setOrigin(10.0, 5.0);
    reloadBar.setPosition(WIN_X_LEN / 2, WIN_Y_LEN);
    reloadBar.setFillColor(RELOADING_COLOR);

    //the "no fire" zone
    sf::RectangleShape noFireZone;
    noFireZone.setSize(sf::Vector2f(WIN_X_LEN, 120));
    noFireZone.setFillColor(sf::Color(0, 100, 0));
    noFireZone.setPosition(0.0, WIN_Y_LEN - noFireZone.getSize().y);

    std::vector<Bullet> bullets;
    std::vector<Explosion> explosions;
    std::vector<Missile> missiles;
    std::vector<Building> buildings;
    buildings.push_back(Building(sf::Vector2f(WIN_X_LEN / 8, WIN_Y_LEN - BUILDING_SIZE.y)));
    buildings.push_back(Building(sf::Vector2f(WIN_X_LEN / 8 * 2, WIN_Y_LEN - BUILDING_SIZE.y)));
    buildings.push_back(Building(sf::Vector2f(WIN_X_LEN / 8 * 3, WIN_Y_LEN - BUILDING_SIZE.y)));
    buildings.push_back(Building(sf::Vector2f(WIN_X_LEN / 8 * 5, WIN_Y_LEN - BUILDING_SIZE.y)));
    buildings.push_back(Building(sf::Vector2f(WIN_X_LEN / 8 * 6, WIN_Y_LEN - BUILDING_SIZE.y)));
    buildings.push_back(Building(sf::Vector2f(WIN_X_LEN / 8 * 7, WIN_Y_LEN - BUILDING_SIZE.y)));

    for (int i = 0; i < MISSILE_INIT_COUNT; i++)
    {
        int SkyXCoord = std::rand() % WIN_X_LEN;
        int BuildingNum = std::rand() % BUILDING_COUNT;
        int GroundXCoord = buildings[BuildingNum].getPosition().x;
        sf::Vector2f velocity;
        int duration = std::rand() % MISSILE_DURATION_VARIATION_FRAMES + MISSILE_DURATION_MIN_FRAMES;
        velocity.x = ((float)(GroundXCoord - SkyXCoord)) / duration;
        velocity.y = ((float)(WIN_Y_LEN - BUILDING_SIZE.y)) / duration;
        missiles.push_back(Missile(velocity, sf::Vector2f(SkyXCoord, 0.0), BuildingNum));
    }

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }
        window.clear();
        //handle spawning missiles
        if (std::rand() % MISSILE_RANDOM_SPAWN_CHANCE == 0)
        {
            int SkyXCoord = std::rand() % WIN_X_LEN;
            int BuildingNum = std::rand() % BUILDING_COUNT;
            if (!gameLost)
            {
                while (buildings[BuildingNum].destroyed)
                {
                    BuildingNum++;
                    BuildingNum %= BUILDING_COUNT;
                }
            }
            int GroundXCoord = buildings[BuildingNum].getPosition().x;
            sf::Vector2f velocity;
            int duration = std::rand() % MISSILE_DURATION_VARIATION_FRAMES + MISSILE_DURATION_MIN_FRAMES;
            velocity.x = ((float)(GroundXCoord - SkyXCoord)) / duration;
            velocity.y = ((float)(WIN_Y_LEN - BUILDING_SIZE.y)) / duration;
            missiles.push_back(Missile(velocity, sf::Vector2f(SkyXCoord, 0.0), BuildingNum));
        }
        //handle the mouse
        sf::Vector2i mousePosition(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
        if (mousePosition.y > noFireZone.getPosition().y)
        {
            window.setMouseCursor(noFireCursor);
            mousePosition.y = noFireZone.getPosition().y;
        }
        else
        {
            window.setMouseCursor(normalCursor);
        }
        float yDifference = mousePosition.y - rectangle.getPosition().y;
        float xDifference = mousePosition.x - rectangle.getPosition().x;
        float degree =  std::atan(yDifference / xDifference) / (2.0 * M_PI) * 360.0;
        if (xDifference < 0.0)
        {
            degree += 180;
        }
        if (!gameLost)
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                if (!fireKeyPressed)
                {
                    fireKeyPressed = true;
                    if (reloadTimeRemaining == 0)
                    {
                        turretFireSound.play();
                        reloadTimeRemaining = RELOAD_DURATION_FRAMES;
                        reloadBar.setSize(sf::Vector2f(0.0, reloadBar.getSize().y));
                        float bulletXLen = rectangle.getPosition().x - mousePosition.x;
                        float bulletYLen = rectangle.getPosition().y - mousePosition.y;
                        float bulletTravelDis = bulletXLen * bulletXLen + bulletYLen * bulletYLen;
                        bullets.push_back(Bullet(rectangle.getPosition(), sf::Vector2f(BULLET_RELATIVE_SPEED * cos(rectangle.getRotation() / 360 * 2 * M_PI), BULLET_RELATIVE_SPEED * sin(rectangle.getRotation() / 360 * 2 * M_PI)), bulletTravelDis));
                    }
                }
            }
            else
            {
                fireKeyPressed = false;
            }
            if (reloadTimeRemaining > 0)
            {
                reloadTimeRemaining--;
                reloadBar.setSize(sf::Vector2f(reloadBar.getSize().x + RELOAD_TICK_SIZE, reloadBar.getSize().y));
            }
        }
        rectangle.setRotation(degree);
        reloadBar.setRotation(degree);


        // handle bullets
        for (unsigned int b = 0; b < bullets.size(); )
        {
            bullets[b].distanceTravelled += bullets[b].tickDistance;
            if (bullets[b].distanceTravelled > bullets[b].travelDistance)
            {
                explosions.push_back(Explosion(bullets[b].getPosition(), true));
                bullets.erase(bullets.begin() + b);
            }
            else
            {
                bullets[b].move(bullets[b].velocity);
                b++;
            }
        }


        //handle missiles
        for (unsigned int m = 0; m < missiles.size(); )
        {
            if (missiles[m].getPosition().y >= WIN_Y_LEN - BUILDING_SIZE.y)
            {
                buildingDestroySound.play();
                explosions.push_back(Explosion(missiles[m].getPosition(), false));
                buildings[missiles[m].BuildingIndex].destroyed = true;
                bool allBuildingsDestroyed = true;
                for (unsigned int b = 0; b < buildings.size(); b++)
                {
                    if (!buildings[b].destroyed)
                    {
                        allBuildingsDestroyed = false;
                        break;
                    }
                }
                gameLost = allBuildingsDestroyed;
                missiles.erase(missiles.begin() + m);
            }
            else
            {
                missiles[m].move(missiles[m].velocity);
                missiles[m].trail[1].position = missiles[m].getPosition();
                m++;
            }
        }


        //handle explosions
        for (unsigned int e = 0; e < explosions.size(); )
        {
            if (explosions[e].explodeTick > EXPLOSION_TICK_COUNT)
            {
                explosions.erase(explosions.begin() + e);
            }
            else
            {
                explosions[e].ChangeColor();
                explosions[e].Expand();
                explosions[e].explodeTick++;
                for (unsigned int m = 0; m < missiles.size(); )
                {
                    if (CheckCollision(explosions[e], missiles[m].getPosition()))
                    {
                        missileDestroySound.play();
                        if (explosions[e].fromBullet)
                        {
                            score.addValue((WIN_Y_LEN - missiles[m].getPosition().y) * SCORE_MULTIPLIER);
                        }
                        missiles.erase(missiles.begin() + m);
                    }
                    else
                    {
                        m++;
                    }
                }
                e++;
            }
        }


        //draw everything
        window.draw(noFireZone);
        for (Bullet& b : bullets)
        {
            window.draw(b);
        }
        for (Missile& m : missiles)
        {
            window.draw(m);
            window.draw(m.trail);
        }
        for (Building& b : buildings)
        {
            if (!b.destroyed)
            {
                window.draw(b);
            }
        }
        for (Explosion& e : explosions)
        {
            window.draw(e);
        }
        window.draw(rectangle);
        window.draw(reloadBar);
        window.draw(score);
        if (gameLost)
        {
            window.draw(loseScreen);
        }

        window.display();
    }

    return EXIT_SUCCESS;
}
