#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <vector>
#include <iostream>
#include <windows.h>

// Структура для движущейся платформы
struct MovingPlatform {
    sf::RectangleShape shape;
    float speed = 1.5f;
    float minCoord, maxCoord;
    bool isVertical = false;
};

// Структура для врага
struct Enemy {
    sf::RectangleShape shape;
    float speed = 2.f;
    float minX, maxX;
    int platformIndex = -1; // Индекс платформы, на которой сидит враг (-1 если на земле)
    bool onMovingPlatform = false;
};

// Структура для монетки
struct Coin {
    sf::RectangleShape shape;
    int platformIndex = -1; // Индекс платформы (-1 если статичная)
    bool onMovingPlatform = false;
};

int main()
{
    // Устанавливаем кодировку UTF-8 подключение русского шрифта в Windows
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // Создаем окно 1920 на 1080 пикселей
    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "My First Game");
    window.setFramerateLimit(60);

    // Создаем камеру под размер окна 1920 на 1080
    sf::View camera(sf::FloatRect({ 0.f, 0.f }, { 1920.f, 1080.f }));

    // Автоматически загружаем стандартный шрифт из системы Windows
    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        if (!font.openFromFile("C:/Windows/Fonts/calibri.ttf")) {
            std::cout << "Warning: System font not found!\n";
        }
    }

    // --- ЗАГРУЗКА ФОНОВОЙ МУЗЫКИ (ОТКЛЮЧЕНА ПО ЖЕЛАНИЮ) ---
/*  sf::Music bgMusic;
    if (!bgMusic.openFromFile("audio/Fon2.ogg")) {
        std::cout << "Error: could not load audio/Fon2.ogg!\n";
    }
    else {
        bgMusic.setLooping(true);
        bgMusic.setVolume(30.f);
        bgMusic.play();
    }*/

    // Загрузка звука прыжка
    sf::SoundBuffer jumpBuffer;
    if (!jumpBuffer.loadFromFile("audio/jump.ogg")) {
        std::cout << "Error: could not load audio/jump.ogg!\n";
    }
    sf::Sound jumpSound(jumpBuffer);
    jumpSound.setVolume(15.f);

    // Загрузка звука монетки
    sf::SoundBuffer coinBuffer;
    if (!coinBuffer.loadFromFile("audio/coin.wav")) {
        std::cout << "Error: could not load audio/coin.wav!\n";
    }
    sf::Sound coinSound(coinBuffer);
    coinSound.setVolume(20.f);

    // Загрузка звука победы над врагом
    sf::SoundBuffer enemyBuffer;
    if (!enemyBuffer.loadFromFile("audio/enemy.wav")) {
        std::cout << "Error: could not load audio/enemy.wav!\n";
    }
    sf::Sound enemySound(enemyBuffer);
    enemySound.setVolume(20.f);

    // Текст для монеток (Score)
    sf::Text scoreText(font);
    scoreText.setCharacterSize(22);
    scoreText.setFillColor(sf::Color::White);

    // Текст для врагов (Enemies)
    sf::Text enemyText(font);
    enemyText.setCharacterSize(22);
    enemyText.setFillColor(sf::Color::Red);

    // Большая надпись ПОБЕДА (YOU WIN)
    sf::Text winText(font);
    winText.setCharacterSize(64);
    winText.setFillColor(sf::Color::Yellow);
    winText.setStyle(sf::Text::Bold);
    winText.setString("YOU WIN!");

    // Надпись ПРОИГРЫШ (GAME OVER)
    sf::Text gameOverText(font);
    gameOverText.setCharacterSize(64);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setStyle(sf::Text::Bold);
    gameOverText.setString("GAME OVER");

    // --- ПОЛОСКА ЗДОРОВЬЯ (HP BAR) ---
    float maxHealth = 100.f;
    float currentHealth = 100.f;

    sf::RectangleShape healthBarBg(sf::Vector2f(200.f, 20.f));
    healthBarBg.setFillColor(sf::Color(50, 50, 50));
    healthBarBg.setOutlineThickness(2.f);
    healthBarBg.setOutlineColor(sf::Color::White);

    sf::RectangleShape healthBar(sf::Vector2f(200.f, 20.f));
    healthBar.setFillColor(sf::Color::Green);

    sf::Clock damageClock;
    float damageCooldown = 1.5f;

    bool gameOver = false;

    // Создаем игрока (зеленый квадратик)
    sf::RectangleShape player(sf::Vector2f(40.f, 60.f));
    player.setFillColor(sf::Color::Green);
    player.setPosition({ 100.f, 200.f });

    // Физика и движение
    float velocityX = 0.f;
    float velocityY = 0.f;
    const float gravity = 0.5f;
    const float jumpForce = -12.f;
    const float speed = 5.f;

    // --- ПЕРЕМЕННЫЕ ДЛЯ ДВОЙНОГО ПРЫЖКА ---
    int jumpCount = 0;
    const int maxJumps = 2;
    bool jumpKeyReleased = true;

    // Счётчики
    int score = 0;
    int enemiesDefeated = 0;
    bool gameWon = false;

    // --- ПЕРЕМЕННЫЕ ДЛЯ СИСТЕМЫ РЕСПАУНА И БАССЕЙНА ---
    sf::Clock checkpointClock;
    sf::Vector2f lastSafePosition = player.getPosition();

    // Переменные для таймера ожидания в бассейне с шариками
    bool isInBallPit = false;
    sf::Clock ballPitClock;

    // Список статичных платформ (земля, стены и первая платформа)
    std::vector<sf::RectangleShape> staticPlatforms;

    // --- ЗЕМЛЯ (ЛЕВАЯ ЧАСТЬ ДО ЯМЫ) ---
    sf::RectangleShape groundLeft(sf::Vector2f(1900.f, 350.f));
    groundLeft.setFillColor(sf::Color(100, 100, 100));
    groundLeft.setPosition({ 0.f, 650.f });
    staticPlatforms.push_back(groundLeft);

    // --- БАССЕЙН С ШАРИКАМИ В ЯМЕ (ВИЗУАЛ) ---
    sf::RectangleShape ballPit(sf::Vector2f(400.f, 150.f));
    ballPit.setFillColor(sf::Color(255, 182, 193)); // Нежно-розовый цвет
    ballPit.setPosition({ 1900.f, 850.f });

    // --- ПРАВАЯ ЗЕМЛЯ (ПОСЛЕ ЯМЫ) ---
    sf::RectangleShape groundRight(sf::Vector2f(1700.f, 350.f));
    groundRight.setFillColor(sf::Color(100, 100, 100));
    groundRight.setPosition({ 2300.f, 650.f });
    staticPlatforms.push_back(groundRight);

    // Стены (уровень расширен до 4000.f)
    sf::RectangleShape leftWall(sf::Vector2f(20.f, 1000.f));
    leftWall.setFillColor(sf::Color::Transparent);
    leftWall.setPosition({ -20.f, 0.f });
    staticPlatforms.push_back(leftWall);

    sf::RectangleShape rightWall(sf::Vector2f(20.f, 1000.f));
    rightWall.setFillColor(sf::Color::Transparent);
    rightWall.setPosition({ 4000.f, 0.f });
    staticPlatforms.push_back(rightWall);

    // --- ПЕРВАЯ ПЛАТФОРМА (НЕПОДВИЖНАЯ) ---
    sf::RectangleShape plat1(sf::Vector2f(200.f, 20.f));
    plat1.setFillColor(sf::Color(150, 75, 0));
    plat1.setPosition({ 300.f, 440.f });
    staticPlatforms.push_back(plat1);

    // --- ОСТАЛЬНЫЕ ПЛАТФОРМЫ (ДВИЖУЩИЕСЯ) ---
    std::vector<MovingPlatform> movingPlatforms;

    auto addMovingPlatform = [&](float x, float y, float w, float h, float minC, float maxC, float spd, bool vertical) {
        sf::RectangleShape pShape(sf::Vector2f(w, h));
        pShape.setFillColor(sf::Color(150, 75, 0));
        pShape.setPosition({ x, y });
        movingPlatforms.push_back({ pShape, spd, minC, maxC, vertical });
        };

    addMovingPlatform(600.f, 330.f, 250.f, 20.f, 550.f, 650.f, 2.0f, false);

    sf::RectangleShape plat3(sf::Vector2f(180.f, 20.f));
    plat3.setFillColor(sf::Color(150, 75, 0));
    plat3.setPosition({ 950.f, 200.f });
    staticPlatforms.push_back(plat3);

    addMovingPlatform(1300.f, 360.f, 220.f, 20.f, 1200.f, 1550.f, 2.5f, false);

    // --- СПИСОК МОНЕТОК ---
    std::vector<Coin> coins;
    auto addCoin = [&](float x, float y, int pIndex = -1, bool onMoving = false) {
        sf::RectangleShape cShape(sf::Vector2f(20.f, 20.f));
        cShape.setFillColor(sf::Color::Yellow);
        cShape.setPosition({ x, y });
        coins.push_back({ cShape, pIndex, onMoving });
        };

    addCoin(350.f, 400.f);
    addCoin(450.f, 600.f);
    addCoin(650.f, 290.f, 0, true);
    addCoin(1000.f, 175.f);
    addCoin(1350.f, 320.f, 1, true);
    addCoin(2500.f, 600.f);

    // --- СПИСОК ВРАГОВ ---
    std::vector<Enemy> enemies;

    {
        sf::RectangleShape eShape(sf::Vector2f(40.f, 40.f));
        eShape.setFillColor(sf::Color::Red);
        eShape.setPosition({ 500.f, 610.f });
        enemies.push_back({ eShape, 2.f, 400.f, 700.f, -1, false });
    }

    {
        sf::RectangleShape eShape(sf::Vector2f(40.f, 40.f));
        eShape.setFillColor(sf::Color::Red);
        eShape.setPosition({ 650.f, 290.f });
        enemies.push_back({ eShape, 1.5f, 0.f, 0.f, 0, true });
    }

    {
        sf::RectangleShape eShape(sf::Vector2f(40.f, 40.f));
        eShape.setFillColor(sf::Color::Red);
        eShape.setPosition({ 1350.f, 320.f });
        enemies.push_back({ eShape, 1.5f, 0.f, 0.f, 1, true });
    }

    bool isOnGround = false;

    // Главный игровой цикл
    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        if (!gameWon && !gameOver)
        {
            if (damageClock.getElapsedTime().asSeconds() > 0.3f && !isInBallPit)
            {
                velocityX = 0.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
                {
                    velocityX = -speed;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
                {
                    velocityX = speed;
                }
            }

            // --- ОБНОВЛЕНИЕ ДВИЖУЩИХСЯ ПЛАТФОРМ ---
            for (auto& mp : movingPlatforms)
            {
                float oldPlatformY = mp.shape.getPosition().y;
                float oldPlatformX = mp.shape.getPosition().x;

                if (mp.isVertical)
                {
                    mp.shape.move({ 0.f, mp.speed });
                    float currentY = mp.shape.getPosition().y;
                    if (currentY <= mp.minCoord || currentY >= mp.maxCoord)
                    {
                        mp.speed = -mp.speed;
                    }
                }
                else
                {
                    mp.shape.move({ mp.speed, 0.f });
                    float currentX = mp.shape.getPosition().x;
                    if (currentX <= mp.minCoord || currentX >= mp.maxCoord)
                    {
                        mp.speed = -mp.speed;
                    }
                }

                float platformDeltaY = mp.shape.getPosition().y - oldPlatformY;
                float platformDeltaX = mp.shape.getPosition().x - oldPlatformX;

                sf::FloatRect playerBounds = player.getGlobalBounds();
                sf::FloatRect platBounds = mp.shape.getGlobalBounds();

                bool isStandingOnThisPlatform = (playerBounds.position.y + playerBounds.size.y >= platBounds.position.y) &&
                    (playerBounds.position.y + playerBounds.size.y <= platBounds.position.y + 12.f) &&
                    (playerBounds.position.x + playerBounds.size.x > platBounds.position.x) &&
                    (playerBounds.position.x < platBounds.position.x + platBounds.size.x);

                if (isStandingOnThisPlatform && velocityY >= 0.f && !isInBallPit)
                {
                    player.move({ platformDeltaX, platformDeltaY });
                    isOnGround = true;
                    velocityY = 0.f;
                    jumpCount = 0;
                }
            }

            // --- ОБНОВЛЕНИЕ МОНЕТОК ---
            for (auto& coin : coins)
            {
                if (coin.onMovingPlatform && coin.platformIndex >= 0 && coin.platformIndex < movingPlatforms.size())
                {
                    auto& mp = movingPlatforms[coin.platformIndex];
                    sf::FloatRect pBounds = mp.shape.getGlobalBounds();
                    coin.shape.setPosition({ coin.shape.getPosition().x, pBounds.position.y - coin.shape.getSize().y - 5.f });
                    coin.shape.move({ mp.isVertical ? 0.f : mp.speed, 0.f });
                }
            }

            // --- ЛОГИКА ДВИЖЕНИЯ ВРАГОВ ---
            for (auto& enemy : enemies)
            {
                if (enemy.onMovingPlatform && enemy.platformIndex >= 0 && enemy.platformIndex < movingPlatforms.size())
                {
                    auto& mp = movingPlatforms[enemy.platformIndex];
                    sf::FloatRect pBounds = mp.shape.getGlobalBounds();

                    enemy.shape.move({ enemy.speed, 0.f });

                    if (enemy.shape.getPosition().x <= pBounds.position.x)
                    {
                        enemy.shape.setPosition({ pBounds.position.x, enemy.shape.getPosition().y });
                        enemy.speed = -enemy.speed;
                    }
                    else if (enemy.shape.getPosition().x + enemy.shape.getSize().x >= pBounds.position.x + pBounds.size.x)
                    {
                        enemy.shape.setPosition({ pBounds.position.x + pBounds.size.x - enemy.shape.getSize().x, enemy.shape.getPosition().y });
                        enemy.speed = -enemy.speed;
                    }

                    enemy.shape.setPosition({ enemy.shape.getPosition().x, pBounds.position.y - enemy.shape.getSize().y });
                }
                else
                {
                    enemy.shape.move({ enemy.speed, 0.f });
                    float enemyX = enemy.shape.getPosition().x;
                    if (enemyX <= enemy.minX || enemyX >= enemy.maxX)
                    {
                        enemy.speed = -enemy.speed;
                    }
                }
            }

            // --- ЛОГИКА ДВОЙНОГО ПРЫЖКА ---
            bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);

            if (jumpPressed && !isInBallPit)
            {
                if (jumpKeyReleased && jumpCount < maxJumps)
                {
                    velocityY = jumpForce;
                    isOnGround = false;
                    jumpCount++;
                    jumpKeyReleased = false;
                    jumpSound.play();
                }
            }
            else
            {
                jumpKeyReleased = true;
            }

            // --- ДВИЖЕНИЕ И КОЛЛИЗИИ (ЕСЛИ НЕ В БАССЕЙНЕ) ---
            if (!isInBallPit)
            {
                // По оси X
                player.move({ velocityX, 0.f });
                sf::FloatRect playerBounds = player.getGlobalBounds();

                for (const auto& plat : staticPlatforms)
                {
                    sf::FloatRect platBounds = plat.getGlobalBounds();
                    if (auto intersection = playerBounds.findIntersection(platBounds))
                    {
                        if (velocityX > 0.f)
                        {
                            player.setPosition({ platBounds.position.x - playerBounds.size.x, player.getPosition().y });
                        }
                        else if (velocityX < 0.f)
                        {
                            player.setPosition({ platBounds.position.x + platBounds.size.x, player.getPosition().y });
                        }
                        playerBounds = player.getGlobalBounds();
                    }
                }

                for (const auto& mp : movingPlatforms)
                {
                    sf::FloatRect platBounds = mp.shape.getGlobalBounds();
                    if (auto intersection = playerBounds.findIntersection(platBounds))
                    {
                        if (velocityX > 0.f)
                        {
                            player.setPosition({ platBounds.position.x - playerBounds.size.x, player.getPosition().y });
                        }
                        else if (velocityX < 0.f)
                        {
                            player.setPosition({ platBounds.position.x + platBounds.size.x, player.getPosition().y });
                        }
                        playerBounds = player.getGlobalBounds();
                    }
                }

                // По оси Y
                velocityY += gravity;
                player.move({ 0.f, velocityY });
                playerBounds = player.getGlobalBounds();

                for (const auto& plat : staticPlatforms)
                {
                    sf::FloatRect platBounds = plat.getGlobalBounds();
                    if (auto intersection = playerBounds.findIntersection(platBounds))
                    {
                        if (velocityY > 0.f)
                        {
                            player.setPosition({ player.getPosition().x, platBounds.position.y - player.getSize().y });
                            velocityY = 0.f;
                            isOnGround = true;
                            jumpCount = 0;
                        }
                        else if (velocityY < 0.f)
                        {
                            player.setPosition({ player.getPosition().x, platBounds.position.y + platBounds.size.y });
                            velocityY = 0.f;
                        }
                        playerBounds = player.getGlobalBounds();
                    }
                }

                for (const auto& mp : movingPlatforms)
                {
                    sf::FloatRect platBounds = mp.shape.getGlobalBounds();
                    if (auto intersection = playerBounds.findIntersection(platBounds))
                    {
                        if (velocityY > 0.f)
                        {
                            player.setPosition({ player.getPosition().x, platBounds.position.y - player.getSize().y });
                            velocityY = 0.f;
                            isOnGround = true;
                            jumpCount = 0;
                        }
                        else if (velocityY < 0.f)
                        {
                            player.setPosition({ player.getPosition().x, platBounds.position.y + platBounds.size.y });
                            velocityY = 0.f;
                        }
                        playerBounds = player.getGlobalBounds();
                    }
                }
            }

            // --- ЛОГИКА ВРАГОВ ---
            for (auto it = enemies.begin(); it != enemies.end(); )
            {
                sf::FloatRect enemyBounds = it->shape.getGlobalBounds();
                sf::FloatRect playerBounds = player.getGlobalBounds();

                if (playerBounds.findIntersection(enemyBounds))
                {
                    bool hitFromSide = player.getPosition().y + player.getSize().y > enemyBounds.position.y + 15.f;
                    float tolerance = 8.f;

                    bool withinHorizontalBounds = (player.getPosition().x + player.getSize().x > enemyBounds.position.x - tolerance) &&
                        (player.getPosition().x < enemyBounds.position.x + enemyBounds.size.x + tolerance);

                    if (!hitFromSide && velocityY > 0.f && withinHorizontalBounds)
                    {
                        enemiesDefeated++;
                        enemySound.play();
                        std::cout << "Enemy defeated! Total enemies: " << enemiesDefeated << std::endl;

                        velocityY = -8.f;
                        jumpCount = 0;
                        it = enemies.erase(it);
                    }
                    else
                    {
                        if (damageClock.getElapsedTime().asSeconds() > damageCooldown)
                        {
                            currentHealth -= 25.f;
                            damageClock.restart();
                            std::cout << "Ouch! HP left: " << currentHealth << std::endl;

                            float playerCenter = player.getPosition().x + player.getSize().x / 2.f;
                            float enemyCenter = enemyBounds.position.x + enemyBounds.size.x / 2.f;

                            if (playerCenter < enemyCenter) {
                                velocityX = -10.f;
                            }
                            else {
                                velocityX = 8.f;
                            }

                            velocityY = -6.f;
                            isOnGround = false;

                            if (currentHealth <= 0.f)
                            {
                                currentHealth = 0.f;
                                gameOver = true;
                                std::cout << "GAME OVER!" << std::endl;
                            }
                        }
                        ++it;
                    }
                }
                else
                {
                    ++it;
                }
            }

            // Проверка сбора монеток
            for (auto it = coins.begin(); it != coins.end(); )
            {
                if (player.getGlobalBounds().findIntersection(it->shape.getGlobalBounds()))
                {
                    score++;
                    coinSound.play();
                    std::cout << "Coins collected: " << score << std::endl;
                    it = coins.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // --- БАССЕЙН С ШАРИКАМИ С ЗАДЕРЖКОЙ 3 СЕКУНДЫ И ЗВУКОМ ---
            if (player.getPosition().y > 900.f && !isInBallPit)
            {
                if (player.getPosition().x >= 1900.f && player.getPosition().x <= 2300.f)
                {
                    isInBallPit = true;
                    ballPitClock.restart();
                    velocityY = 0.f;
                    velocityX = 0.f;

                    // Звук монетки при падении в шарики (плюх)
                    coinSound.play();
                    std::cout << "Плюх в шарики! Купаемся 3 секунды...\n";
                }
                else
                {
                    player.setPosition(lastSafePosition);
                    player.setPosition({ player.getPosition().x, player.getPosition().y - 20.f });
                    velocityY = 0.f;
                    velocityX = 0.f;
                    jumpCount = 0;
                }
            }

            // Ожидание 3 секунды в бассейне
            if (isInBallPit)
            {
                velocityX = 0.f;
                velocityY = 0.f;

                if (ballPitClock.getElapsedTime().asSeconds() >= 3.0f)
                {
                    player.setPosition({ 1850.f, 550.f });
                    isInBallPit = false;
                    jumpCount = 0;
                    std::cout << "Время вышло! Возвращаемся наверх.\n";
                }
            }

            // Проверка условия победы
            if (coins.empty() && enemies.empty())
            {
                gameWon = true;
                std::cout << "YOU WIN!" << std::endl;
            }
        }

        // Сохранение безопасной позиции
        if (checkpointClock.getElapsedTime().asSeconds() > 2.0f)
        {
            if (isOnGround && !isInBallPit)
            {
                lastSafePosition = player.getPosition();
            }
            checkpointClock.restart();
        }

        // --- УПРАВЛЕНИЕ КАМЕРОЙ ---
        float cameraX = player.getPosition().x;
        if (cameraX < 960.f) cameraX = 960.f;

        float maxCameraX = 4000.f - 960.f;
        if (cameraX > maxCameraX) cameraX = maxCameraX;

        camera.setCenter({ cameraX, 430.f });
        window.setView(camera);

        // Обновление интерфейса
        scoreText.setString("Score: " + std::to_string(score));
        scoreText.setPosition({ camera.getCenter().x - 930.f, camera.getCenter().y - 510.f });

        enemyText.setString("Enemies: " + std::to_string(enemiesDefeated));
        enemyText.setPosition({ camera.getCenter().x - 930.f, camera.getCenter().y - 470.f });

        float healthPercent = currentHealth / maxHealth;
        healthBar.setSize(sf::Vector2f(200.f * healthPercent, 20.f));

        healthBarBg.setPosition({ camera.getCenter().x + 700.f, camera.getCenter().y - 510.f });
        healthBar.setPosition({ camera.getCenter().x + 700.f, camera.getCenter().y - 510.f });

        winText.setPosition({ camera.getCenter().x - 150.f, camera.getCenter().y - 40.f });
        gameOverText.setPosition({ camera.getCenter().x - 200.f, camera.getCenter().y - 40.f });

        // --- ОТРИСОВКА ---
        window.clear(sf::Color::Blue);

        for (const auto& plat : staticPlatforms)
        {
            window.draw(plat);
        }

        for (const auto& mp : movingPlatforms)
        {
            window.draw(mp.shape);
        }

        for (const auto& coin : coins)
        {
            window.draw(coin.shape);
        }

        for (const auto& enemy : enemies)
        {
            window.draw(enemy.shape);
        }

        window.draw(ballPit); // Рисуем розовый бассейн с шариками
        window.draw(player);

        window.draw(scoreText);
        window.draw(enemyText);
        window.draw(healthBarBg);
        window.draw(healthBar);

        if (gameWon)
        {
            window.draw(winText);
        }

        if (gameOver)
        {
            window.draw(gameOverText);
        }

        window.display();
    }

    return 0;
} //конец main