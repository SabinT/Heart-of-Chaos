#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#define CONTROLS_FILE "controls.def"

#include "anim.h"
#include <string>

#define GROUND_Y 440
#define DEFAULT_TIMELIMIT 60

#define FRICTION 1.2
#define JUMP_DX 40.0
#define HIT_DX 20
// but gravity not defined

#define DAMAGE_A1 5
#define DAMAGE_A2 5
#define DAMAGE_A3 15

typedef struct ControlSet {
    int left, right, up, down, a, b, c, d;
};

// A D W S T Y G H
// LT RT UP DN O P L ;
enum Player_Status {
    IDLE, WALKING, BACKING, A1, A2, A3, GUARDING, HIT1, HIT2, DUCKING, JUMPING, NA
};

enum Player_Facing {
    RIGHT, LEFT
};

class Player;
class GameCore;

class Player {
    private:
    SpriteList images, mimages;
    SoundList sounds;

    Mix_Chunk *winSound, *loseSound;

    Animation   entry[2],
                idle[2],
                walk[2],
                back[2],
                a1[2],
                a2[2],
                a3[2],
                guard[2],
                duck[2],
                hit1[2],
                hit2[2],
                jump[2],
                win[2],
                lose[2];

    // what about mirror? [0] for facing rt [1] for left

    ControlSet ctrl;

    Animation* currentAnim;

    bool playerNo;
    std::string name;
    int w, h;

    bool winner; // for post-game checking

    bool facing; // 0 = left to rt    1 = rt to lt
    // put mirrored frames

    Player_Status status;
    bool jumping;
    bool receding;
    // input

    float x, y;
    float lastx, lasty;
    float dx, dy;
    float enemyDistance;
    // to be superimposed over animation's dx dy

    float hp, thp, dhp;
    // sync coordinates

public:
    Player ();
    Player (int PlayerNo);
    ~Player ();

    inline void SetY (int _y) { y = _y; }

    void LoadPlayer (int PlayerNo);
    void FreePlayer ();
    void LoadAnimationSet (Animation* animSet, string fileName, bool loop);
    void UpdateStatus ();
    void Show ();
    void Show (float xOff, float yOff = 0);
    void SetControls (ControlSet gCtrl) {
        ctrl = gCtrl;
    }
    void ResetData ();
    void PutInPlace (Player_Facing face);

    void TakeHit (Player_Status enemyStatus);

    inline void SetWinner (bool value) { winner = value; }
    inline bool GetWinner (bool value) { return winner; }
    void PlayGameOverSound(int channel);

    // for test purposes only
    Animation GetAnim() {
        return walk[0];
    }
    friend class GameCore;
    bool CollideWith (Player& enemy);
};

// **********************************************************************************************
// Now the game

enum MenuResult {
    // for all
    OK, CANCEL,
    // for main menu
    VERSUSMODE, INSTRUCTIONS, CREDITS, EXIT, JPT
};

class GameCore {
  private:
    SpriteList images;
    SoundList sounds;
    int numImages, numSounds, numMaps, numPlayers;
    SDL_Surface *vscreen, *tscreen;
    // for all menus and ui etc, non player specific

    Player p0, p1;
    BackGround stage;

    int p0Index, p1Index, stageIndex;

    int timeLimit;
    float timeElapsed;
    long framesElasped;
    float scroll, tscroll, dscroll; // tscroll = target scroll for smooth scrolling

    bool gameOver;

    ControlSet controls[2];
    Mix_Music *menuMusic, *gameMusic;
    bool musicPlaying;
  public:

    GameCore ();
    ~GameCore ();

    void ResetData ();
    void LoadControls ();
    // collision checking
    // k.o check
    // gameover screen
    // high scores
    // pause overlay
    // timer
    int MainMenu ();
    MenuResult InstructionsMenu ();
    MenuResult SelectMenu ();
    MenuResult CreditsMenu ();

    MenuResult QuitMenu ();

    void JawBite (SDL_Surface *);

    void LoadElements (); // just before match starts, after select menu;
    void FreeElements (); // just before match starts, after select menu;

    void PlayGame ();

    void ShowBackground ();
    void ShowPlayers ();
    void ShowHud ();
    void MakeAllChecks ();
    bool GameOverCheck (); // checks if game is over, shows PlayerX Wins and waits for key
};

inline int sign(float x);
inline float abs(float x);


/* Controls.def format
(for player 0)
left right up down a b c d

(for player 1)
left right up down a b c d

*/
enum CoreImages {
    HPBAR_BACK = 0, HPBAR, HPBAR_RED, MENUBACK, LJAW, RJAW,
    MAINMENU = 6, ARROW,
    INSTRUCTIONSMENU = 8, CREDITSMENU,
    SELECTBACK = 10, SELECTMASK, P0SELECT, P1SELECT,
    THUMB0 = 14, THUMB1, FULL0, FULL1,
    LOADING = 18,
    READY = 19, FIGHT, KO,
    PAUSEOVERLAY = 22, SHADOWOVERLAY
};

enum CoreSounds {
    SELECTOR , SELECT, JAWBITE, DONESELECT, VOICEREADY, VOICEFIGHT, VOICEKO
};

#endif // GAME_H_INCLUDED
