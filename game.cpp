#include "game.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

extern SDL_Graphics gfx;
extern SDL_Sfx sfx;
extern SDL_Input in;
extern TextUtils tx, digital;

const ControlSet default_controls[2] = {
    {97, 100, 119, 115, 116, 121, 103, 104 },
    {276, 275, 273, 274, 111, 112, 108, 59}
    // a d w s u i j k
    // left right up down kp8 kp9 kp5 kp6
};

Player::Player () {
    ResetData ();
}

Player::Player (int PlayerNo) {
    ResetData ();
    LoadPlayer(PlayerNo);
}

void Player::ResetData () {
    x = 320; y = 240;
    dx = dy = 0;
    currentAnim = &idle[0];
    ctrl = default_controls[0];
    facing = RIGHT;
    status = IDLE;
    jumping = 0;
    thp = 100;
    hp = 0;
    receding = false;
}

Player::~Player () {
    FreePlayer();
}

/* **************** Def file format *************
Player Name
numFrames
numSounds

*************************************************/
void Player::LoadPlayer (int PlayerNo) {
    stringstream vcout;
    string folderName;
    string tFolder;

    vcout.clear(); vcout.str("");
    vcout << PlayerNo;
    folderName = string("characters/") + vcout.str();

    string fileName = folderName + "/player.def";
    ifstream defFile(fileName.c_str());
    if (!defFile.good()) {
        cerr << "LoadPlayer: Error opening file " << fileName.c_str() << endl;
        exit(3);
    }

    int numFrames, numSounds;
    getline(defFile, name);         // name
    defFile >> numFrames;           // Frames
    defFile >> numSounds;           // sounds
    defFile >> w;                   // width
    defFile >> h;                   // height

    cout << name << numFrames << numSounds << endl;

    defFile.close();

    tFolder = folderName + "/frames";
    images.LoadSprites (tFolder.c_str(), numFrames, "png" );
    tFolder =folderName + "/frames/mirror";
    mimages.LoadSprites (tFolder.c_str(), numFrames, "png");

    tFolder = folderName + "/sounds";
    sounds.LoadSounds (tFolder.c_str(), numSounds, "wav" );
    fileName = folderName + "/win.wav";
    winSound = sfx.LoadSample(fileName.c_str());
    fileName = folderName + "/win.wav";
    loseSound = sfx.LoadSample(fileName.c_str());

    LoadAnimationSet(entry, folderName + "/frames/entry.adf", false);
    LoadAnimationSet(idle, folderName + "/frames/idle.adf", true);
    LoadAnimationSet(walk, folderName + "/frames/walk.adf", true);
    LoadAnimationSet(back, folderName + "/frames/back.adf", true);
    LoadAnimationSet(a1, folderName + "/frames/a1.adf", false);
    LoadAnimationSet(a2, folderName + "/frames/a2.adf", false);
    LoadAnimationSet(a3, folderName + "/frames/a3.adf", false);
    LoadAnimationSet(jump, folderName + "/frames/jump.adf", false);
    LoadAnimationSet(duck, folderName + "/frames/duck.adf", true);
    LoadAnimationSet(hit1, folderName + "/frames/hit1.adf", false);
    LoadAnimationSet(hit2, folderName + "/frames/hit2.adf", false);
    LoadAnimationSet(win, folderName + "/frames/win.adf", true);
    LoadAnimationSet(lose, folderName + "/frames/lose.adf", true);
    LoadAnimationSet(guard, folderName + "/frames/guard.adf", false);

    // load other adf's here

    currentAnim = &idle[facing];
}

void Player::LoadAnimationSet (Animation *animSet, string fileName, bool loop) {
    animSet[0].Load (&images, &sounds, fileName.c_str());
    animSet[0].EnableLoop(loop);
    animSet[1].LoadMirror (&mimages, &sounds, fileName.c_str());
    animSet[1].EnableLoop(loop);
}

void Player::FreePlayer () {
    images.ClearList();
    mimages.ClearList();
    sounds.ClearList();
    sfx.FreeSample(winSound);
    sfx.FreeSample(loseSound);
}
void Player::PutInPlace (Player_Facing face) {
    facing = face;
    if (facing == RIGHT) {
        x = 640 + 140; y = GROUND_Y;
        ctrl = default_controls[0];
    } else { // facing == LEFT
        x = 640 + 500; y = GROUND_Y;
        ctrl = default_controls[1];
    }

    currentAnim = &idle[facing];
    currentAnim -> SetXY (x,y);
    currentAnim -> Rewind ();
    currentAnim -> playing = 1;
    thp = hp = 100;
}

void Player::UpdateStatus () {
    static bool* keys;
    static Player_Status prevStatus;

    static int hor, ver;
    keys = in.GetKeyState();

    prevStatus = status;
    lastx = x; lasty = y;

    if (currentAnim->playing == 0) {
        if (jumping) status = JUMPING;
        else {
            currentAnim->Rewind(); currentAnim->EnablePlay(true);
            status = IDLE;
        }
    }
    if (jump[facing].playing == false and jumping) {
        jumping = false;
        status = IDLE;
    }

    hor = keys[ctrl.right] - keys[ctrl.left];
    ver = keys[ctrl.up] - keys[ctrl.down];

    // the conditions when status can change
    if (status == BACKING or status == WALKING or status == IDLE or status == DUCKING or status == JUMPING or status == GUARDING)
    {
        Player_Status tstatus = status;
        // conditions when attacks can be made
        if (status != GUARDING) {
            if( keys[ctrl.a] ) status = A1;
            if( keys[ctrl.b] ) status = A2;
            if( keys[ctrl.c] and !jumping) status = A3;
        }

        // conditions to be walking or idle
        if(status == IDLE or status == WALKING or status == BACKING) {
            if (hor != 0) {
                if (hor == 1)
                    if (facing == RIGHT) status = WALKING;
                    else status = BACKING;
                else
                    if (facing == RIGHT) status = BACKING;
                    else status = WALKING;
            } else status = IDLE;
        }

        // for jumping or ducking
        if(status == IDLE or status == WALKING or status == BACKING or status == DUCKING) {
            if (ver == 1) {
                // jump now
                jumping = true;
                status = JUMPING;

                jump[facing].SetXY(x,y);
                jump[facing].Rewind();
                jump[facing].EnablePlay(true);

                // side can change in mid air
                jump[!facing].SetXY(x,y);
                jump[!facing].Rewind();
                jump[!facing].EnablePlay(true);

                dx = hor * (JUMP_DX / gfx.GetFramerate()) * 10; // add direction
                // no dy, the jump anim has it
            } else {
                if (ver == -1)
                    status = DUCKING;
                else
                    if (tstatus == DUCKING) status = IDLE;
                //else status = tstatus;
            }
        }

        // for guarding
        if(status == IDLE or status == WALKING or status == BACKING or status == GUARDING) {
            if (keys[ctrl.d]) status = GUARDING;
            else
                if (tstatus == GUARDING) status = IDLE;
                //else status = tstatus;
        }

    }

    if (prevStatus != status) {
        // then set new anim to current anim
        // take dx but not dy as well and eat it through friction
        switch (status) {
            case IDLE:
                currentAnim = &idle[facing];
                break;
            case WALKING:
                currentAnim = &walk[facing];
                break;
            case BACKING:
                currentAnim = &back[facing];
                break;
            case A1:
                currentAnim = &a1[facing];
                break;
            case A2:
                currentAnim = &a2[facing];
                break;
            case A3:
                currentAnim = &a3[facing];
                break;
            case GUARDING:
                currentAnim = &guard[facing];
                break;
            case JUMPING:
                currentAnim = &jump[facing];
                break;
            case DUCKING:
                currentAnim = &duck[facing];
                break;
            // no case HIT:
                // that is handled during checks
            default:
                break;
        }
        if (status != JUMPING) {
            // jump animation advances on its own accord and is only RESUMED when air attack ends
            currentAnim -> SetXY (x,y);
            currentAnim -> Rewind ();
            currentAnim -> EnablePlay (true);
        }
    }


    if (status != JUMPING) currentAnim->Play();


    if (jumping) {
        // x, y sync from jump
        jump[facing].Play();
        jump[!facing].Play();
        if (!receding) x = jump[facing].GetX() + dx;
        y = jump[facing].GetY();
    } else {
        // x, y sync from current anim
        x = currentAnim -> GetX();
        y = currentAnim -> GetY();
        //dx += currentAnim -> GetDX();
        //if (enemyDistance < 60) dx = (facing ? -1 : 1) * 10;
        if (!receding); x += dx;
        //y += dy;
        if (dx != 0) dx += FRICTION * (- sign(dx));
        if (abs(dx) < .2) dx = 0;
    }

    if (receding) if (facing == RIGHT) x -= 3; else x += 3;

    if (abs(dx) < .2) dx = 0;

    //if (status != JUMPING)
    currentAnim -> SetXY(x,y);
    if (jumping) {
        jump[facing].SetXY(x,y);
        jump[!facing].SetXY(x,y);
    }

    // if there's damage to take then take it
    dhp = (thp - hp) / (gfx.GetFramerate() / 4.0) + sign(thp - hp) * .1;
    if (abs(thp - hp) < 1) {dhp = 0; hp = thp;}
    hp += dhp;

}

void Player::TakeHit (Player_Status enemyStatus) {
    static Player_Status prevStatus; //prevS = NA;

    float multiplier = 0, damage = 0;

    //x = lastx;
    prevStatus = status;

//    currentAnim -> SetXY(x,y);
//    if (jumping) {
//        jump[facing].SetXY(x,y);
//        jump[!facing].SetXY(x,y);
//    }

    //if (s == prevS) return;
    if (status == HIT1 or status == HIT2) return;
    else if (status == GUARDING) multiplier = .2;
    else multiplier = 1;

    switch (enemyStatus) {
        case A1:
            dx = (facing ? 1 : -1) * HIT_DX * 30.0 / gfx.GetFramerate();
            if (status != GUARDING) status = HIT1;
            thp -= (int) (multiplier * DAMAGE_A1);
            break;
        case A2:
            dx = (facing ? 1 : -1) * HIT_DX * 30.0 / gfx.GetFramerate() * 1.2;
            if (status != GUARDING)status = HIT1;
            thp -= (int) multiplier * DAMAGE_A2;
            break;
        case A3:
            dx = (facing ? 1 : -1) * HIT_DX * 30.0 / gfx.GetFramerate() * 2;
            if (status != GUARDING) status = HIT2;
            thp -= (int) multiplier * DAMAGE_A3;
            break;
            // decrease hp etc


//        case WALKING:
//        case JUMPING:
//        case IDLE:
//            dx = (facing ? 1 : -1) * HIT_DX;
//            break;
        default:
            break;
    }

    if (prevStatus != status) {
        switch (status) {
            case HIT1:
                currentAnim = &hit1[facing];
                break;
            case HIT2:
                currentAnim = &hit2[facing];
                break;
            // no case HIT:
                // that is handled during checks
            default:
                break;
        }
        currentAnim -> SetXY (x,y);
        currentAnim -> Rewind ();
        currentAnim -> EnablePlay(true);
    }

    if (thp < 0) thp = 0;

    //prevS = s;
}

void Player::Show () {
    // what about facing?
    currentAnim -> Show ();
}

void Player::Show (float xOff, float yOff) {
    currentAnim -> Show (xOff, yOff);
}

bool Player::CollideWith (Player &enemy) {
    int left1, left2;
    int right1, right2;
    int top1, top2;
    int bottom1, bottom2;

    left1 = currentAnim->GetTX();
    right1 = left1 + currentAnim->w;
    left2 = enemy.currentAnim->GetTX();
    right2 = left2 + enemy.currentAnim->w;

    top1 = currentAnim->GetTY();
    bottom1 = top1 + currentAnim->h;
    top2 = enemy.currentAnim->GetTY();
    bottom2 = top2 + enemy.currentAnim->h;

    if (bottom1 < top2) return(0);
    if (top1 > bottom2) return(0);

    if (right1 < left2) return(0);
    if (left1 > right2) return(0);

    return(1);
}

void Player::PlayGameOverSound(int channel) {
    if (winner) sfx.PlaySample(winSound, channel, -1);
    else sfx.PlaySample(loseSound, channel, -1);
}

// ******************************** The HEART of The GAME *******************************
//
//                                   Here Comes the pain
//
// **************************************************************************************
GameCore::GameCore () {
    LoadControls();
    p0Index = p1Index = 0;
    stageIndex = 0;
    timeLimit = DEFAULT_TIMELIMIT;
    menuMusic = NULL;
    gameMusic = NULL;

    ifstream defFile("game.def");

    defFile >> numImages;           // Images
    defFile >> numSounds;           // sounds
    defFile >> numPlayers;          // players
    defFile >> numMaps;             // maps

    cout << numImages << numSounds << numPlayers << numMaps << endl;

    if (!defFile.good()) {
        cerr << "LoadGameCore: Error opening file: game.def" << endl;
        exit(3);
    }

    vscreen = gfx.LoadImage("gfx/03.png");
    tscreen = gfx.LoadImage("gfx/03.png");

    defFile.close();

    //images.LoadSprites("gfx", numImages, "png");
    images.LoadSprites("gfx", numImages, "png");
    sounds.LoadSounds("sfx", numSounds, "wav");
    menuMusic = sfx.LoadMusic("sfx/menu.mp3");
    //p0.SetControls(controls[0]);
    //p1.SetControls(controls[1]);
    musicPlaying = false;
    ResetData ();
}

GameCore::~GameCore () {
    images.ClearList();
    sounds.ClearList();
    SDL_FreeSurface (vscreen);
    SDL_FreeSurface (tscreen);
    sfx.FreeMusic(menuMusic);
}

void GameCore::ResetData () {
    p0.ResetData();
    p1.ResetData();
    dscroll = tscroll = scroll = 0;
    gameOver = false;
    timeElapsed = 0;
}

/* Controls.def format
(for player 0)
left right up down a b c d

(for player 1)
left right up down a b c d
*/

void GameCore::LoadControls () {
    ifstream controlsFile(CONTROLS_FILE);

    for (int i = 0; i < 2 ; i++ ) {
        controlsFile >> controls[i].left;
        controlsFile >> controls[i].right;
        controlsFile >> controls[i].up;
        controlsFile >> controls[i].down;
        controlsFile >> controls[i].a;
        controlsFile >> controls[i].b;
        controlsFile >> controls[i].c;
        controlsFile >> controls[i].d;

    }
    controlsFile.close();
}

int GameCore::MainMenu () {
    bool* keys = in.GetKeyState();
    int selection = VERSUSMODE;
    // show graphics

    gfx.PutImage(0, 0, images.images[MAINMENU], vscreen);

    JawBite (vscreen);
    if (!musicPlaying) {
        musicPlaying = true;
        sfx.Player(LOAD, -1, menuMusic);
        sfx.Player(PLAY);
    }

    int arrowX = 200, arrowY = 200;
    while (1) {
        in.ProcessEvents();
        if (keys[SDLK_UP]) {
            selection = selection - 1;
            if (selection < VERSUSMODE) selection = EXIT;
            while(keys[SDLK_UP]) in.ProcessEvents();
            sfx.PlaySample(sounds.sounds[SELECTOR]);
        }
        if (keys[SDLK_DOWN]) {
            selection = selection + 1;
            if (selection > EXIT) selection = VERSUSMODE;
            while(keys[SDLK_DOWN]) in.ProcessEvents();
            sfx.PlaySample(sounds.sounds[SELECTOR]);
        }

        gfx.BeginDrawing();
            gfx.PutImage(0, 0, images.images[MAINMENU]);
            switch (selection) {
                case VERSUSMODE:
                    arrowY = 200 + 40; break;
                case INSTRUCTIONS:
                    arrowY = 200 + 80; break;
                case CREDITS:
                    arrowY = 200 + 120; break;
                case EXIT:
                    arrowY = 200 + 160; break;
            }
            gfx.PutImage(arrowX, arrowY, images.images[ARROW]);
        gfx.EndDrawing();

        if (keys[SDLK_ESCAPE]) return EXIT;
        if (keys[SDLK_RETURN]) {
            sfx.PlaySample(sounds.sounds[SELECT]);
            return selection;
        }
    }
    ;
}

MenuResult GameCore::InstructionsMenu () {
    gfx.PutImage(0,0,images.images[INSTRUCTIONSMENU], vscreen);
    JawBite(vscreen);

    bool *keys = in.GetKeyState();
    in.ProcessEvents();
    while (!keys[SDLK_ESCAPE] and !keys[SDLK_RETURN] and !keys[SDLK_SPACE]) in.ProcessEvents();
    sfx.PlaySample(sounds.sounds[SELECT]);
    return OK;
}

MenuResult GameCore::CreditsMenu () {
    gfx.PutImage(0,0,images.images[CREDITSMENU], vscreen);
    JawBite(vscreen);

    bool *keys = in.GetKeyState();
    in.ProcessEvents();
    while (!keys[SDLK_ESCAPE] and !keys[SDLK_RETURN] and !keys[SDLK_SPACE]) in.ProcessEvents();
    sfx.PlaySample(sounds.sounds[SELECT]);
    return OK;
}

MenuResult GameCore::SelectMenu () {
    p0Index = 0;
    p1Index = 1;
    stageIndex = 0;

    gfx.PutImage(0,0,images.images[SELECTBACK], vscreen);
    gfx.PutImage(40, 20,images.images[FULL0], vscreen);
    gfx.PutImage(440, 20,images.images[FULL1], vscreen);
    gfx.PutImage(40, 380,images.images[THUMB0], vscreen);
    gfx.PutImage(120, 380,images.images[THUMB1], vscreen);
    gfx.PutImage(0, 0,images.images[SELECTMASK], vscreen);

    JawBite(vscreen);

    bool* keys = in.GetKeyState();
    SDL_Surface *full;
    SDL_Rect nameBox0 = {40,240,160,40}, nameBox1 = {440,240,160,40}, timeBox = {280,200,80,80};
    int framesLeft = 15 * gfx.GetFramerate();
    int selectX;
    float timeLeft;

    string name;
    tx.SetFColor(255,50,50);
    tx.SetJustify(JUST_CENTER, VJUST_CENTER);

    while (1) {
        in.ProcessEvents();
        if (keys[default_controls[0].left]) {
            p0Index--;
            if (p0Index < 0) p0Index = 1;
            while(keys[default_controls[0].left]) in.ProcessEvents();
            sfx.PlaySample(sounds.sounds[SELECTOR]);
        }
        if (keys[default_controls[0].right]) {
            p0Index++;
            if (p0Index > 1) p0Index = 0;
            while(keys[default_controls[0].right]) in.ProcessEvents();
            sfx.PlaySample(sounds.sounds[SELECTOR]);
        }

        if (keys[default_controls[1].left]) {
            p1Index--;
            if (p1Index < 0) p1Index = 1;
            while(keys[default_controls[1].left]) in.ProcessEvents();
            sfx.PlaySample(sounds.sounds[SELECTOR]);
        }
        if (keys[default_controls[1].right]) {
            p1Index++;
            if (p1Index > 1) p1Index = 0;
            while(keys[default_controls[1].right]) in.ProcessEvents();
            sfx.PlaySample(sounds.sounds[SELECTOR]);
        }

        gfx.BeginDrawing();
        gfx.PutImage(0,0,images.images[SELECTBACK]);
        gfx.PutImage(40, 380, images.images[THUMB0]);
        gfx.PutImage(120, 380, images.images[THUMB1]);

        if (p0Index == 0) { selectX = 40; full = images.images[FULL0]; name = "Shadow Song";}
        else { selectX = 120; full = images.images[FULL1]; name = "Stooping Death";}
        gfx.PutImage(40, 20, full);
        gfx.PutImage(selectX, 380, images.images[P0SELECT]);
        tx.PrintIn(nameBox0, name.c_str());

        if (p1Index == 0) { selectX = 40; full = images.images[FULL0]; name = "Shadow Song";}
        else { selectX = 120; full = images.images[FULL1]; name = "Stooping Death";}
        gfx.PutImage(440, 20, full);
        gfx.PutImage(selectX, 380, images.images[P1SELECT]);
        tx.PrintIn(nameBox1, name.c_str());

        framesLeft--;
        timeLeft = framesLeft / (float) gfx.GetFramerate();
        if (timeLeft < 0) timeLeft = 0;

        stringstream ss;
        ss << setw(2) << setfill('0') << (int) timeLeft;
        string timeStr = ss.str();
        digital.SetJustify(JUST_CENTER, VJUST_CENTER);
        digital.PrintIn(timeBox, timeStr.c_str());

        gfx.PutImage(0,0,images.images[SELECTMASK]);
        gfx.EndDrawing();
        gfx.FramerateDelay();

        if (keys[SDLK_ESCAPE]) {
            sfx.PlaySample(sounds.sounds[SELECT]);
            return CANCEL;
        }
        if (keys[SDLK_RETURN] or timeLeft == 0) {
            sfx.PlaySample(sounds.sounds[DONESELECT]);
            return OK;
        }
    }
}

void GameCore::JawBite (SDL_Surface *final) {
    // the eating phase
    // assuming the jaws are equally wide
    int gape = images.images[LJAW]->w;
    int w = gfx.screen->w;

    for (int i = 0; i <= gape; i+=4) {
        gfx.PutImage (i - (gape - 1), 0, images.images[LJAW]);
        gfx.PutImage (w - i, 0, images.images[RJAW]);
        gfx.EndDrawing();
    }

    sfx.PlaySample(sounds.sounds[JAWBITE]);
    gfx.FramerateDelay();

    // the opening part
    for (int i = gape; i >= 0; i-=4) {
        //gfx.BeginDrawing();
        gfx.PutImage (0,0,final);
        gfx.PutImage (i - (gape - 1), 0, images.images[LJAW]);
        gfx.PutImage (w - i, 0, images.images[RJAW]);
        //gfx.PutImage (i - (gape - 1), 0, globalgfx.ljaw);
        //put_image (w - i, 0, globalgfx.rjaw, screen);
        gfx.EndDrawing();
    }
}


// All the menus done ***********************************************************
void GameCore::LoadElements () {

    musicPlaying = false;
    sfx.Player(STOP);

    gfx.PutImage(0,0,images.images[LOADING]);

        SDL_Surface *full;
        if (p0Index == 0) { full = images.images[FULL0];}
        else {full = images.images[FULL1];}
        gfx.PutImage(40, 140, full);

        if (p1Index == 0) { full = images.images[FULL0];}
        else {full = images.images[FULL1];}
        gfx.PutImage(440, 140, full);

        gfx.EndDrawing();

    p0.LoadPlayer(p0Index);
    p1.LoadPlayer(p1Index);

    stringstream ss;
    string foldername, filename;

    ss.clear(); ss.str("");
    ss << "maps/";
    ss << setw(1) << setfill ('0') << 0;
    foldername = ss.str();
    filename = foldername + "/music.mp3";

    gameMusic = sfx.LoadMusic(filename.c_str());

    stage.Load(0);
    ;
}

void GameCore::FreeElements () {
    sfx.FreeMusic(gameMusic);
    p0.FreePlayer();
    p1.FreePlayer();
    ;
}

void GameCore::PlayGame () {
    //p0.LoadPlayer(0);
    //p0.SetControls(default_controls[0]);

    bool done = false;
    bool* keys = in.GetKeyState();

    p0.PutInPlace(RIGHT);
    p1.PutInPlace(LEFT);

    p0.currentAnim = &p0.entry[p0.facing];
    p1.currentAnim = &p1.entry[p1.facing];
    p0.currentAnim->SetXY(p0.x, p0.y); p0.currentAnim->Rewind(); p0.currentAnim->EnablePlay(true);
    p1.currentAnim->SetXY(p1.x, p1.y); p1.currentAnim->Rewind(); p1.currentAnim->EnablePlay(true);

    // entry animation
    bool p0done = false, p1done = false;

        gfx.PutImage(0, 0, gfx.screen, tscreen);
        ShowBackground ();
        //ShowPlayers ();
        ShowHud ();
        gfx.PutImage (0, 0, gfx.screen, vscreen);
        gfx.PutImage (0, 0, tscreen);
        JawBite (vscreen);

    bool announced = false;
    bool saidReady = false, saidFight = false;
    long count  = 0;
    while (!p0done or !p1done or !announced) {
        //p0.currentAnim->Play();
        //p1.currentAnim->Play();
        MakeAllChecks();

        if (p0.currentAnim->playing == false) {
            p0done = true; p0.currentAnim = &p0.idle[p0.facing];
            p0.currentAnim->Rewind(); p0.currentAnim->EnablePlay(true);
        }
        if (p1.currentAnim->playing == false) {
            p1done = true; p1.currentAnim = &p1.idle[p1.facing];
            p1.currentAnim->Rewind(); p1.currentAnim->EnablePlay(true);
        }

        gfx.BeginDrawing();
            ShowBackground ();
            ShowPlayers ();
            ShowHud ();
            if (p1done and p0done) {
                count++;
                if (count / (float) gfx.GetFramerate() < 2) {
                    gfx.PutImage(195,200,images.images[READY]);
                    if (!saidReady) {
                        saidReady = true;
                        sfx.PlaySample(sounds.sounds[VOICEREADY]);
                            sfx.Player(LOAD, -1, gameMusic);
                            sfx.Player(PLAY);
                    }
                } else
                if (count / (float) gfx.GetFramerate() < 3) {
                    gfx.PutImage(195,200,images.images[FIGHT]);
                    if (!saidFight) {
                        saidFight = true;
                        sfx.PlaySample(sounds.sounds[VOICEFIGHT]);
                    }
                }
                if (count / (float) gfx.GetFramerate() > 3) announced = true;
            }
        gfx.EndDrawing();

        gfx.FramerateDelay();
        //if (keys[SDLK_ESCAPE]) break;
    }

    p0.PutInPlace(RIGHT);
    p1.PutInPlace(LEFT);

    framesElasped = 0; timeElapsed = 0;
    // main game part

    while (!done) {
        in.ProcessEvents();

        MakeAllChecks ();

        gfx.BeginDrawing();
            ShowBackground ();
            ShowPlayers ();
            ShowHud ();
        gfx.EndDrawing();

        gfx.FramerateDelay();
        if (keys[SDLK_ESCAPE]) {
            gfx.PutImage(0,0,images.images[PAUSEOVERLAY]);
            gfx.EndDrawing();
            while(keys[SDLK_ESCAPE]) in.ProcessEvents();

            while(!keys[SDLK_ESCAPE] and !keys[SDLK_q]) in.ProcessEvents();
            if (keys[SDLK_q]) {
                while(keys[SDLK_q]) in.ProcessEvents();
                done = true;
                continue;
            }
            if (keys[SDLK_ESCAPE])
                while(keys[SDLK_ESCAPE]) in.ProcessEvents();
        }

        framesElasped++;
        timeElapsed = framesElasped / gfx.GetFramerate();
        if (timeElapsed > timeLimit) timeElapsed = timeLimit;
        done = GameOverCheck() or keys[SDLK_PAGEUP];
    }
    sfx.Player(STOP);
}

bool GameCore::GameOverCheck () {
    int winner = -1; // -1 = draw
    bool* keys = in.GetKeyState();

    gameOver = true;
    if (p0.hp == 0 or p1.hp == 0) {
        if (p0.hp) winner = 0;
        else if (p1.hp) winner = 1;
        else winner = -1; // draw
    } else if (timeElapsed >= timeLimit) {
        if (p0.hp > p1.hp) winner  = 0;
        else if (p1.hp > p0.hp) winner  = 1;
        else winner = NULL;
    } else gameOver = false;

    if (gameOver) {
        Mix_Music *gameOverMusic = sfx.LoadMusic("sfx/announcer/gameover.mp3");
        SDL_Surface *winnerSplash = gfx.LoadImageAlpha("gfx/winner.png");

        sfx.Player(STOP);
        sfx.PlayMusic(gameOverMusic, 0);

        p0.SetY(GROUND_Y);
        p1.SetY(GROUND_Y);

        if (winner == -1) {
            p0.currentAnim = &p0.lose[p0.facing];
            p1.currentAnim = &p1.lose[p1.facing];
            p0.SetWinner(false);
            p1.SetWinner(false);
        } else if (winner == 0) {
            p0.currentAnim = &p0.win[p0.facing];
            p1.currentAnim = &p1.lose[p1.facing];
            p0.SetWinner(true);
            p1.SetWinner(false);
        } else if (winner == 1) {
            p0.currentAnim = &p0.lose[p0.facing];
            p1.currentAnim = &p1.win[p1.facing];
            p0.SetWinner(false);
            p1.SetWinner(true);
        }

        p0.currentAnim->SetXY(p0.x, p0.y); p0.currentAnim->Rewind(); p0.currentAnim->EnablePlay(true);
        p1.currentAnim->SetXY(p1.x, p1.y); p1.currentAnim->Rewind(); p1.currentAnim->EnablePlay(true);

        // show  winning or losing animation
        bool done = false;
        float x0, x1, y0, y1;
        long count = 0;
        bool saidKO = false;

        p0.PlayGameOverSound(16);
        p1.PlayGameOverSound(15);

        while (!done) {
            x0 = p0.currentAnim->GetX();
            x1 = p1.currentAnim->GetX();

            p0.currentAnim->Play();
            p1.currentAnim->Play();
            y0 = p0.currentAnim->GetY();
            y1 = p1.currentAnim->GetY();

            p0.currentAnim->SetXY(x0,y0);
            p1.currentAnim->SetXY(x1,y1);

            if (abs(x0 - x1) < 200 and count < 40) {
                count++;
                if (x0 < x1) {p0.currentAnim->SetXY(x0 - 1, y0); p1.currentAnim->SetXY(x1 + 1, y1);}
                else {p0.currentAnim->SetXY(x0 + 1, y0); p1.currentAnim->SetXY(x1 - 1, y1);}
            }

            //MakeAllChecks();
            in.ProcessEvents();
            if (keys[SDLK_RETURN]) done = true;

            if (!saidKO) {
                saidKO = true;
                sfx.PlaySample(sounds.sounds[VOICEKO]);
            }

            gfx.BeginDrawing();
                ShowBackground ();
                ShowPlayers ();
                ShowHud ();
                gfx.PutImage(195, 190, images.images[KO]);
                if(winner == 0)
                    gfx.PutImage(50, 80, winnerSplash);
                else if (winner == 1)
                    gfx.PutImage(370, 80, winnerSplash);
                // winner overlay
            gfx.EndDrawing();

            gfx.FramerateDelay();
            //if (keys[SDLK_ESCAPE]) break;
        }

        sfx.StopSample(16);
        sfx.StopSample(15);

        sfx.Player(STOP);
        sfx.FreeMusic(gameOverMusic);
        gfx.FreeImage(winnerSplash);
    }

    return gameOver;
}

void GameCore::MakeAllChecks () {

    static int mp;
    static int framerate = gfx.GetFramerate();

    p0.enemyDistance = p1.enemyDistance = abs(p0.x - p1.x);

    p0.UpdateStatus();
    p1.UpdateStatus();

    mp = (int) (p0.x + p1.x)/2;

    if (mp < 320) tscroll = 0;
    else if (mp > 640 * 3 - 320) tscroll = 640 * 2;
    else tscroll = mp - 320;

    if (p0.x < tscroll + 40) p0.x = tscroll + 40;
    if (p1.x < tscroll + 40) p1.x = tscroll + 40;
    if (p0.x > tscroll + 600) p0.x = tscroll + 600;
    if (p1.x > tscroll + 600) p1.x = tscroll + 600;

    // sync current anim to player coordinate changes
    p0.currentAnim->SetXY(p0.x, p0.y);
    p1.currentAnim->SetXY(p1.x, p1.y);

    // smooth scrolling
    dscroll = (tscroll - scroll) / (framerate / 4) + sign(tscroll - scroll) * .1;
    if (abs(tscroll - scroll) < 1) dscroll = 0;
    scroll += dscroll;


    // p0 p1 interactions
    if (p0.CollideWith(p1) and p0.enemyDistance < 120) {

        Player_Status s0 = p0.status, s1 = p1.status;
        if (p0.enemyDistance < 80) {
            p0.receding = true; p1.receding = true;
        } else { p0.receding = p1.receding = false; }
        //p0.receding = true; p1.receding = true;

        p0.TakeHit (s1);
        p1.TakeHit (s0);

    } else {
        p0.receding = p1.receding = false;
        if (p0.x > p1.x) {
            p0.facing = LEFT;
            p1.facing = RIGHT;
        } else {
            p0.facing = RIGHT;
            p1.facing = LEFT;
        }
    }
}

// ***************************** Show what Chaos really is!!! *******************************

void GameCore::ShowBackground () {
    stage.Show((int) scroll);
}

void GameCore::ShowPlayers () {
    float sx, sy;

    sx = p0.x + (GROUND_Y - p0.y) * 0.984807;  sy = GROUND_Y + (GROUND_Y - p0.y) * 0.17364;
    gfx.PutImage(sx - scroll - 50, sy - 25, images.images[SHADOWOVERLAY]);
    p0.Show(-scroll);

    sx = p1.x + (GROUND_Y - p1.y) * 0.984807;  sy = GROUND_Y + (GROUND_Y - p1.y) * 0.17364;
    gfx.PutImage(sx - scroll - 50, sy - 25, images.images[SHADOWOVERLAY]);
    p1.Show(-scroll);
}

void GameCore::ShowHud () {
    static int x1, w1, x2, w2;
    static SDL_Rect clip;

    // nothing now
    gfx.PutImage(0, 0, images.images[HPBAR_BACK]);

    x1 = (int) ((1 - p0.hp / 100.0) * 250);
    w1 = (int) ((p0.hp / 100.0) * 250);
    x2 = 0;
    w2 = (int) ((p1.hp / 100.0) * 250);


    clip.x = x1; clip.y = 0;
    clip.w = w1; clip.h = 20;
    if (p0.dhp == 0) gfx.PutImage(x1 + 20, 20, images.images[HPBAR], &clip);
    else gfx.PutImage(x1 + 20, 20, images.images[HPBAR_RED], &clip);
    tx.SetJustify(JUST_LEFT, VJUST_TOP);
    tx.MoveTo(0,50);
    tx.PrintLine(p0.name.c_str());

    clip.x = x2; clip.y = 0;
    clip.w = w2; clip.h = 20;
    if (p1.dhp == 0) gfx.PutImage(x2 + 370, 20, images.images[HPBAR], &clip);
    else gfx.PutImage(x2 + 370, 20, images.images[HPBAR_RED], &clip);
    tx.SetJustify(JUST_RIGHT, VJUST_TOP);
    tx.MoveTo(0,50);
    tx.PrintLine(p1.name.c_str());

    static SDL_Rect timeBox = {290, 20, 60, 40};
    stringstream ss;
    ss << setw(2) << setfill('0') << (int) (timeLimit - timeElapsed);
    string timeStr = ss.str();
    digital.SetJustify(JUST_CENTER, VJUST_CENTER);
    digital.PrintIn(timeBox, timeStr.c_str());

    // shownames
}

inline int sign (float x) {
    return x < 0 ? -1 : 1;
}

inline float abs (float x) {
    return x * sign(x);
}
