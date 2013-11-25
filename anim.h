#ifndef ANIM_INCLUDED
#define ANIM_INCLUDED

#include "SDL_graphics.h"
#include "SDL_input.h"
#include "SDL_sfx.h"
#include "textutil.h"
#include <stdio.h>
#include <conio.h>

#define DEFAULT_DDX .01
#define DEFAULT_DDY .01


enum anchorPositions {
    ANCHOR_CENTER = 0, ANCHOR_LD = 1, ANCHOR_LT, ANCHOR_LU, ANCHOR_RD = 4, ANCHOR_RT, ANCHOR_RU,
    ANCHOR_UP = 7, ANCHOR_DN, ANCHOR_DN_AS_LD = 9
};

enum animLoopTypes { LOOP_NONE = 0, LOOP_SIMPLE, LOOP_BACK_FORTH };

// freeing done in destructor, so careful to call destructor only once
// NEVER pass 'list' obects, only pass pointers since destructor will be called in the funciton too.

typedef struct SpriteList {
    int n;
    SDL_Surface** images;

    SpriteList() { n = 0; images = NULL;}
    void ClearList ();
    // careful here
    ~SpriteList() { ClearList(); }
    int LoadSprites(const char* filename);
    // better choice
    // filename = text file containing number of sprites and consecutive filenames
    int LoadSprites(const char* foldername, int numSprites, const char* ext);
    // from 00.ext to XX.ext   XX = numSprites -1
    // ad option for loadimage trans etc
};

typedef struct SoundList {
    int n;
    Mix_Chunk** sounds;
    SoundList() { n = 0; sounds = NULL;}
    void ClearList ();
    // careful here
    ~SoundList() { ClearList(); }
    int LoadSounds(const char* filename);
    // better choice
    // filename = text file containing number of sprites and consecutive filenames
    int LoadSounds(const char* foldername, int numSounds, const char* ext);
};


typedef struct Frame {
    SDL_Surface* image;
    Mix_Chunk* sound;
    int x, y;   // relative x and y w.r.t the animation's original x and y
                // how much to move ahead when the frame begins (not when the frame is done);
    int w, h;
    float dx, dy; // for motion during waiting
    // could use a pointer to function for special movement options
    int wait;

    Frame() {
        dx = dy = wait = x = y = 0;
        image = NULL;
    }
    ~Frame() {
        ;
    }
};


//**************************************************************************************************
// the class that implements a series of frames as an animation
class Animation {
  private:
    int numFrames;
    Frame* frames;

    int currentFrame, toWait;
    int loopsMade;

    // something to store the values of when the anim was created
    float x, y;
    float dx, dy;
    float ddx, ddy; // ?



    bool friction, gravity;
    // smooth stopping, put in player class?

  public:
    int anchor;
    int w, h;

    bool loop;
    int playing; // to know if its playing, to set visibility
    bool frameJustStarted;
    // maybe setalpha to the whole anim
    // maybe something to implementpalette functions


    Animation();
    Animation (SpriteList* gList, SoundList* sList, const char* filename);
    ~Animation();

    void Load (SpriteList* gList, SoundList* sList, const char* filename);
    void LoadMirror (SpriteList* gList, SoundList* sList, const char* filename);
    // to load from adf file

    void ResetData ();

    inline void SetXY (float gX, float gY) { x = gX; y = gY; }
    inline void SetDXDY (float gX, float gY) { dx = gX; dy = gY; }
    inline float GetX () { return x; }
    inline float GetY () { return y; }
    float GetTX ();
    float GetTY ();
    inline float GetDX () { return dx; }
    inline float GetDY () { return dy; }
    inline int GetAnchor () { return anchor; }
    inline void SetAnchor (int gAnchor) { anchor = gAnchor; }

    // something for the animation to drag player coordinates

    inline void EnableLoop (bool gLoop) {loop = gLoop;}
    inline bool EnableLoop () {return loop;}
    inline void EnableFriction (bool gFriction) {friction = gFriction;}
    inline void SetDDX (float gDdx) {ddx = gDdx;}
    inline void SetDDY (float gDdy) {ddy = gDdy;}

    void Rewind();
    void Play();
    void Stop () { Rewind(); playing = 0; }
    void EnablePlay (bool gPlaying) { playing = gPlaying; }
    void GotoFrame (unsigned int frameNo);
    void Show();
    void Show (float xOff, float yOff = 0);

    bool SimpleCollision (Animation *a);
};

/*  To Play an animation
 #  Rewind() takes to the first frame
 #  EnablePlay(1) sets palying to 1
 #  Play() advances frames if playing == 1
 #  Play() never stops (i.e. playing = 1 doesn't change)
    if loop = 1
*/

/*
**************************************************************
Now load animation from .adf files
Animation Definition File
format:

BOF
<int numframes>
<anchor>
<imageno> <x> <y> <dx> <dy> <wait> <soundno>
<imageno> <x> <y> <dx> <dy> <wait> <soundno>
etc...
EOF

meaning of <dx> <dy>
<dx> pixels covered in wait/100 seconds

**************************************************************
*/

class BackGround {
    SDL_Surface* back;
    SDL_Surface* fore[3];
    // constant for now
    public:
    BackGround ();
    ~BackGround ();
    void ClearImages ();
    int Load(int mapNo);
    void Show(int scroll = 0);
};
// can be loaded from a folder
// expects back.bmp, 0.bmp, 1.bmp, 2.bmp

#endif // ANIM_INCLUDED
