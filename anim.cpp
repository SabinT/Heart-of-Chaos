#include "anim.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <iostream>

extern SDL_Graphics gfx;
extern SDL_Sfx sfx;
extern SDL_Input in;
extern SDL_Surface* blob;

//using std::cout;
//using std::cerr;
//using std::endl;
//using std::string;
//using std::stringstream;
//using std::setw;
//using std::setfill;
using namespace std;

// *********************** Sprilelists Deal ********************************
void  SpriteList::ClearList () {
    if (images == NULL) return;

    for (int i = 0; i < n; i++)
        if (images[i]) SDL_FreeSurface(images[i]);

    delete[] images;
    images = NULL;
    n = 0;

    cout << "SpriteList Cleared.\n" << endl;
}

int SpriteList::LoadSprites(const char* foldername, int numSprites, const char* ext) {
    n = numSprites;
    images = new SDL_Surface* [n];

    stringstream ss;
    string filename;

    bool error = 0;

    for (int i = 0; i < n; i++ ) {
        ss.clear();
        ss.str("");
        ss << foldername << '/';
        ss << setw(2) << setfill ('0') << i << '.' << ext;
        filename = ss.str();

        images[i] = gfx.LoadImageAlpha(filename.c_str());
        if (images[i] == NULL) {
            cerr << "LoadSprites: " << gfx.GetError() << ", assigning NULL image..." << endl;
            images[i] = gfx.LoadImage("gfx/null.bmp");
            error = 1;
        }
        // needs option for alpha or trans or none according to file extension
    }

    if (error) return -1;
    return 0;
}

// *********************** Soundlists Deal ********************************
void  SoundList::ClearList () {
    if (sounds == NULL) return;

    for (int i = 0; i < n; i++)
        if (sounds[i]) sfx.FreeSample(sounds[i]);

    delete[] sounds;
    sounds = NULL;
    n = 0;

    cout << "SoundList Cleared.\n";
}

int SoundList::LoadSounds(const char* foldername, int numSounds, const char* ext) {
    n = numSounds;
    sounds = new Mix_Chunk*[n];

    stringstream ss;
    string filename;

    bool error = 0;

    for (int i = 0; i < n; i++ ) {
        ss.clear();
        ss.str("");
        ss << foldername << '/';
        ss << setw(2) << setfill ('0') << i << '.' << ext;
        filename = ss.str();

        sounds[i] = sfx.LoadSample(filename.c_str());
        if (sounds[i] == NULL) {
            cerr << "LoadSounds: " << sfx.GetError() << endl;
            error = 1;
        }
        // needs option for alpha or trans or none according to file extension
    }

    if (error) return -1;
    return 0;
}

// ************************ Now the animation part ********************************
void Animation::ResetData () {
    loopsMade = 0;
    numFrames = 0;
    frames = NULL;
    currentFrame = toWait = 0;

    x = y = dx = dy = ddx = ddy = 0;
    ddx = DEFAULT_DDX;
    ddy = DEFAULT_DDY;

    anchor = ANCHOR_DN;

    friction = gravity = true;
    loop = 0;
    playing = 0; // to know if its playing, to set visibility
}

Animation::Animation () {
    ResetData ();
}

Animation::Animation(SpriteList* gList, SoundList* sList, const char* filename) {
    ResetData();
    Load(gList, sList, filename);
}

// load animation from .adf file
void Animation::Load (SpriteList* gList, SoundList* sList, const char* filename) {
    ResetData();

    ifstream animFile(filename);
    if (!animFile.good()) {
        cerr << "Animation::Load: Error opening file " << filename << endl;
        exit(3);
    }

    if (gList == NULL) {
        cerr << "Animation::Load: Spritelist is empty." << endl;
        exit(3);
    }
    if (sList == NULL) {
        cerr << "Animation::Load: SoundList is empty." << endl;
        //exit(3);
    }

    animFile >> numFrames;
    animFile >> anchor;

    frames = new Frame[numFrames];
    int imageNo, soundNo;
    int tdx, tdy;
    for (int i = 0; i < numFrames; i++) {
        animFile >> imageNo;
        animFile >> frames[i].x;
        animFile >> frames[i].y;
        animFile >> tdx;
        animFile >> tdy;
        animFile >> frames[i].wait;
        animFile >> soundNo;

        frames[i].dx = tdx / ((frames[i].wait / 100.0) * gfx.GetFramerate());
        frames[i].dy = tdy / ((frames[i].wait / 100.0) * gfx.GetFramerate());

        if (!animFile.good()) {
            cerr << "Animation::Load: Error reading file " << filename << endl;
            animFile.close();
            exit(3);
        }

        frames[i].image = gList->images[imageNo];
        frames[i].w = frames[i].image->w;
        frames[i].h = frames[i].image->h;
        if (soundNo and sList != NULL)
            frames[i].sound = sList->sounds[soundNo - 1];
        else
            frames[i].sound = NULL;

        if (anchor == ANCHOR_DN_AS_LD and i != 0) {
            int w2 = frames[i].w, w1 = frames[i-1].w;
            frames[i].x = frames[i].x / 2 + (w2 - w1) / 2;
            // frames[i].dx = frames[i].dx / 2 + (w2 - w1) / 2;
        }
    }

    if (anchor == ANCHOR_DN_AS_LD) anchor = ANCHOR_DN;
    animFile.close();

    Rewind();
    // make it ready to play
}

void Animation::LoadMirror (SpriteList* gList, SoundList* sList, const char* filename) {
    Load (gList, sList, filename);

    for (int i = 0; i < numFrames; i++) {
        frames[i].x *= -1;
        frames[i].dx *= -1;
    }
}

Animation::~Animation() {
    delete[] frames;
    cout << "Frames of Animaoyition deleted.\n";
}

void Animation::Rewind() {
    currentFrame = 0;
    toWait = (int) ((frames[currentFrame].wait / 100.0) * gfx.GetFramerate());
    frameJustStarted = true;
    //playing = 1;
}

void Animation::GotoFrame(unsigned int frameNo) {
    if (frameNo > numFrames) currentFrame = 0;
    else currentFrame = frameNo;
}

void Animation::Play () {
// Play = advance, not show the pictures
    if (!playing) return;
    //if (!playing) playing = 1;

    toWait--;
    if (toWait == 0) {
        loopsMade++;
        if (currentFrame == numFrames -1) { // last frame
            if (loop) { Rewind(); }
            else playing = 0;
        } else {
            frameJustStarted = true;
            currentFrame++;
            toWait = (int) ((frames[currentFrame].wait / 100.0) * gfx.GetFramerate());
            // adjust to current framerate, in Rewiind() too
        }
    }

    if (frameJustStarted) {
        // beginning of frame so update x and y
        x += frames[currentFrame].x;
        y += frames[currentFrame].y;
        dx = frames[currentFrame].dx;
        dy = frames[currentFrame].dy;
        sfx.PlaySample(frames[currentFrame].sound);
        frameJustStarted = false;
    }

    x += frames[currentFrame].dx;
    y += frames[currentFrame].dy;

    w = frames[currentFrame].w;
    h = frames[currentFrame].h;

// gravity pretty extravagant for a few frames

}

void Animation::Show () {
     gfx.PutImage((int)GetTX(), (int)GetTY(), frames[currentFrame].image);
}


void Animation::Show (float xOff, float yOff) {
    gfx.PutImage((int)(GetTX() + xOff), (int)(GetTY() + yOff), frames[currentFrame].image);

}


float Animation::GetTX () {
    static float tx;

    switch (anchor) {
    case ANCHOR_LD:         // left down
        tx = x;
        break;
    case ANCHOR_LT:         // left center
        tx = x;
        break;
    case ANCHOR_LU:         // left up
        tx = x;
        break;
    case ANCHOR_RD:         // RIGHT DOWN
        tx = x - frames[currentFrame].image->w;
        break;
    case ANCHOR_RT:         // RIGHT CENTER
        tx = x - frames[currentFrame].image->w;
        break;
    case ANCHOR_RU:         // RIGHT UP
        tx = x - frames[currentFrame].image->w;
        break;
    case ANCHOR_UP:         // UP
        tx = x - frames[currentFrame].image->w;
        break;
    case ANCHOR_DN:         // DOWN
        tx = x - frames[currentFrame].image->w / 2;
        break;
    case ANCHOR_CENTER:     //CENTER
        tx = x - frames[currentFrame].image->w / 2;
        break;
    }
    return tx;
}

float Animation::GetTY () {
    static float ty;

    switch (anchor) {
    case ANCHOR_LD:         // left down
        ty = y - frames[currentFrame].image->h;
        break;
    case ANCHOR_LT:         // left center
        ty = y - frames[currentFrame].image->h / 2;
        break;
    case ANCHOR_LU:         // left up
        ty = y;
        break;
    case ANCHOR_RD:         // RIGHT DOWN
        ty = y - frames[currentFrame].image->h;
        break;
    case ANCHOR_RT:         // RIGHT CENTER
        ty = y - frames[currentFrame].image->h / 2;
        break;
    case ANCHOR_RU:         // RIGHT UP
        ty = y - frames[currentFrame].image->h;
        break;
    case ANCHOR_UP:         // UP
        ty = y;
        break;
    case ANCHOR_DN:         // DOWN
        ty = y - frames[currentFrame].image->h;
        break;
    case ANCHOR_CENTER:     //CENTER
        ty = y;
        break;
    }

    return ty;
}

bool Animation::SimpleCollision (Animation *a) {
    int left1, left2;
    int right1, right2;
    int top1, top2;
    int bottom1, bottom2;

    left1 = GetTX();
    left2 = a->GetTX();
    right1 = GetTX() + frames[currentFrame].w;
    right2 = a->GetTX() + a->frames[currentFrame].w;
    top1 = GetTY();
    top2 = a->GetTY();
    bottom1 = GetTY() + frames[currentFrame].h;
    bottom2 = a->GetTY(); + a->frames[currentFrame].h;

    if (bottom1 < top2) return(0);
    if (top1 > bottom2) return(0);

    if (right1 < left2) return(0);
    if (left1 > right2) return(0);

    return(1);
}

// Now the background party.... **********************************************
int BackGround::Load(int mapNo) {
    stringstream ss;
    string filename;
    string foldername;

    ss.clear(); ss.str("");
    ss << "maps/";
    ss << setw(1) << setfill ('0') << mapNo;
    foldername = ss.str();

    bool error = 0;

    for (int i = 0; i < 3; i++ ) {
        ss.clear();
        ss.str("");
        ss << foldername << '/';
        ss << setw(1) << setfill ('0') << i << ".png";
        filename = ss.str();

        fore[i] = gfx.LoadImageAlpha( filename.c_str());
        if (fore[i] == NULL) {
            cerr << "Background::Load() : " << gfx.GetError() << ", assigning NULL image..." << endl;
            fore[i] = gfx.LoadImage("gfx/null.bmp");
            error = 1;
        }
        // needs option for alpha or trans or none according to file extension
    }

    filename = foldername + "/back.png";
    back = gfx.LoadImage( filename.c_str());
    if (back == NULL) {
        cerr << "Background::Load() : " << gfx.GetError() << ", assigning NULL image..." << endl;
        back = gfx.LoadImage("gfx/null.bmp");
        error = 1;
    }

    if (error) return -1;
    return 0;
}

void BackGround::Show (int scroll) {
    // actual scroll is float, converted to int when passing here
    static int lt, rt, ltx, rtx;

    gfx.PutImage(0,0,back);
    if (scroll < 0) {
        lt = 0; rt = -1; // rt = -1 means only lt displayed
    } else if (scroll % 640 == 0) {
        // only one image needs to be diaplayed
        lt = scroll / 640;
        rt = -1;
    }
    else if (scroll > 2 * 640) {
        lt = -1; rt = 2;
        // only last part shown
    } else {
        lt = scroll / 640;
        rt = lt + 1;
    }

    lt = scroll / 640;
    rt = lt + 1;

    if (lt != -1) {
        ltx = - (scroll % 640);
        gfx.PutImage(ltx, 0, fore[lt]);
    }
    if (rt != -1) {
        rtx = 640 - (scroll % 640);
        gfx.PutImage(rtx, 0, fore[rt]);
    }
}

BackGround::BackGround () {
    back = NULL;
    for (int i = 0; i < 3; i++) fore[i] = NULL;
}

BackGround::~BackGround () {
    ClearImages();
}

void BackGround::ClearImages () {
}
