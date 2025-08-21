#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <raylib.h>

int nameInputMode = 0;
int currentNameInput = 0;
char nameInputBuffer[21] = "";

typedef struct {
    int value;  
    int suit;   
} Card;

typedef struct {
    char name[50];
    Card hand[2];
    int chips;
    int bet;
    int folded;
    int handRank;      
    int handHighCard;  
    char handName[50]; 
} Player;

typedef struct {
    Card cards[52];
    int top;
} Deck;

typedef struct {
    Card cards[5];
    int count;
} Community;

typedef struct {
    char title[100];
    char message[200];
    int active;
    int x;
    int y;
    int width;
    int height;
    float timeRemaining;
} AdPopup;

typedef struct {
    char name[50];
    int gamesPlayed;
    int gamesWon;
    int totalEarnings;
    int biggestPot;
    char bestHand[50];
} PlayerProfile;

const char *suits[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
const char *values[] = {"", "", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};
const char *handRankNames[] = {"", "High Card", "One Pair", "Two Pair", "Three of a Kind", 
                              "Straight", "Flush", "Full House", "Four of a Kind", 
                              "Straight Flush", "Royal Flush"};

int raiseInputMode = 0;
char raiseInputBuffer[10] = "";
int raiseAmount = 0;

Sound cardDealSound;
Sound betSound;
Sound foldSound;
Sound winSound;
Sound adPopupSound;
Music backgroundMusic;

void initDeck(Deck *deck);
void shuffleDeck(Deck *deck);
Card dealCard(Deck *deck);
void dealHoleCards(Player *players, int numPlayers, Deck *deck);
void dealCommunityCards(Community *community, Deck *deck, int count);
void displayGameInfo(Player *players, int numPlayers, Community *community, int currentPlayer, int pot);
void drawCard(Card card, int x, int y, int size);
void evaluateAllHands(Player *players, int numPlayers, Community *community);
int compareHands(Player *players, int numPlayers, Community *community);
void handleRaiseInput(Player *players, int *currentPlayer, int *pot, int *highestBet, int *roundStartPlayer);
int evaluateHand(Card *hand, int handSize, int *highCard, char *handName);
void sortCards(Card *cards, int count);
int countValues(Card *cards, int count, int value);
int countSuits(Card *cards, int count, int suit);
int checkStraight(Card *cards, int count, int *highCard);
int checkFlush(Card *cards, int count, int *suit);
int checkFullHouse(Card *cards, int count, int *threeValue, int *pairValue);
int checkTwoPair(Card *cards, int count, int *highPair, int *lowPair);
void triggerAdPopup(AdPopup *ad);
void drawAdPopup(AdPopup *ad);
bool checkCloseButtonPressed(AdPopup *ad);
void loadGameSounds();
void unloadGameSounds();
void drawLoadingScreen(float timeElapsed);
void savePlayerProfile(Player player, int wonGame, int potSize);
void displayPlayerStats(Player player);
void saveGameConfig(int numPlayers, int startingChips);
void loadGameConfig(int *numPlayers, int *startingChips);
void handleNameInput(Player *players, int numPlayers, int *gameState);
int winner = -1;
int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);  
    srand(time(NULL));
    InitWindow(1280, 720, "The Winning Hand - Poker Game");
    
    SetTargetFPS(60);

    
    InitAudioDevice();
    loadGameSounds();

    
    Player players[10];
    int numPlayers = 4;
    Deck deck;
    Community community;
    int pot = 0;
    int currentPlayer = 0;
    int gameState = 0;  
    int roundOver = 0;
    int highestBet = 0;
    int roundStartPlayer = 0;
    float gameTimer = 0.0f;
    int flopAdShown = 0;
    int turnAdShown = 0;
    int riverAdShown = 0;
    float loadingTimer = 0.0f;
    
    AdPopup adPopup = {0};
    
    PlayMusicStream(backgroundMusic);
    SetMusicVolume(backgroundMusic, 0.5f);
    
    for (int i = 0; i < numPlayers; i++) {
        sprintf(players[i].name, "Player %d", i+1);
        players[i].chips = 1000;
        players[i].bet = 0;
        players[i].folded = 0;
        strcpy(players[i].handName, "");
    }
    
    initDeck(&deck);
    shuffleDeck(&deck);
    community.count = 0;
    dealHoleCards(players, numPlayers, &deck);
    
    while (!WindowShouldClose()) {
        gameTimer += GetFrameTime();
        UpdateMusicStream(backgroundMusic);
        
        // Random ad popup trigger (approx every 30-60 seconds)
        /*if (gameState > 0 && gameState < 5 && gameTimer > 30.0f && GetRandomValue(0, 100) < 2 && !adPopup.active) {
            triggerAdPopup(&adPopup);
            PlaySound(adPopupSound);
            gameTimer = 0.0f;
        }*/
        
        BeginDrawing();
        ClearBackground(DARKGREEN);
        
        switch (gameState) {
            case 0:
                DrawText("The Winning Hand - Poker Game", 400, 200, 30, WHITE);
                DrawText("Press SPACE to start the game", 500, 250, 20, WHITE);
                DrawText("Press S to view player statistics", 500, 280, 20, WHITE);
                
                if (IsKeyPressed(KEY_SPACE)) {
                    gameState = 6;
                    PlaySound(cardDealSound);
                    loadingTimer = 0.0f;
                }
                if (IsKeyPressed(KEY_S)) {
                    gameState = 7; 
                }
                break;
                
            case 1:
                DrawText("Pre-flop Round", 500, 50, 30, WHITE);
                displayGameInfo(players, numPlayers, &community, currentPlayer, pot);
                
                if (!roundOver) {
                    if (!raiseInputMode) {
                        DrawText("Options:", 900, 500, 20, WHITE);
                        DrawText("F - Fold", 900, 530, 20, WHITE);
                        DrawText("C - Call/Check", 900, 560, 20, WHITE);
                        DrawText("R - Raise", 900, 590, 20, WHITE);
                        
                        while (players[currentPlayer].folded) {
                            currentPlayer = (currentPlayer + 1) % numPlayers;
                            if (currentPlayer == roundStartPlayer) {
                                roundOver = 1;
                                break;
                            }
                        }
                        
                        if (!roundOver) {
                            if (IsKeyPressed(KEY_F)) {
                                players[currentPlayer].folded = 1;
                                PlaySound(foldSound);
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_C)) {
                                int callAmount = highestBet - players[currentPlayer].bet;
                                if (callAmount > 0 && callAmount <= players[currentPlayer].chips) {
                                    players[currentPlayer].chips -= callAmount;
                                    players[currentPlayer].bet += callAmount;
                                    pot += callAmount;
                                    PlaySound(betSound);
                                }
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_R)) {
                                raiseInputMode = 1;
                                memset(raiseInputBuffer, 0, sizeof(raiseInputBuffer));
                            }
                        }
                    } else {
                        handleRaiseInput(players, &currentPlayer, &pot, &highestBet, &roundStartPlayer);
                    }
                } else {
                    DrawText("Press SPACE for Flop", 500, 650, 20, WHITE);
                    if (IsKeyPressed(KEY_SPACE)) {
                        gameState = 2;
                        roundOver = 0;
                        dealCommunityCards(&community, &deck, 3);
                        PlaySound(cardDealSound);
                    }
                }
                break;
                
            case 2:
                if (!flopAdShown) {
                    triggerAdPopup(&adPopup);
                    PlaySound(adPopupSound);
                    flopAdShown = 1;
                }
                DrawText("Flop Round", 500, 50, 30, WHITE);
                displayGameInfo(players, numPlayers, &community, currentPlayer, pot);
                
                if (!roundOver) {
                    if (!raiseInputMode) {
                        DrawText("Options:", 900, 500, 20, WHITE);
                        DrawText("F - Fold", 900, 530, 20, WHITE);
                        DrawText("C - Call/Check", 900, 560, 20, WHITE);
                        DrawText("R - Raise", 900, 590, 20, WHITE);
                        
                        while (players[currentPlayer].folded) {
                            currentPlayer = (currentPlayer + 1) % numPlayers;
                            if (currentPlayer == roundStartPlayer) {
                                roundOver = 1;
                                break;
                            }
                        }
                        
                        if (!roundOver) {
                            if (IsKeyPressed(KEY_F)) {
                                players[currentPlayer].folded = 1;
                                PlaySound(foldSound);
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_C)) {
                                int callAmount = highestBet - players[currentPlayer].bet;
                                if (callAmount > 0 && callAmount <= players[currentPlayer].chips) {
                                    players[currentPlayer].chips -= callAmount;
                                    players[currentPlayer].bet += callAmount;
                                    pot += callAmount;
                                    PlaySound(betSound);
                                }
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_R)) {
                                raiseInputMode = 1;
                                memset(raiseInputBuffer, 0, sizeof(raiseInputBuffer));
                            }
                        }
                    } else {
                        handleRaiseInput(players, &currentPlayer, &pot, &highestBet, &roundStartPlayer);
                    }
                } else {
                    DrawText("Press SPACE for Turn", 500, 650, 20, WHITE);
                    if (IsKeyPressed(KEY_SPACE)) {
                        gameState = 3;
                        roundOver = 0;
                        dealCommunityCards(&community, &deck, 1);
                        PlaySound(cardDealSound);
                    }
                }
                break;
                
            case 3:
                if (!turnAdShown) {
                triggerAdPopup(&adPopup);
                PlaySound(adPopupSound);
                turnAdShown = 1;
            }
                DrawText("Turn Round", 500, 50, 30, WHITE);
                displayGameInfo(players, numPlayers, &community, currentPlayer, pot);
                
                if (!roundOver) {
                    if (!raiseInputMode) {
                        DrawText("Options:", 900, 500, 20, WHITE);
                        DrawText("F - Fold", 900, 530, 20, WHITE);
                        DrawText("C - Call/Check", 900, 560, 20, WHITE);
                        DrawText("R - Raise", 900, 590, 20, WHITE);
                        
                        while (players[currentPlayer].folded) {
                            currentPlayer = (currentPlayer + 1) % numPlayers;
                            if (currentPlayer == roundStartPlayer) {
                                roundOver = 1;
                                break;
                            }
                        }
                        
                        if (!roundOver) {
                            if (IsKeyPressed(KEY_F)) {
                                players[currentPlayer].folded = 1;
                                PlaySound(foldSound);
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_C)) {
                                int callAmount = highestBet - players[currentPlayer].bet;
                                if (callAmount > 0 && callAmount <= players[currentPlayer].chips) {
                                    players[currentPlayer].chips -= callAmount;
                                    players[currentPlayer].bet += callAmount;
                                    pot += callAmount;
                                    PlaySound(betSound);
                                }
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_R)) {
                                raiseInputMode = 1;
                                memset(raiseInputBuffer, 0, sizeof(raiseInputBuffer));
                            }
                        }
                    } else {
                        handleRaiseInput(players, &currentPlayer, &pot, &highestBet, &roundStartPlayer);
                    }
                } else {
                    DrawText("Press SPACE for River", 500, 650, 20, WHITE);
                    if (IsKeyPressed(KEY_SPACE)) {
                        gameState = 4;
                        roundOver = 0;
                        dealCommunityCards(&community, &deck, 1);
                        PlaySound(cardDealSound);
                    }
                }
                break;
                
            case 4:
            if (!riverAdShown) {
                triggerAdPopup(&adPopup);
                PlaySound(adPopupSound);
                riverAdShown = 1;
            }
                DrawText("River Round", 500, 50, 30, WHITE);
                displayGameInfo(players, numPlayers, &community, currentPlayer, pot);
                
                if (!roundOver) {
                    if (!raiseInputMode) {
                        DrawText("Options:", 900, 500, 20, WHITE);
                        DrawText("F - Fold", 900, 530, 20, WHITE);
                        DrawText("C - Call/Check", 900, 560, 20, WHITE);
                        DrawText("R - Raise", 900, 590, 20, WHITE);

                        
                        while (players[currentPlayer].folded) {
                            currentPlayer = (currentPlayer + 1) % numPlayers;
                            if (currentPlayer == roundStartPlayer) {
                                roundOver = 1;
                                break;
                            }
                        }
                        
                        if (!roundOver) {
                            if (IsKeyPressed(KEY_F)) {
                                players[currentPlayer].folded = 1;
                                PlaySound(foldSound);
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_C)) {
                                int callAmount = highestBet - players[currentPlayer].bet;
                                if (callAmount > 0 && callAmount <= players[currentPlayer].chips) {
                                    players[currentPlayer].chips -= callAmount;
                                    players[currentPlayer].bet += callAmount;
                                    pot += callAmount;
                                    PlaySound(betSound);
                                }
                                currentPlayer = (currentPlayer + 1) % numPlayers;
                                
                                if (currentPlayer == roundStartPlayer) {
                                    roundOver = 1;
                                }
                            } else if (IsKeyPressed(KEY_R)) {
                                raiseInputMode = 1;
                                memset(raiseInputBuffer, 0, sizeof(raiseInputBuffer));
                            }
                        }
                    } else {
                        handleRaiseInput(players, &currentPlayer, &pot, &highestBet, &roundStartPlayer);
                    }
                } else {
                    DrawText("Press SPACE for Showdown", 500, 650, 20, WHITE);
                    if (IsKeyPressed(KEY_SPACE)) {
                        gameState = 5;
                        evaluateAllHands(players, numPlayers, &community);
                    }
                }
                break;
                
            case 5:  
                DrawText("Showdown", 500, 50, 30, WHITE);
                displayGameInfo(players, numPlayers, &community, -1, pot);
                
                winner = compareHands(players, numPlayers, &community);
                    if (winner >= 0) {
                        DrawText(TextFormat("Player %s wins $%d!", players[winner].name, pot), 400, 300, 30, GOLD);
                        DrawText(TextFormat("Winning Hand: %s", players[winner].handName), 400, 340, 24, GOLD);
                        //for (int i = 0; i < numPlayers; i++) {
                            //savePlayerProfile(players[i], (i == winner), pot);
                        //}
                    } else {
                        DrawText("No winners (all folded)!", 400, 300, 30, RED);
                        //for (int i = 0; i < numPlayers; i++) {
                          // savePlayerProfile(players[i], 0, pot);
                        //}
                    }
                    
                    static bool profilesSaved = false;
                    if (!profilesSaved) {
                        for (int i = 0; i < numPlayers; i++) {
                            savePlayerProfile(players[i], (i == winner), pot);
                        }
                        profilesSaved = true;
                    }       
                
                    static bool soundPlayed = false;
                    if (!soundPlayed) {
                        PlaySound(winSound);
                        soundPlayed = true;
                    }
                
                
                    
                
                
                DrawText("All player hands:", 900, 370, 20, WHITE);
                for (int i = 0; i < numPlayers; i++) {
                    if (!players[i].folded) {
                        DrawText(TextFormat("%s:", players[i].name), 750, 400 + i * 50, 20, WHITE);
                        drawCard(players[i].hand[0], 850, 400 + i * 50, 20);
                        drawCard(players[i].hand[1], 900, 400 + i * 50, 20);
                        DrawText(TextFormat("%s", players[i].handName), 1000, 420 + i * 50, 20, 
                              (i == winner) ? GOLD : WHITE);
                    } else {
                        DrawText(TextFormat("%s: (Folded)", players[i].name), 1000, 400 + i * 50, 20, GRAY);
                    }
                }
                
                DrawText("Game Over! Press R to restart", 400, 650, 30, WHITE);
                if (IsKeyPressed(KEY_R)) {
                    initDeck(&deck);
                    shuffleDeck(&deck);
                    community.count = 0;
                    pot = 0;
                    currentPlayer = 0;
                    roundStartPlayer = 0;
                    highestBet = 0;
                    raiseInputMode = 0;
                    flopAdShown = 0;
                    turnAdShown = 0;
                    riverAdShown = 0;
                    
                    for (int i = 0; i < numPlayers; i++) {
                        players[i].chips = 1000;
                        players[i].bet = 0;
                        players[i].folded = 0;
                        players[i].handRank = 0;
                        players[i].handHighCard = 0;
                        strcpy(players[i].handName, "");
                    }

                    dealHoleCards(players, numPlayers, &deck);
                    PlaySound(cardDealSound);
                    
                    gameState = 0;
                    roundOver = 0;
                }
                break;
                
            case 6:  
                loadingTimer += GetFrameTime();
                drawLoadingScreen(loadingTimer);
        
                if (loadingTimer >= 5.0f) {  
                    gameState = 8;  
                    nameInputMode = 1;
                    currentNameInput = 0;
                    memset(nameInputBuffer, 0, sizeof(nameInputBuffer));
                }
                break;
                
            case 7:  
                DrawText("Player Statistics", 500, 50, 30, WHITE);
                
                static int statsPlayerIndex = 0;
                
                displayPlayerStats(players[statsPlayerIndex]);
                
                DrawText("Press LEFT/RIGHT to view different players", 400, 400, 20, WHITE);
                DrawText("Press SPACE to return to main menu", 400, 430, 20, WHITE);
                
                if (IsKeyPressed(KEY_RIGHT)) {
                    statsPlayerIndex = (statsPlayerIndex + 1) % numPlayers;
                }
                if (IsKeyPressed(KEY_LEFT)) {
                    statsPlayerIndex = (statsPlayerIndex - 1 + numPlayers) % numPlayers;
                }
                if (IsKeyPressed(KEY_SPACE)) {
                    gameState = 0;  
                }
                break;
                
            case 8:  
                handleNameInput(players, numPlayers, &gameState);
                
                /*if (gameState == 1) {
                    loadPlayerProfiles(players, numPlayers);
                }*/
                break;
        }
        
        if (adPopup.active) {
            drawAdPopup(&adPopup);
            
            adPopup.timeRemaining -= GetFrameTime();
            if (adPopup.timeRemaining <= 0) {
                adPopup.active = 0;
            }
            
            if (checkCloseButtonPressed(&adPopup)) {
                adPopup.active = 0;
            }
        }
        
        EndDrawing();
}
    
    unloadGameSounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void loadGameSounds() {
    cardDealSound = LoadSound("card_deal_sound.wav");      
    betSound = LoadSound("cash_register.wav");       
    foldSound = LoadSound("toilet_flush.wav");       
    winSound = LoadSound("applause.wav");            
    adPopupSound = LoadSound("error_beep.wav");      
    backgroundMusic = LoadMusicStream("casino_jazz.mp3"); 
}

void unloadGameSounds() {
    // Unload all sounds to free memory
    UnloadSound(cardDealSound);
    UnloadSound(betSound);
    UnloadSound(foldSound);
    UnloadSound(winSound);
    UnloadSound(adPopupSound);
    UnloadMusicStream(backgroundMusic);
}

void handleRaiseInput(Player *players, int *currentPlayer, int *pot, int *highestBet, int *roundStartPlayer) {
    DrawText("Enter raise amount:", 900, 510, 20, WHITE);
    DrawText(raiseInputBuffer, 910, 550, 20, BLACK);
    DrawText("Press ENTER to confirm", 900, 590, 15, WHITE);
    
    int key = GetKeyPressed();
    if ((key >= 48 && key <= 57) && strlen(raiseInputBuffer) < 9) { 
        raiseInputBuffer[strlen(raiseInputBuffer)] = (char)key;
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(raiseInputBuffer) > 0) {
        raiseInputBuffer[strlen(raiseInputBuffer) - 1] = '\0';
    }
  
    if (IsKeyPressed(KEY_ENTER) && strlen(raiseInputBuffer) > 0) {
        raiseAmount = atoi(raiseInputBuffer);
 
        if (raiseAmount > 0 && raiseAmount <= players[*currentPlayer].chips) {
            *highestBet = players[*currentPlayer].bet + raiseAmount;
            players[*currentPlayer].chips -= raiseAmount;
            players[*currentPlayer].bet += raiseAmount;
            *pot += raiseAmount;
            *roundStartPlayer = *currentPlayer; 
            *currentPlayer = (*currentPlayer + 1) % 4;
            raiseInputMode = 0;
            PlaySound(betSound);
        } else {
            memset(raiseInputBuffer, 0, sizeof(raiseInputBuffer));
        }
    }
}

void initDeck(Deck *deck) {
    int idx = 0;
    for (int suit = 0; suit < 4; suit++) {
        for (int value = 2; value <= 14; value++) {
            deck->cards[idx].suit = suit;
            deck->cards[idx].value = value;
            idx++;
        }
    }
    deck->top = 0;
}

void shuffleDeck(Deck *deck) {
    for (int i = 51; i > 0; i--) {
        int j = rand() % (i + 1);
        Card temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
}

Card dealCard(Deck *deck) {
    Card card = deck->cards[deck->top];
    deck->top++;
    return card;
}

void dealHoleCards(Player *players, int numPlayers, Deck *deck) {
    for (int i = 0; i < numPlayers; i++) {
        for (int j = 0; j < 2; j++) {
            players[i].hand[j] = dealCard(deck);
        }
    }
}

void dealCommunityCards(Community *community, Deck *deck, int count) {
    for (int i = 0; i < count; i++) {
        community->cards[community->count] = dealCard(deck);
        community->count++;
    }
}

void displayGameInfo(Player *players, int numPlayers, Community *community, int currentPlayer, int pot) {
    DrawText("Community Cards:", 400, 100, 20, WHITE);
    for (int i = 0; i < community->count; i++) {
        drawCard(community->cards[i], 400 + i * 120, 130, 30);
    }
    
    DrawText(TextFormat("Pot: $%d", pot), 500, 400, 24, YELLOW);
    
    DrawText("Players:", 50, 270, 22, WHITE);
    for (int i = 0; i < numPlayers; i++) {
        int yPos = 300 + i * 45;
        
        if (i == currentPlayer && !players[i].folded) {
            DrawRectangle(45, yPos - 5, 300, 30, (Color){0, 100, 0, 150});
        }
        
        DrawText(TextFormat("%s: $%d", players[i].name, players[i].chips), 50, yPos, 20, WHITE);
        
        if (players[i].bet > 0) {
            DrawText(TextFormat("Bet: $%d", players[i].bet), 220, yPos, 20, YELLOW);
        }
        
        if (players[i].folded) {
            DrawText("(Folded)", 320, yPos, 20, RED);
        } else if (i == currentPlayer) {
            DrawText("(Current Turn)", 320, yPos, 20, GREEN);
        }
    }
    
    if (currentPlayer >= 0 && currentPlayer < numPlayers && !players[currentPlayer].folded) {
        DrawText(TextFormat("%s's turn. Other players, please look away!", players[currentPlayer].name), 
                 750, 300, 20, RED);
        DrawText("Your hand:", 810, 330, 20, WHITE);
        
        drawCard(players[currentPlayer].hand[0], 850, 360, 30);
        drawCard(players[currentPlayer].hand[1], 1000, 360, 30);
    }
}

void drawCard(Card card, int x, int y, int size) {
    int cardWidth = size * 3;
    int cardHeight = size * 4;
    
    Color suitColor = (card.suit == 0 || card.suit == 1) ? RED : BLACK;
    
    DrawRectangle(x, y, cardWidth, cardHeight, WHITE);
    DrawRectangleLines(x, y, cardWidth, cardHeight, BLACK);
    
    DrawRectangleLines(x + 2, y + 2, cardWidth - 4, cardHeight - 4, GRAY);
    
    DrawText(values[card.value], x + 5, y + 5, size, suitColor);
    
    int suitTextSize = size * 0.6;
    const char* suitText = suits[card.suit];
    Vector2 textSize = MeasureTextEx(GetFontDefault(), suitText, suitTextSize, 1);
    float centerX = x + (cardWidth - textSize.x) / 2;
    float centerY = y + (cardHeight - textSize.y) / 2;
    
    DrawText(suitText, centerX, centerY, suitTextSize, suitColor);
    
    DrawText(values[card.value], x + cardWidth - 20, y + cardHeight - 25, size, suitColor);
}

void sortCards(Card *cards, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (cards[i].value < cards[j].value) {
                Card temp = cards[i];
                cards[i] = cards[j];
                cards[j] = temp;
            }
        }
    }
}

int countValues(Card *cards, int count, int value) {
    int result = 0;
    for (int i = 0; i < count; i++) {
        if (cards[i].value == value) {
            result++;
        }
    }
    return result;
}

int countSuits(Card *cards, int count, int suit) {
    int result = 0;
    for (int i = 0; i < count; i++) {
        if (cards[i].suit == suit) {
            result++;
        }
    }
    return result;
}

int checkStraight(Card *cards, int count, int *highCard) {
    int values[15] = {0};
    
    for (int i = 0; i < count; i++) {
        values[cards[i].value] = 1;
    }
    
    if (values[14] && values[2] && values[3] && values[4] && values[5]) {
        *highCard = 5;
        return 1;
    }
    
    int consecutive = 0;
    for (int i = 14; i >= 2; i--) {
        if (values[i]) {
            consecutive++;
            if (consecutive == 5) {
                *highCard = i + 4;
                return 1;
            }
        } else {
            consecutive = 0;
        }
    }
    
    return 0;
}

int checkFlush(Card *cards, int count, int *suit) {
    int suitCounts[4] = {0};
    
    for (int i = 0; i < count; i++) {
        suitCounts[cards[i].suit]++;
    }
    
    for (int i = 0; i < 4; i++) {
        if (suitCounts[i] >= 5) {
            *suit = i;
            return 1;
        }
    }
    
    return 0;
}

int checkFullHouse(Card *cards, int count, int *threeValue, int *pairValue) {
    int hasThree = 0;
    int hasPair = 0;
    *threeValue = 0;
    *pairValue = 0;
    
    for (int i = 14; i >= 2; i--) {
        if (countValues(cards, count, i) >= 3) {
            *threeValue = i;
            hasThree = 1;
            break;
        }
    }
    
    if (hasThree) {
        for (int i = 14; i >= 2; i--) {
            if (i != *threeValue && countValues(cards, count, i) >= 2) {
                *pairValue = i;
                hasPair = 1;
                break;
            }
        }
    }
    
    return (hasThree && hasPair);
}

int checkTwoPair(Card *cards, int count, int *highPair, int *lowPair) {
    *highPair = 0;
    *lowPair = 0;
    
    for (int i = 14; i >= 2; i--) {
        if (countValues(cards, count, i) >= 2) {
            *highPair = i;
            break;
        }
    }
    
    if (*highPair > 0) {
        for (int i = 14; i >= 2; i--) {
            if (i != *highPair && countValues(cards, count, i) >= 2) {
                *lowPair = i;
                break;
            }
        }
    }
    
    return (*highPair > 0 && *lowPair > 0);
}

int evaluateHand(Card *hand, int handSize, int *highCard, char *handName) {
    sortCards(hand, handSize);
    
    int flushSuit = -1;
    if (checkFlush(hand, handSize, &flushSuit)) {
        // Create a temporary hand with only the flush suit cards
        Card flushCards[7];
        int flushCount = 0;
        
        for (int i = 0; i < handSize; i++) {
            if (hand[i].suit == flushSuit) {
                flushCards[flushCount] = hand[i];
                flushCount++;
            }
        }
        
        int straightHighCard = 0;
        if (checkStraight(flushCards, flushCount, &straightHighCard)) {
            if (straightHighCard == 14) {
                *highCard = 14;
                sprintf(handName, "Royal Flush");
                return 10;
            }
            else {
                *highCard = straightHighCard;
                sprintf(handName, "Straight Flush, %s High", values[*highCard]);
                return 9;
            }
        }
    }
    
    for (int i = 14; i >= 2; i--) {
        if (countValues(hand, handSize, i) == 4) {
            *highCard = i;
            sprintf(handName, "Four of a Kind, %ss", values[i]);
            return 8;
        }
    }
    
    int threeValue = 0;
    int pairValue = 0;
    if (checkFullHouse(hand, handSize, &threeValue, &pairValue)) {
        *highCard = threeValue;
        sprintf(handName, "Full House, %ss over %ss", values[threeValue], values[pairValue]);
        return 7;
    }
    
    if (flushSuit >= 0 || checkFlush(hand, handSize, &flushSuit)) {
        // Find highest card of the flush suit
        for (int i = 0; i < handSize; i++) {
            if (hand[i].suit == flushSuit) {
                *highCard = hand[i].value;
                break;
            }
        }
        sprintf(handName, "Flush, %s High", values[*highCard]);
        return 6;
    }
    
    int straightHighCard = 0;
    if (checkStraight(hand, handSize, &straightHighCard)) {
        *highCard = straightHighCard;
        sprintf(handName, "Straight, %s High", values[*highCard]);
        return 5;
    }
    
    for (int i = 14; i >= 2; i--) {
        if (countValues(hand, handSize, i) == 3) {
            *highCard = i;
            sprintf(handName, "Three of a Kind, %ss", values[i]);
            return 4;
        }
    }
    
    int highPair = 0;
    int lowPair = 0;
    if (checkTwoPair(hand, handSize, &highPair, &lowPair)) {
        *highCard = highPair;
        sprintf(handName, "Two Pair, %ss and %ss", values[highPair], values[lowPair]);
        return 3;
    }
    
    for (int i = 14; i >= 2; i--) {
        if (countValues(hand, handSize, i) == 2) {
            *highCard = i;
            sprintf(handName, "One Pair, %ss", values[i]);
            return 2;
        }
    }
    
    *highCard = hand[0].value;
    sprintf(handName, "High Card, %s", values[*highCard]);
    return 1;
}

void evaluateAllHands(Player *players, int numPlayers, Community *community) {
    for (int i = 0; i < numPlayers; i++) {
        if (!players[i].folded) {
            Card allCards[7];
            allCards[0] = players[i].hand[0];
            allCards[1] = players[i].hand[1];
            
            for (int j = 0; j < community->count; j++) {
                allCards[j + 2] = community->cards[j];
            }
            
            int highCard = 0;
            players[i].handRank = evaluateHand(allCards, community->count + 2, &highCard, players[i].handName);
            players[i].handHighCard = highCard;
        }
    }
}

int compareHands(Player *players, int numPlayers, Community *community) {
    int bestRank = 0;
    int bestHighCard = 0;
    int winner = -1;
    int tieCheck = 0;
    
    for (int i = 0; i < numPlayers; i++) {
        if (!players[i].folded) {
            if (players[i].handRank > bestRank || 
                (players[i].handRank == bestRank && players[i].handHighCard > bestHighCard)) {
                bestRank = players[i].handRank;
                bestHighCard = players[i].handHighCard;
                winner = i;
                tieCheck = 0;
            } else if (players[i].handRank == bestRank && players[i].handHighCard == bestHighCard) {
                tieCheck = 1;
            }
        }
    }
    
    return winner;
}

void triggerAdPopup(AdPopup *ad) {
    static const char *adTitles[] = {
        "CONGRATULATIONS!!!",
        "YOU WON A PRIZE!",
        "YOUR COMPUTER HAS A VIRUS!",
        "YOU'VE WON A FREE SPIN!",
        "BECOME A MILLIONAIRE TODAY",
        "EXCLUSIVE CASINO OFFER"
    };
    
    static const char *adMessages[] = {
        "You're our 1,000,000th visitor! \nClick now to claim your prize!",
        "Your computer needs an urgent \nupdate to fix 27 critical errors!",
        "You've been selected for \nour exclusive poker tournament!",
        "Join our VIP players club \nand get 500 free chips!",
        "Only 3 spots left for our poker \nmasterclass with professionals!",
        "Your antivirus subscription has \nexpired! Renew now for 70% off!"
    };
    
    int titleIndex = GetRandomValue(0, 5);
    int messageIndex = GetRandomValue(0, 5);
    
    strcpy(ad->title, adTitles[titleIndex]);
    strcpy(ad->message, adMessages[messageIndex]);
    
    ad->active = 1;
    ad->width = 400;
    ad->height = 250;
    ad->x = GetRandomValue(50, GetScreenWidth() - ad->width - 50);
    ad->y = GetRandomValue(50, GetScreenHeight() - ad->height - 50);
    ad->timeRemaining = 10.0f;  // Pop-up stays visible for 10 seconds unless closed
}

void drawAdPopup(AdPopup *ad) {
    if (!ad->active) return;
    
    DrawRectangle(ad->x, ad->y, ad->width, ad->height, WHITE);
    DrawRectangleLines(ad->x, ad->y, ad->width, ad->height, BLACK);
    
    DrawRectangle(ad->x, ad->y, ad->width, 30, RED);
    DrawText(ad->title, ad->x + 10, ad->y + 5, 20, WHITE);
    
    DrawRectangle(ad->x + ad->width - 30, ad->y, 30, 30, RED);
    DrawText("X", ad->x + ad->width - 20, ad->y + 5, 20, WHITE);
    
    DrawText(ad->message, ad->x + 10, ad->y + 50, 20, BLACK);
    
    DrawRectangle(ad->x + ad->width/2 - 50, ad->y + ad->height - 50, 100, 30, GREEN);
    DrawText("CLICK HERE", ad->x + ad->width/2 - 45, ad->y + ad->height - 45, 15, WHITE);
}

bool checkCloseButtonPressed(AdPopup *ad) {
    if (!ad->active) return false;
    
    Vector2 mousePos = GetMousePosition();
    Rectangle closeBtn = {
        ad->x + ad->width - 30, 
        ad->y, 
        30, 
        30
    };
    
    if (CheckCollisionPointRec(mousePos, closeBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        return true;
    }
    
    return false;
}

void drawLoadingScreen(float timeElapsed) {
    ClearBackground(DARKGREEN);
    
    float progress = timeElapsed / 5.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    DrawRectangle(GetScreenWidth()/2 - 150, GetScreenHeight()/2 + 50, 300, 20, DARKGRAY);
    DrawRectangle(GetScreenWidth()/2 - 150, GetScreenHeight()/2 + 50, (int)(300 * progress), 20, WHITE);
    
    DrawText("Loading...", GetScreenWidth()/2 - 50, GetScreenHeight()/2 + 80, 20, WHITE);
    
    DrawText("Did you know?", GetScreenWidth()/2 - 80, GetScreenHeight()/2 - 100, 30, GOLD);
    DrawText("99% of gamblers quit right before they hit the jackpot", 
             GetScreenWidth()/2 - 250, GetScreenHeight()/2 - 50, 20, WHITE);
    
    const int radius = 20;
    const int centerX = GetScreenWidth()/2;
    const int centerY = GetScreenHeight()/2;
    
    DrawCircleLines(centerX, centerY, radius, GRAY);
    
    float startAngle = timeElapsed * 360.0f;
    float endAngle = startAngle + 90.0f;
    DrawCircleSector((Vector2){centerX, centerY}, radius, startAngle, endAngle, 0, WHITE);
}

void savePlayerProfile(Player player, int wonGame, int potSize) {
    PlayerProfile profile;
    char filename[60];
    memset(&profile, 0, sizeof(PlayerProfile));
    strcpy(profile.name, player.name);

    strcpy(filename, player.name);
    strcat(filename, ".txt");

    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        char line[100];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Name: ", 6) == 0) {
                profile.name[strcspn(line + 6, "\n")] = '\0'; // clean newline
                strcpy(profile.name, line + 6);
            } else if (strncmp(line, "GamesPlayed: ", 13) == 0) {
                profile.gamesPlayed = atoi(line + 13);
            } else if (strncmp(line, "GamesWon: ", 10) == 0) {
                profile.gamesWon = atoi(line + 10);
            } else if (strncmp(line, "TotalEarnings: ", 15) == 0) {
                profile.totalEarnings = atoi(line + 15);
            } else if (strncmp(line, "BiggestPot: ", 12) == 0) {
                profile.biggestPot = atoi(line + 12);
            } else if (strncmp(line, "BestHand: ", 10) == 0) {
                strcpy(profile.bestHand, line + 10);
                profile.bestHand[strcspn(profile.bestHand, "\n")] = '\0'; 
            }
        }
        fclose(file);
        profile.gamesPlayed++;
    } else {
   
    profile.gamesPlayed = 1;  
    profile.gamesWon = 0;
    profile.totalEarnings = player.chips > 1000 ? (player.chips - 1000) : 0;
    profile.biggestPot = potSize;
    if (strlen(player.handName) > 0)
        strcpy(profile.bestHand, player.handName);
    else
        strcpy(profile.bestHand, "None yet");
}
    
    if (wonGame) profile.gamesWon++;
    if (player.chips > 1000)
        profile.totalEarnings += (player.chips - 1000);
    if (potSize > profile.biggestPot)
        profile.biggestPot = potSize;
    if (strlen(player.handName) > 0 && player.handRank > 1)  
        strcpy(profile.bestHand, player.handName);

    file = fopen(filename, "w");
    if (file != NULL) {
        fprintf(file, "Name: %s\n", profile.name);
        fprintf(file, "GamesPlayed: %d\n", profile.gamesPlayed);
        fprintf(file, "GamesWon: %d\n", profile.gamesWon);
        fprintf(file, "TotalEarnings: %d\n", profile.totalEarnings);
        fprintf(file, "BiggestPot: %d\n", profile.biggestPot);
        fprintf(file, "BestHand: %s\n", profile.bestHand);
        fclose(file);
    }
    if (player.chips < 0 || player.chips > 1000000) return;
}

void displayPlayerStats(Player player) {
    PlayerProfile profile;
    char filename[60];
    
    memset(&profile, 0, sizeof(PlayerProfile));
    
    strcpy(filename, player.name);
    strcat(filename, ".txt");
    
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        char line[100];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Name: ", 6) == 0) {
                profile.name[strcspn(line + 6, "\n")] = '\0';
                strcpy(profile.name, line + 6);
            } else if (strncmp(line, "GamesPlayed: ", 13) == 0) {
                profile.gamesPlayed = atoi(line + 13);
            } else if (strncmp(line, "GamesWon: ", 10) == 0) {
                profile.gamesWon = atoi(line + 10);
            } else if (strncmp(line, "TotalEarnings: ", 15) == 0) {
                profile.totalEarnings = atoi(line + 15);
            } else if (strncmp(line, "BiggestPot: ", 12) == 0) {
                profile.biggestPot = atoi(line + 12);
            } else if (strncmp(line, "BestHand: ", 10) == 0) {
                strcpy(profile.bestHand, line + 10);
                profile.bestHand[strcspn(profile.bestHand, "\n")] = '\0';
            }
        }
        fclose(file);
        
        DrawText(TextFormat("Player: %s", profile.name), 50, 100, 24, WHITE);
        
        DrawText("Player Stats:", 50, 130, 24, WHITE);
        DrawText(TextFormat("Games Played: %d", profile.gamesPlayed), 50, 170, 18, WHITE);
        DrawText(TextFormat("Games Won: %d", profile.gamesWon), 50, 200, 18, WHITE);
        DrawText(TextFormat("Win Rate: %.1f%%", 
                 profile.gamesPlayed > 0 ? (float)profile.gamesWon / profile.gamesPlayed * 100 : 0), 
                 50, 230, 18, WHITE);
        DrawText(TextFormat("Total Earnings: $%d", profile.totalEarnings), 50, 260, 18, WHITE);
        DrawText(TextFormat("Biggest Pot Won: $%d", profile.biggestPot), 50, 290, 18, WHITE);
        DrawText(TextFormat("Best Hand: %s", profile.bestHand), 50, 320, 18, WHITE);
    } else {
        DrawText(TextFormat("Player: %s", player.name), 50, 100, 24, WHITE);
        DrawText("No previous game data found.", 50, 170, 18, WHITE);
    }
}
void saveGameConfig(int numPlayers, int startingChips) {
    FILE *file = fopen("poker_config.txt", "w");  // Changed to .txt and text mode
    if (file != NULL) {
        fprintf(file, "NumPlayers: %d\n", numPlayers);
        fprintf(file, "StartingChips: %d\n", startingChips);
        fclose(file);
    }
}

void loadGameConfig(int *numPlayers, int *startingChips) {
    FILE *file = fopen("poker_config.txt", "r");  
    if (file != NULL) {
        fscanf(file, "NumPlayers: %d\n", numPlayers);
        fscanf(file, "StartingChips: %d\n", startingChips);
        fclose(file);
    }
}

void handleNameInput(Player *players, int numPlayers, int *gameState) {
    DrawText("Enter Player Names", 500, 50, 30, WHITE);
    
    DrawText(TextFormat("Enter name for Player %d:", currentNameInput + 1), 
             GetScreenWidth()/2 - 150, GetScreenHeight()/2 - 50, 24, WHITE);
    
    DrawRectangle(GetScreenWidth()/2 - 150, GetScreenHeight()/2, 300, 40, WHITE);
    DrawText(nameInputBuffer, GetScreenWidth()/2 - 140, GetScreenHeight()/2 + 10, 20, BLACK);
    
    int key = GetKeyPressed();
    
    if (((key >= 65 && key <= 90) || (key >= 97 && key <= 122) || 
         (key >= 48 && key <= 57) || (key == 32 && strlen(nameInputBuffer) > 0)) && 
        strlen(nameInputBuffer) < 20) {
        nameInputBuffer[strlen(nameInputBuffer)] = (char)key;
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(nameInputBuffer) > 0) {
        nameInputBuffer[strlen(nameInputBuffer) - 1] = '\0';
    }
    
    if (IsKeyPressed(KEY_ENTER) && strlen(nameInputBuffer) > 0) {
        strcpy(players[currentNameInput].name, nameInputBuffer);
        
        memset(nameInputBuffer, 0, sizeof(nameInputBuffer));
        
        currentNameInput++;
        if (currentNameInput >= numPlayers) {
            currentNameInput = 0;
            *gameState = 1; // Move to actual game
            nameInputMode = 0;
        }
    }
    
    DrawText("Press ENTER to confirm name", GetScreenWidth()/2 - 150, 
             GetScreenHeight()/2 + 50, 20, WHITE);
}