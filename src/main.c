#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_W      1280
#define SCREEN_H      720
#define MAP_W         20
#define MAP_H         20
#define CELL_SIZE     64
#define NUM_RAYS      SCREEN_W
#define FOV_DEG       66.0f
#define MAX_DEPTH     22
#define MAX_HEALTH    100
#define MAX_SANITY    100
#define MAX_SPRITES   48
#define MAX_DOORS     32
#define NUM_LEVELS    4
#define DOOR_SPD      3.0f
#define TEX_SZ        64

#define C_EMPTY   0
#define C_WALL1   1   
#define C_WALL2   2   
#define C_WALL3   3   
#define C_DOOR    4  
#define C_EXIT    5  
#define C_WATER   6   
#define S_NONE     0
#define S_ALMOND   1   
#define S_HEALTH   2   
#define S_SMILER   3   
#define S_HOUND    4   
#define S_CLUMP    5  
#define S_NOTE     6   
#define S_LAMP     7   

#define NTEX  12

static int has_key;
typedef struct {
    float x, y;
    int   type;
    int   active;
    float anim;       
    float speed;      
    int   health;     
    float aggro_dist; 
    int   lore_id;    
} Sprite;

typedef struct {
    int   mx, my;
    float open;
    int   opening;
} Door;

typedef struct {
    int   map[MAP_H][MAP_W];
    float spawn_x, spawn_y, spawn_angle;
    float ceil_top[3], ceil_bot[3];
    float floor_top[3], floor_bot[3];
    float fog[3];
    float fog_dist;     
    int   flicker;      
    float flicker_freq;
    const char *name;
    const char *desc[4];
    int tex_wall1, tex_wall2, tex_wall3, tex_door, tex_exit;
    const char *ambiance;
    float spr_data[MAX_SPRITES][3]; 
    int   spr_count;
} Level;


static const int MAP0[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,1,0,0,1,0,1,1,0,1,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1,1,1,0,0,0,0,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,1,0,1,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,0,0,0,0,1,1,0,1,1,0,1},
    {1,0,1,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0,1},
    {1,0,0,0,1,0,0,0,1,1,0,1,1,0,0,0,0,0,4,1},
    {1,1,1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,1},
    {1,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,0,0,0,1},
    {1,0,1,1,0,1,1,0,1,0,1,0,1,0,0,0,1,1,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1},
    {1,1,0,1,0,1,1,0,1,1,1,1,0,1,1,0,1,1,0,1},
    {1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1,0,1,0,0,0,1,0,1,1,0,1,1},
    {1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,1,0,1},
    {1,0,1,0,0,0,1,0,1,1,0,1,1,0,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static const int MAP1[MAP_H][MAP_W] = {
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
    {2,0,0,0,0,0,2,0,0,0,0,0,0,2,0,0,0,0,0,2},
    {2,0,2,0,2,0,2,0,2,2,0,2,2,0,2,0,2,0,2,2},
    {2,0,2,0,0,0,0,0,2,0,0,0,2,0,2,0,0,0,0,2},
    {2,0,0,0,2,2,2,0,2,0,2,0,2,0,0,0,2,2,0,2},
    {2,0,2,0,2,0,0,0,0,0,2,0,0,0,2,0,2,0,0,2},
    {2,0,2,0,2,0,2,2,2,0,2,0,2,2,2,0,2,0,2,2},
    {2,0,0,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,4,2},
    {2,2,2,0,2,0,2,0,2,0,0,0,2,0,2,0,2,2,0,2},
    {2,0,0,0,2,0,0,0,2,0,2,0,2,0,0,0,2,0,0,2},
    {2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,2},
    {2,0,2,0,0,0,2,0,0,0,0,0,0,0,2,0,0,0,0,2},
    {2,0,2,2,2,0,2,2,0,2,2,2,2,0,2,2,0,2,0,2},
    {2,0,0,0,2,0,0,0,0,0,0,2,0,0,0,0,0,2,0,2},
    {2,0,2,0,2,0,2,0,2,2,0,0,0,2,0,2,2,0,0,2},
    {2,0,2,0,0,0,2,0,2,0,0,2,0,2,0,0,2,0,0,2},
    {2,0,0,0,2,0,0,0,2,0,2,2,0,0,0,2,0,2,0,2},
    {2,0,2,0,0,0,2,0,0,0,2,0,0,2,0,0,0,2,0,2},
    {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,2},
    {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2}
};

static const int MAP2[MAP_H][MAP_W] = {
    {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
    {3,0,0,0,3,0,0,0,0,0,3,0,0,0,3,0,0,0,0,3},
    {3,0,3,0,3,0,3,3,0,3,3,3,0,3,3,0,3,3,0,3},
    {3,0,3,0,0,0,3,0,0,0,0,3,0,0,3,0,3,0,0,3},
    {3,0,3,3,3,0,3,0,3,3,0,3,0,3,3,0,3,0,3,3},
    {3,0,0,0,3,0,0,0,3,0,0,0,0,3,0,0,0,0,3,3},
    {3,3,3,0,3,0,3,0,3,0,3,3,0,3,0,3,3,0,3,3},
    {3,0,3,0,0,0,3,0,0,0,3,0,0,3,0,3,0,0,4,3},
    {3,0,3,3,3,3,3,3,3,3,0,3,0,3,0,3,0,3,0,3},
    {3,0,0,0,0,0,0,0,0,3,0,3,0,0,0,3,0,3,0,3},
    {3,0,3,3,3,3,3,3,0,3,0,3,3,3,0,3,0,0,0,3},
    {3,0,3,0,0,0,0,3,0,3,0,0,0,3,0,3,3,3,0,3},
    {3,3,3,0,3,3,0,3,0,3,3,3,0,3,0,0,0,3,0,3},
    {3,0,0,0,3,0,0,0,0,0,0,3,0,0,0,3,0,0,0,3},
    {3,0,3,0,3,0,3,0,3,3,0,0,0,3,0,3,3,0,3,3},
    {3,0,3,0,0,0,3,0,3,0,0,3,0,3,0,0,3,0,0,3},
    {3,0,0,0,3,0,0,0,3,0,3,3,0,0,0,3,0,3,0,3},
    {3,0,3,0,0,0,3,0,0,0,3,0,0,3,0,0,0,3,0,3},
    {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,3},
    {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3}
};

static const int MAP3[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,0,0,0,0,1,0,1,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,0,1},
    {1,0,0,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,1,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,1},
    {1,0,0,0,1,0,1,0,0,0,0,0,0,1,0,1,0,0,4,1},
    {1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,1,0,1,0,1,0,1,0,1,1,0,1,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1},
    {1,1,0,1,0,1,1,0,1,1,1,1,0,1,1,0,1,1,0,1},
    {1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1,0,1,0,0,0,1,0,1,1,0,1,1},
    {1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,1,0,1},
    {1,0,1,0,0,0,1,0,1,1,0,1,1,0,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static int   current_level = 0;
static float px_pos, py_pos;
static float pa;
static float pdx, pdy;

static int   health    = MAX_HEALTH;
static int   sanity    = MAX_SANITY;  
static int   score     = 0;
static int   ammo      = 20;
static int   shoot_anim= 0;
static int   hurt_anim = 0;
static int   scare_anim= 0;  
static int   win_state = 0;  
static int   game_started = 0;

static int   transition_timer = 0;
#define TRANS_DURATION 120
static float flicker_val  = 1.0f;
static float flicker_timer= 0.0f;

static float total_time = 0.0f;

static unsigned char tex[NTEX][TEX_SZ][TEX_SZ][3];

static float z_buffer[SCREEN_W];

static Sprite sprites[MAX_SPRITES];
static int    num_sprites = 0;

static Door   doors[MAX_DOORS];
static int    num_doors = 0;

static int map_cur[MAP_H][MAP_W];

static Level levels[NUM_LEVELS];

static struct { int w,s,a,d,left,right; } keys_st;

static int   last_time  = 0;
static float dt         = 0.016f;
static int   fcount     = 0;
static float fps_val    = 60.0f;
static float fps_timer2 = 0.0f;

static const char *lore_notes[8] = {
    "Note froissee : 'Le bruit des neons ne s'arrete jamais.\n Il parait que si tu les entends clignoter,\n quelque chose t'observe...'",
    "Graffiti : 'NE REGARDE PAS DERRIERE TOI.\n Trop tard pour moi. - R'",
    "Manuel dechire : 'Niveau 0 — Proprietes : Moquette jaune,\n murs creme, lumiere fluorescente.\n Superficie infinie. Aucune sortie connue.'",
    "Post-it : 'L'Almond Water calme les entites.\n Boire avant d'entrer dans une zone sombre.'",
    "Journal p.47 : 'Jour 12. J'ai entendu le Hound\n de nouveau cette nuit. Il court dans\n les couloirs. Il ne s'arrete jamais.'",
    "Avertissement : 'NIVEAU 2 DANGEREUX.\n Les tuyaux chauffent. Les entites\n ne voient pas mais entendent tout.'",
    "Inscription murale : 'Qui a construit les Poolrooms ?\n L eau ne coule nulle part.\n Elle est juste... la.'",
    "Message final : 'Si tu lis ceci tu as presque reussi.\n Niveau 4 = sortie REELLE. Bonne chance.\n Tu en auras besoin. — Survivant'"
};

static float fix_ang(float a)
{
    while(a <  0.0f)         a += 2.0f*(float)M_PI;
    while(a >= 2.0f*(float)M_PI) a -= 2.0f*(float)M_PI;
    return a;
}
static float vdist(float ax,float ay,float bx,float by)
{ return sqrtf((bx-ax)*(bx-ax)+(by-ay)*(by-ay)); }

static float hash(int x, int y, int seed)
{
    unsigned int h = (unsigned int)(x*374761 + y*668265 + seed*2246822);
    h ^= h>>15; h *= 2246822519u; h ^= h>>13; h *= 3266489917u; h ^= h>>16;
    return (float)(h & 0xFFFF) / 65535.0f;
}

static void gen_tex(void)
{
    int x,y;
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        float n = hash(x,y,0)*0.12f;
        int chev = ((x+y)%8 < 4);
        unsigned char r = (unsigned char)((chev?0.82f:0.75f+n)*255);
        unsigned char g = (unsigned char)((chev?0.72f:0.65f+n)*255);
        unsigned char b = (unsigned char)((chev?0.18f:0.14f+n)*255);
        tex[0][y][x][0]=r; tex[0][y][x][1]=g; tex[0][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        float n = hash(x,y,1)*0.08f;
        int seam = (x%16==0)||(y%16==0);
        unsigned char r,g,b;
        if(seam){r=180;g=170;b=150;}
        else{ r=(unsigned char)((0.90f+n)*255); g=(unsigned char)((0.86f+n)*255); b=(unsigned char)((0.74f+n)*255);}
        tex[1][y][x][0]=r; tex[1][y][x][1]=g; tex[1][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        float n  = hash(x,y,2)*0.15f;
        float wet= (hash(x,y,42)>0.85f)?0.85f:1.0f;
        int crack= (hash(x*3,y,5)>0.92f && hash(x,y*3,6)>0.90f);
        unsigned char r,g,b;
        if(crack){r=40;g=40;b=38;}
        else{ r=(unsigned char)((0.38f+n)*wet*255); g=(unsigned char)((0.36f+n)*wet*255); b=(unsigned char)((0.34f+n)*wet*255);}
        tex[2][y][x][0]=r; tex[2][y][x][1]=g; tex[2][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int pipe = (y%12 < 7);
        int joint= (x%20==0);
        float n  = hash(x,y,3)*0.12f;
        float rust= hash(x+y*3,y+x*2,7)*0.3f;
        unsigned char r,g,b;
        if(joint&&pipe){r=60;g=50;b=40;}
        else if(pipe){ r=(unsigned char)(fminf(1.0f,(0.45f+rust+n))*255); g=(unsigned char)(fminf(1.0f,(0.25f+rust*0.4f+n))*255); b=(unsigned char)((0.10f+n)*255);}
        else{ r=(unsigned char)((0.20f+n)*255); g=(unsigned char)((0.18f+n)*255); b=(unsigned char)((0.16f+n)*255);}
        tex[3][y][x][0]=r; tex[3][y][x][1]=g; tex[3][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int grout = (x%16==0)||(y%16==0);
        float n   = hash(x,y,4)*0.05f;
        float damp= hash(x/4,y/4,8)*0.15f;
        unsigned char r,g,b;
        if(grout){r=160;g=165;b=170;}
        else{ r=(unsigned char)((0.88f+n-damp)*255); g=(unsigned char)((0.90f+n-damp*0.8f)*255); b=(unsigned char)((0.92f+n)*255);}
        tex[4][y][x][0]=r; tex[4][y][x][1]=g; tex[4][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int panel = (x/16)&1;
        int edge  = (x%16<2)||(y%16<2);
        float n   = hash(x,y,9)*0.1f;
        unsigned char r,g,b;
        if(edge){r=40;g=35;b=30;}
        else if(panel){r=(unsigned char)((0.35f+n)*255);g=(unsigned char)((0.28f+n)*255);b=(unsigned char)((0.20f+n)*255);}
        else{r=(unsigned char)((0.30f+n)*255);g=(unsigned char)((0.24f+n)*255);b=(unsigned char)((0.18f+n)*255);}
        tex[5][y][x][0]=r; tex[5][y][x][1]=g; tex[5][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int bord = (x<3||x>=TEX_SZ-3||y<3||y>=TEX_SZ-3);
        int arrow= (abs(x-TEX_SZ/2)+abs(y-TEX_SZ/2)<14);
        unsigned char r,g,b;
        if(bord){r=200;g=80;b=10;}
        else if(arrow){r=255;g=220;b=0;}
        else{r=40;g=30;b=20;}
        tex[6][y][x][0]=r; tex[6][y][x][1]=g; tex[6][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        float n   = hash(x,y,10)*0.10f;
        int crack = (hash(x*2+1,y,11)>0.88f)||(hash(x,y*2+1,12)>0.90f);
        unsigned char r,g,b;
        if(crack){r=18;g=16;b=14;}
        else{r=(unsigned char)((0.22f+n)*255);g=(unsigned char)((0.20f+n)*255);b=(unsigned char)((0.18f+n)*255);}
        tex[7][y][x][0]=r; tex[7][y][x][1]=g; tex[7][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int cable = (x%10<2)&&(hash(x/10,y,13)>0.5f);
        int bolt  = ((x%20<3)&&(y%20<3));
        float n   = hash(x,y,14)*0.08f;
        unsigned char r,g,b;
        if(bolt){r=180;g=170;b=140;}
        else if(cable){r=15;g=15;b=15;}
        else{r=(unsigned char)((0.18f+n)*255);g=(unsigned char)((0.17f+n)*255);b=(unsigned char)((0.15f+n)*255);}
        tex[8][y][x][0]=r; tex[8][y][x][1]=g; tex[8][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        float ripple = sinf((x+y)*0.4f)*0.08f + hash(x,y,15)*0.06f;
        unsigned char r=(unsigned char)((0.30f+ripple)*255);
        unsigned char g=(unsigned char)((0.55f+ripple)*255);
        unsigned char b=(unsigned char)((0.72f+ripple)*255);
        tex[9][y][x][0]=r; tex[9][y][x][1]=g; tex[9][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int grout = (x%16==0)||(y%16==0);
        float wet  = 0.7f + hash(x/8,y/8,16)*0.2f;
        float n    = hash(x,y,17)*0.04f;
        unsigned char r,g,b;
        if(grout){r=100;g=120;b=130;}
        else{r=(unsigned char)((0.70f+n)*wet*255);g=(unsigned char)((0.80f+n)*wet*255);b=(unsigned char)((0.85f+n)*wet*255);}
        tex[10][y][x][0]=r; tex[10][y][x][1]=g; tex[10][y][x][2]=b;
    }
    for(y=0;y<TEX_SZ;y++) for(x=0;x<TEX_SZ;x++){
        int neon = (y%22 < 3);
        int seam = (x%16==0)||(y%16==0);
        float n  = hash(x,y,18)*0.06f;
        unsigned char r,g,b;
        if(neon&&!seam){r=(unsigned char)((0.85f+n)*200); g=(unsigned char)((0.85f+n)*200); b=(unsigned char)((0.60f+n)*180);}
        else if(seam){r=150;g=140;b=120;}
        else{r=(unsigned char)((0.80f+n)*245);g=(unsigned char)((0.78f+n)*240);b=(unsigned char)((0.68f+n)*210);}
        tex[11][y][x][0]=r; tex[11][y][x][1]=g; tex[11][y][x][2]=b;
    }
}

static void init_levels(void)
{
    {
        Level *L = &levels[0];
        memcpy(L->map, MAP0, sizeof(MAP0));
        L->spawn_x=1.5f*CELL_SIZE; L->spawn_y=1.5f*CELL_SIZE; L->spawn_angle=0.0f;
        L->ceil_top[0]=0.82f;  L->ceil_top[1]=0.80f;  L->ceil_top[2]=0.68f;
        L->ceil_bot[0]=0.75f;  L->ceil_bot[1]=0.73f;  L->ceil_bot[2]=0.60f;
        L->floor_top[0]=0.48f; L->floor_top[1]=0.40f; L->floor_top[2]=0.12f;
        L->floor_bot[0]=0.25f; L->floor_bot[1]=0.20f; L->floor_bot[2]=0.06f;
        L->fog[0]=0.72f; L->fog[1]=0.68f; L->fog[2]=0.48f; L->fog_dist=900.0f;
        L->flicker=1; L->flicker_freq=0.8f;
        L->tex_wall1=1; L->tex_wall2=11; L->tex_wall3=1;
        L->tex_door=5; L->tex_exit=6;
        L->name="NIVEAU 0 — The Lobby";
        L->desc[0]="Moquette jaune. Neons qui clignotent.";
        L->desc[1]="L'odeur de moisissure est partout.";
        L->desc[2]="Tu as no-clippé hors de la réalité.";
        L->desc[3]="Evite les Smilers. Trouve la sortie.";
        L->ambiance="~~ bruitages de neons ~~";
        float sd[][3]={
            {3,3,S_NOTE},{7,5,S_SMILER},{12,4,S_ALMOND},
            {5,9,S_SMILER},{10,10,S_NOTE},{15,7,S_HEALTH},
            {8,14,S_SMILER},{16,12,S_ALMOND},{3,16,S_LAMP},
            {14,16,S_NOTE},{17,3,S_SMILER}
        };
        L->spr_count=(int)(sizeof(sd)/sizeof(sd[0]));
        memcpy(L->spr_data,sd,sizeof(sd));
        L->spr_data[2][2]=S_ALMOND;
    }
    {
        Level *L = &levels[1];
        memcpy(L->map, MAP1, sizeof(MAP1));
        L->spawn_x=1.5f*CELL_SIZE; L->spawn_y=1.5f*CELL_SIZE; L->spawn_angle=0.0f;
        L->ceil_top[0]=0.14f; L->ceil_top[1]=0.13f; L->ceil_top[2]=0.12f;
        L->ceil_bot[0]=0.20f; L->ceil_bot[1]=0.18f; L->ceil_bot[2]=0.16f;
        L->floor_top[0]=0.22f; L->floor_top[1]=0.20f; L->floor_top[2]=0.18f;
        L->floor_bot[0]=0.08f; L->floor_bot[1]=0.07f; L->floor_bot[2]=0.06f;
        L->fog[0]=0.15f; L->fog[1]=0.14f; L->fog[2]=0.12f; L->fog_dist=700.0f;
        L->flicker=0; L->flicker_freq=0.0f;
        L->tex_wall1=2; L->tex_wall2=7; L->tex_wall3=2;
        L->tex_door=5; L->tex_exit=6;
        L->name="NIVEAU 1 — The Habitable Zone";
        L->desc[0]="Béton. Humidité. Obscurité totale.";
        L->desc[1]="Les Hounds chassent au son.";
        L->desc[2]="L'Almond Water les calme temporairement.";
        L->desc[3]="Ne cours pas. Ne fais pas de bruit.";
        L->ambiance="~~ gouttes d'eau distantes ~~";
        float sd[][3]={
            {3,3,S_HOUND},{6,6,S_HEALTH},{11,5,S_HOUND},
            {4,11,S_ALMOND},{8,8,S_NOTE},{15,9,S_HOUND},
            {2,14,S_HEALTH},{13,13,S_ALMOND},{17,5,S_HOUND},
            {10,16,S_NOTE},{7,3,S_LAMP}
        };
        L->spr_count=(int)(sizeof(sd)/sizeof(sd[0]));
        memcpy(L->spr_data,sd,sizeof(sd));
    }
    {
        Level *L = &levels[2];
        memcpy(L->map, MAP2, sizeof(MAP2));
        L->spawn_x=1.5f*CELL_SIZE; L->spawn_y=1.5f*CELL_SIZE; L->spawn_angle=0.0f;
        /* Plafond quasi-noir industriel */
        L->ceil_top[0]=0.06f; L->ceil_top[1]=0.05f; L->ceil_top[2]=0.04f;
        L->ceil_bot[0]=0.10f; L->ceil_bot[1]=0.08f; L->ceil_bot[2]=0.06f;
        /* Sol métal / vapeur */
        L->floor_top[0]=0.12f; L->floor_top[1]=0.10f; L->floor_top[2]=0.08f;
        L->floor_bot[0]=0.04f; L->floor_bot[1]=0.03f; L->floor_bot[2]=0.02f;
        L->fog[0]=0.20f; L->fog[1]=0.18f; L->fog[2]=0.15f; L->fog_dist=500.0f;
        L->flicker=1; L->flicker_freq=2.5f;
        L->tex_wall1=3; L->tex_wall2=8; L->tex_wall3=3;
        L->tex_door=5; L->tex_exit=6;
        L->name="NIVEAU 2 — Pipe Dreams";
        L->desc[0]="Tuyaux. Vapeur. Chaleur insupportable.";
        L->desc[1]="Le Clump rode dans l'obscurité.";
        L->desc[2]="Tu entends quelque chose ramper...";
        L->desc[3]="Les tuyaux peuvent te blesser.";
        L->ambiance="~~ sifflements de vapeur ~~";
        float sd[][3]={
            {3,3,S_CLUMP},{6,4,S_HEALTH},{10,6,S_CLUMP},
            {5,9,S_ALMOND},{14,8,S_CLUMP},{3,13,S_NOTE},
            {12,12,S_HEALTH},{7,15,S_ALMOND},{17,7,S_CLUMP},
            {15,15,S_NOTE},{9,3,S_LAMP}
        };
        L->spr_count=(int)(sizeof(sd)/sizeof(sd[0]));
        memcpy(L->spr_data,sd,sizeof(sd));
    }
    {
        Level *L = &levels[3];
        memcpy(L->map, MAP3, sizeof(MAP3));
        L->spawn_x=1.5f*CELL_SIZE; L->spawn_y=1.5f*CELL_SIZE; L->spawn_angle=0.0f;
        /* Plafond bleu-vert aquatique */
        L->ceil_top[0]=0.30f; L->ceil_top[1]=0.50f; L->ceil_top[2]=0.58f;
        L->ceil_bot[0]=0.40f; L->ceil_bot[1]=0.62f; L->ceil_bot[2]=0.70f;
        /* Sol eau */
        L->floor_top[0]=0.22f; L->floor_top[1]=0.42f; L->floor_top[2]=0.55f;
        L->floor_bot[0]=0.08f; L->floor_bot[1]=0.20f; L->floor_bot[2]=0.30f;
        L->fog[0]=0.30f; L->fog[1]=0.50f; L->fog[2]=0.60f; L->fog_dist=800.0f;
        L->flicker=0; L->flicker_freq=0.0f;
        L->tex_wall1=10; L->tex_wall2=4; L->tex_wall3=10;
        L->tex_door=5; L->tex_exit=6;
        L->name="NIVEAU 3 — The Poolrooms";
        L->desc[0]="Carrelage blanc. Eau partout.";
        L->desc[1]="Calme inquiétant. Lumière douce.";
        L->desc[2]="Quelque chose nage sous l'eau...";
        L->desc[3]="La sortie est votre seul espoir.";
        L->ambiance="~~ eau et silence ~~";
        float sd[][3]={
            {3,3,S_SMILER},{7,5,S_ALMOND},{12,4,S_SMILER},
            {5,9,S_NOTE},{10,10,S_HEALTH},{15,7,S_SMILER},
            {8,14,S_ALMOND},{16,12,S_NOTE},{3,16,S_HOUND},
            {14,16,S_HEALTH},{17,3,S_SMILER},{9,9,S_LAMP}
        };
        L->spr_count=(int)(sizeof(sd)/sizeof(sd[0]));
        memcpy(L->spr_data,sd,sizeof(sd));
    }
}

static void load_level(int lvl)
{
    if(lvl<0||lvl>=NUM_LEVELS) return;
    current_level = lvl;
    Level *L = &levels[lvl];
    memcpy(map_cur, L->map, sizeof(map_cur));

    px_pos = L->spawn_x;
    py_pos = L->spawn_y;
    pa     = L->spawn_angle;
    pdx    = cosf(pa); pdy = sinf(pa);

    has_key = 0; 
    ammo  = 20 + lvl * 5;  

    num_doors = 0;
    for(int y=0;y<MAP_H;y++) for(int x=0;x<MAP_W;x++){
        if(map_cur[y][x]==C_DOOR && num_doors<MAX_DOORS){
            doors[num_doors].mx=x; doors[num_doors].my=y;
            doors[num_doors].open=0.0f; doors[num_doors].opening=0;
            num_doors++;
        }
    }

    num_sprites=0;
    for(int i=0;i<L->spr_count && num_sprites<MAX_SPRITES;i++){
        sprites[num_sprites].x      = (L->spr_data[i][0]+0.5f)*CELL_SIZE;
        sprites[num_sprites].y      = (L->spr_data[i][1]+0.5f)*CELL_SIZE;
        sprites[num_sprites].type   = (int)L->spr_data[i][2];
        sprites[num_sprites].active = 1;
        sprites[num_sprites].anim   = (float)i*0.3f;
        sprites[num_sprites].lore_id= (num_sprites) % 8;
        switch(sprites[num_sprites].type){
            case S_SMILER: sprites[num_sprites].speed=40.0f;  sprites[num_sprites].health=20; sprites[num_sprites].aggro_dist=180.0f; break;
            case S_HOUND:  sprites[num_sprites].speed=120.0f; sprites[num_sprites].health=35; sprites[num_sprites].aggro_dist=280.0f; break;
            case S_CLUMP:  sprites[num_sprites].speed=30.0f;  sprites[num_sprites].health=80; sprites[num_sprites].aggro_dist=320.0f; break;
            default:       sprites[num_sprites].speed=0.0f;   sprites[num_sprites].health=0;  sprites[num_sprites].aggro_dist=0.0f;  break;
        }
        num_sprites++;
    }

    flicker_val=1.0f; flicker_timer=0.0f;
    transition_timer=0;
    win_state=0;
    has_key=0;
}

static int has_key = 0;

static int    showing_note  = 0;
static const char *note_text= NULL;
static float  note_timer    = 0.0f;

typedef struct {
    float dist;
    int   tex_id;
    int   tex_x;
    int   side;  
} RayHit;

static int cell_solid(int mx, int my)
{
    if(mx<0||mx>=MAP_W||my<0||my>=MAP_H) return C_WALL1;
    int v=map_cur[my][mx];
    if(v==C_EMPTY) return 0;
    if(v==C_DOOR){
        for(int i=0;i<num_doors;i++)
            if(doors[i].mx==mx&&doors[i].my==my)
                return (doors[i].open>=(float)CELL_SIZE)?0:C_DOOR;
        return C_DOOR;
    }
    return v;
}

static RayHit cast_ray(float angle)
{
    RayHit H; H.dist=MAX_DEPTH*CELL_SIZE; H.tex_id=0; H.tex_x=0; H.side=0;
    float rdx=cosf(angle), rdy=sinf(angle);
    int mx=(int)(px_pos/CELL_SIZE), my=(int)(py_pos/CELL_SIZE);
    float ddx=(rdx==0)?1e30f:fabsf((float)CELL_SIZE/rdx);
    float ddy=(rdy==0)?1e30f:fabsf((float)CELL_SIZE/rdy);
    float sdx,sdy; int stx,sty;
    if(rdx<0){stx=-1;sdx=(px_pos-mx*CELL_SIZE)/fabsf(rdx);}
    else{stx=1;sdx=((mx+1)*CELL_SIZE-px_pos)/fabsf(rdx);}
    if(rdy<0){sty=-1;sdy=(py_pos-my*CELL_SIZE)/fabsf(rdy);}
    else{sty=1;sdy=((my+1)*CELL_SIZE-py_pos)/fabsf(rdy);}
    int side=0, cell=0;
    for(int depth=0;depth<MAX_DEPTH*CELL_SIZE;depth++){
        if(sdx<sdy){sdx+=ddx;mx+=stx;side=0;}
        else{sdy+=ddy;my+=sty;side=1;}
        cell=cell_solid(mx,my);
        if(cell>0) break;
    }
    float perp;
    if(side==0) perp=(mx*CELL_SIZE-px_pos+(stx<0?CELL_SIZE:0))/rdx;
    else         perp=(my*CELL_SIZE-py_pos+(sty<0?CELL_SIZE:0))/rdy;
    if(perp<0.01f) perp=0.01f;
    float wx;
    if(side==0) wx=py_pos+perp*rdy; else wx=px_pos+perp*rdx;
    wx=fmodf(wx,(float)CELL_SIZE)/(float)CELL_SIZE;
    if((side==0&&rdx>0)||(side==1&&rdy<0)) wx=1.0f-wx;

    Level *L=&levels[current_level];
    int tid;
    switch(cell){
        case C_WALL1: tid=L->tex_wall1; break;
        case C_WALL2: tid=L->tex_wall2; break;
        case C_WALL3: tid=L->tex_wall3; break;
        case C_DOOR:  tid=L->tex_door;  break;
        case C_EXIT:  tid=L->tex_exit;  break;
        default:      tid=L->tex_wall1; break;
    }
    H.dist=perp; H.tex_id=tid; H.tex_x=(int)(wx*(TEX_SZ-1)); H.side=side;
    return H;
}

static void draw_bg(void)
{
    Level *L=&levels[current_level];
    float fk=flicker_val;
    glBegin(GL_QUADS);
    glColor3f(L->ceil_top[0]*fk,L->ceil_top[1]*fk,L->ceil_top[2]*fk); glVertex2i(0,0);
    glColor3f(L->ceil_top[0]*fk,L->ceil_top[1]*fk,L->ceil_top[2]*fk); glVertex2i(SCREEN_W,0);
    glColor3f(L->ceil_bot[0]*fk,L->ceil_bot[1]*fk,L->ceil_bot[2]*fk); glVertex2i(SCREEN_W,SCREEN_H/2);
    glColor3f(L->ceil_bot[0]*fk,L->ceil_bot[1]*fk,L->ceil_bot[2]*fk); glVertex2i(0,SCREEN_H/2);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(L->floor_top[0],L->floor_top[1],L->floor_top[2]); glVertex2i(0,SCREEN_H/2);
    glColor3f(L->floor_top[0],L->floor_top[1],L->floor_top[2]); glVertex2i(SCREEN_W,SCREEN_H/2);
    glColor3f(L->floor_bot[0],L->floor_bot[1],L->floor_bot[2]); glVertex2i(SCREEN_W,SCREEN_H);
    glColor3f(L->floor_bot[0],L->floor_bot[1],L->floor_bot[2]); glVertex2i(0,SCREEN_H);
    glEnd();
}

static void draw_walls(void)
{
    Level *L=&levels[current_level];
    float fov_r=FOV_DEG*(float)M_PI/180.0f;
    float ray_s=pa-fov_r/2.0f;
    float fk=flicker_val;

    for(int col=0;col<NUM_RAYS;col++){
        float angle=ray_s+(float)col/NUM_RAYS*fov_r;
        RayHit H=cast_ray(angle);
        z_buffer[col]=H.dist;

        float line_h=(float)CELL_SIZE*SCREEN_H/H.dist;
        if(line_h>SCREEN_H) line_h=SCREEN_H;
        float top=SCREEN_H/2.0f-line_h/2.0f;

        float shade=1.0f-(H.dist/L->fog_dist);
        if(shade<0.0f) shade=0.0f;
        if(H.side==1) shade*=0.72f;
        shade*=fk;

        float tstep=(float)TEX_SZ/line_h;
        float tpos =(top<0)?-top*tstep:0.0f;
        int y0=(int)top, y1=(int)(top+line_h);
        if(y0<0) y0=0; if(y1>SCREEN_H) y1=SCREEN_H;

        glBegin(GL_LINES);
        for(int y=y0;y<y1;y++){
            int ty=(int)tpos&(TEX_SZ-1);
            unsigned char *c=tex[H.tex_id][ty][H.tex_x];
            glColor3f(c[0]/255.0f*shade, c[1]/255.0f*shade, c[2]/255.0f*shade);
            glVertex2i(col,y); glVertex2i(col,y+1);
            tpos+=tstep;
        }
        glEnd();
    }
}

static int spr_ord[MAX_SPRITES];
static float spr_dst[MAX_SPRITES];

static void sort_sprites(void)
{
    for(int i=0;i<num_sprites;i++){
        spr_ord[i]=i;
        spr_dst[i]=sprites[i].active?vdist(px_pos,py_pos,sprites[i].x,sprites[i].y):-1.0f;
    }
    for(int i=0;i<num_sprites-1;i++)
        for(int j=i+1;j<num_sprites;j++)
            if(spr_dst[spr_ord[j]]>spr_dst[spr_ord[i]]){int t=spr_ord[i];spr_ord[i]=spr_ord[j];spr_ord[j]=t;}
}

static void draw_sprites(void)
{
    sort_sprites();
    float fov_r=FOV_DEG*(float)M_PI/180.0f;
    Level *L=&levels[current_level];

    for(int i=0;i<num_sprites;i++){
        Sprite *sp=&sprites[spr_ord[i]];
        if(!sp->active) continue;
        float dx=sp->x-px_pos, dy=sp->y-py_pos;
        float d=sqrtf(dx*dx+dy*dy);
        if(d<4.0f) continue;
        float sa=atan2f(dy,dx);
        float ra=sa-pa;
        while(ra> (float)M_PI) ra-=2.0f*(float)M_PI;
        while(ra<-(float)M_PI) ra+=2.0f*(float)M_PI;
        if(fabsf(ra)>fov_r*0.75f) continue;

        float sx=(0.5f+ra/fov_r)*SCREEN_W;
        float h=(float)CELL_SIZE*SCREEN_H/d;
        if(h>SCREEN_H*2) h=SCREEN_H*2.0f;
        float top=SCREEN_H/2.0f-h/2.0f;
        float left=sx-h/2.0f;

        float shade=1.0f-d/L->fog_dist;
        if(shade<0.05f) shade=0.05f;
        shade*=flicker_val;

        float bob=0.0f;
        if(sp->type==S_SMILER||sp->type==S_HOUND||sp->type==S_CLUMP)
            bob=sinf(sp->anim*2.0f)*6.0f;

        float cr,cg,cb;
        switch(sp->type){
            case S_ALMOND: cr=0.6f;cg=0.9f;cb=1.0f; break;  
            case S_HEALTH: cr=0.1f;cg=0.9f;cb=0.2f; break;  
            case S_SMILER: cr=1.0f;cg=1.0f;cb=0.0f; break;  
            case S_HOUND:  cr=0.8f;cg=0.2f;cb=0.1f; break;  
            case S_CLUMP:  cr=0.4f;cg=0.1f;cb=0.4f; break;  
            case S_NOTE:   cr=0.9f;cg=0.85f;cb=0.6f;break;  
            case S_LAMP:   cr=1.0f;cg=0.9f;cb=0.6f; break;  
            default:       cr=0.8f;cg=0.8f;cb=0.8f; break;
        }
        cr*=shade; cg*=shade; cb*=shade;

        int x0=(int)left, x1=(int)(left+h);
        int y0=(int)(top+bob), y1=(int)(top+h+bob);
        if(x0<0) x0=0; if(x1>=SCREEN_W) x1=SCREEN_W-1;
        if(y0<0) y0=0; if(y1>=SCREEN_H) y1=SCREEN_H-1;

        for(int cx=x0;cx<=x1;cx++){
            if(d>=z_buffer[cx]) continue;
            float u=(float)(cx-left)/h;
            if(u<0||u>1) continue;
            float v0n=(float)(y0-(int)(top+bob))/h;
            int is_eye=0;
            if(sp->type==S_SMILER){
                float eu=(u-0.25f), ev=0.3f;
                if(fabsf(eu)<0.08f && fabsf(v0n-ev)<0.04f) is_eye=1;
                eu=u-0.75f;
                if(fabsf(eu)<0.08f && fabsf(v0n-ev)<0.04f) is_eye=1;
            }
            float fr=is_eye?1.0f:cr, fg=is_eye?1.0f:cg, fb=is_eye?0.0f:cb;
            glColor3f(fr,fg,fb);
            glBegin(GL_LINES); glVertex2i(cx,y0); glVertex2i(cx,y1); glEnd();
        }
    }
}

static void draw_str(int x,int y,void *font,const char *s)
{ glRasterPos2i(x,y); while(*s){glutBitmapCharacter(font,*s);s++;} }

static void draw_bar(int x,int y,int w,int h,float frac,float r,float g,float b,float br,float bg,float bb)
{
    glColor3f(br*0.3f,bg*0.3f,bb*0.3f);
    glBegin(GL_QUADS);glVertex2i(x,y);glVertex2i(x+w,y);glVertex2i(x+w,y+h);glVertex2i(x,y+h);glEnd();
    int fw=(int)(w*frac); if(fw>w)fw=w;
    glColor3f(r,g,b);
    glBegin(GL_QUADS);glVertex2i(x,y);glVertex2i(x+fw,y);glVertex2i(x+fw,y+h);glVertex2i(x,y+h);glEnd();
    glColor3f(0.6f,0.55f,0.4f);
    glBegin(GL_LINE_LOOP);glVertex2i(x,y);glVertex2i(x+w,y);glVertex2i(x+w,y+h);glVertex2i(x,y+h);glEnd();
}

static void draw_hud(void)
{
    Level *L=&levels[current_level];
    char buf[64];

    float hp=(float)health/MAX_HEALTH;
    float sp=(float)sanity/MAX_SANITY;
    draw_bar(20,SCREEN_H-45,180,14,hp,
             hp>0.5f?0.1f:0.9f, hp>0.5f?0.85f:0.1f, 0.05f,
             0.9f,0.1f,0.1f);
    draw_bar(210,SCREEN_H-45,160,14,sp,
             sp>0.5f?0.2f:0.9f, sp>0.5f?0.3f:0.15f, sp>0.5f?0.9f:0.6f,
             0.3f,0.1f,0.8f);

    glColor3f(0.9f,0.85f,0.65f);
    snprintf(buf,sizeof(buf),"HP:%d",health); draw_str(25,SCREEN_H-34,GLUT_BITMAP_9_BY_15,buf);
    snprintf(buf,sizeof(buf),"SAN:%d",sanity); draw_str(215,SCREEN_H-34,GLUT_BITMAP_9_BY_15,buf);
    snprintf(buf,sizeof(buf),"AMM:%d",ammo); draw_str(385,SCREEN_H-34,GLUT_BITMAP_9_BY_15,buf);

    glColor3f(1.0f,0.85f,0.1f);
    snprintf(buf,sizeof(buf),"SCORE:%d",score); draw_str(20,SCREEN_H-65,GLUT_BITMAP_9_BY_15,buf);

    if(has_key){ glColor3f(1,0.8f,0);draw_str(20,SCREEN_H-82,GLUT_BITMAP_9_BY_15,"[CLE TROUVEE]");}

    glColor3f(0.75f,0.70f,0.55f);
    draw_str(SCREEN_W/2-150,20,GLUT_BITMAP_9_BY_15,L->name);

    glColor3ub(55,50,38);
    draw_str(SCREEN_W/2-100,SCREEN_H-20,GLUT_BITMAP_9_BY_15,L->ambiance);

    glColor3ub(50,45,35); snprintf(buf,sizeof(buf),"%.0ffps",fps_val);
    draw_str(SCREEN_W-70,20,GLUT_BITMAP_9_BY_15,buf);

    if(showing_note && note_text){
        float alpha=fminf(1.0f,note_timer/30.0f);
        glColor4f(0,0,0,alpha*0.75f);
        glBegin(GL_QUADS);
        glVertex2i(SCREEN_W/2-340,SCREEN_H/2-80);
        glVertex2i(SCREEN_W/2+340,SCREEN_H/2-80);
        glVertex2i(SCREEN_W/2+340,SCREEN_H/2+80);
        glVertex2i(SCREEN_W/2-340,SCREEN_H/2+80);
        glEnd();
        glColor3f(0.9f*alpha,0.85f*alpha,0.65f*alpha);
        char tmp[512]; strncpy(tmp,note_text,511); tmp[511]=0;
        int ly=SCREEN_H/2-55; char *line=strtok(tmp,"\n");
        while(line){
            int lw=0; for(const char*p=line;*p;p++) lw+=glutBitmapWidth(GLUT_BITMAP_9_BY_15,*p);
            draw_str(SCREEN_W/2-lw/2,ly,GLUT_BITMAP_9_BY_15,line);
            ly+=20; line=strtok(NULL,"\n");
        }
        glColor3ub(100,90,70);
        draw_str(SCREEN_W/2-60,SCREEN_H/2+65,GLUT_BITMAP_9_BY_15,"[E] pour fermer");
    }

    if(shoot_anim>0){
        float f=(float)shoot_anim/8.0f;
        glColor4f(1,0.9f,0.5f,f*0.25f);
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(SCREEN_W,0);glVertex2i(SCREEN_W,SCREEN_H);glVertex2i(0,SCREEN_H);glEnd();
    }
    if(hurt_anim>0){
        float f=(float)hurt_anim/15.0f;
        glColor4f(0.8f,0,0,f*0.45f);
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(SCREEN_W,0);glVertex2i(SCREEN_W,SCREEN_H);glVertex2i(0,SCREEN_H);glEnd();
    }
    if(scare_anim>0){
        float f=(float)scare_anim/30.0f;
        glColor4f(0,0,0,f*0.5f);
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(SCREEN_W,0);glVertex2i(SCREEN_W,SCREEN_H);glVertex2i(0,SCREEN_H);glEnd();
    }
    float sv=1.0f-(float)sanity/MAX_SANITY;
    if(sv>0.05f){
        glColor4f(0,0,0,sv*0.6f);
        int vm=(int)(sv*80);
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(SCREEN_W,0);glVertex2i(SCREEN_W,vm);glVertex2i(0,vm);glEnd();
        glBegin(GL_QUADS);glVertex2i(0,SCREEN_H-vm);glVertex2i(SCREEN_W,SCREEN_H-vm);glVertex2i(SCREEN_W,SCREEN_H);glVertex2i(0,SCREEN_H);glEnd();
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(vm/2,0);glVertex2i(vm/2,SCREEN_H);glVertex2i(0,SCREEN_H);glEnd();
        glBegin(GL_QUADS);glVertex2i(SCREEN_W-vm/2,0);glVertex2i(SCREEN_W,0);glVertex2i(SCREEN_W,SCREEN_H);glVertex2i(SCREEN_W-vm/2,SCREEN_H);glEnd();
    }
}

static void draw_crosshair(void)
{
    int cx=SCREEN_W/2,cy=SCREEN_H/2,sz=9;
    float sv=(float)sanity/MAX_SANITY;
    glColor3f(0.8f*sv+0.2f,0.8f*sv,0.6f*sv);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex2i(cx-sz,cy);glVertex2i(cx-3,cy);
    glVertex2i(cx+3,cy);glVertex2i(cx+sz,cy);
    glVertex2i(cx,cy-sz);glVertex2i(cx,cy-3);
    glVertex2i(cx,cy+3);glVertex2i(cx,cy+sz);
    glEnd(); glLineWidth(1.0f);
}

#define MM_SZ  6
#define MM_X   (SCREEN_W - MAP_W*MM_SZ - 8)
#define MM_Y   30

static void draw_minimap(void)
{
    Level *L=&levels[current_level];
    glColor4f(0,0,0,0.55f);
    glBegin(GL_QUADS);
    glVertex2i(MM_X-2,MM_Y-2);glVertex2i(MM_X+MAP_W*MM_SZ+2,MM_Y-2);
    glVertex2i(MM_X+MAP_W*MM_SZ+2,MM_Y+MAP_H*MM_SZ+2);glVertex2i(MM_X-2,MM_Y+MAP_H*MM_SZ+2);
    glEnd();
    for(int y=0;y<MAP_H;y++) for(int x=0;x<MAP_W;x++){
        int v=map_cur[y][x];
        switch(v){
            case C_WALL1: glColor3ub(140,130,110); break;
            case C_WALL2: glColor3ub(100,95,80);   break;
            case C_WALL3: glColor3ub(80,80,70);    break;
            case C_DOOR:  glColor3ub(140,90,50);   break;
            case C_EXIT:  glColor3ub(40,200,60);   break;
            default:      glColor3ub(20,18,15);    break;
        }
        glBegin(GL_QUADS);
        glVertex2i(MM_X+x*MM_SZ,MM_Y+y*MM_SZ);
        glVertex2i(MM_X+x*MM_SZ+MM_SZ,MM_Y+y*MM_SZ);
        glVertex2i(MM_X+x*MM_SZ+MM_SZ,MM_Y+y*MM_SZ+MM_SZ);
        glVertex2i(MM_X+x*MM_SZ,MM_Y+y*MM_SZ+MM_SZ);
        glEnd();
    }
    for(int i=0;i<num_sprites;i++){
        if(!sprites[i].active) continue;
        int sx=MM_X+(int)(sprites[i].x/CELL_SIZE*MM_SZ);
        int sy=MM_Y+(int)(sprites[i].y/CELL_SIZE*MM_SZ);
        switch(sprites[i].type){
            case S_SMILER:case S_HOUND:case S_CLUMP: glColor3f(0.9f,0.1f,0.1f); break;
            case S_ALMOND: glColor3f(0.4f,0.7f,1.0f); break;
            case S_HEALTH: glColor3f(0.1f,0.9f,0.3f); break;
            case S_NOTE:   glColor3f(0.9f,0.8f,0.5f); break;
            default:       glColor3f(0.8f,0.8f,0.6f); break;
        }
        glPointSize(3); glBegin(GL_POINTS); glVertex2i(sx,sy); glEnd();
    }
    int ppx=MM_X+(int)(px_pos/CELL_SIZE*MM_SZ);
    int ppy=MM_Y+(int)(py_pos/CELL_SIZE*MM_SZ);
    glColor3f(1,1,0); glPointSize(5);
    glBegin(GL_POINTS); glVertex2i(ppx,ppy); glEnd();
    glLineWidth(1.5f); glBegin(GL_LINES);
    glVertex2i(ppx,ppy); glVertex2i(ppx+(int)(pdx*8),ppy+(int)(pdy*8)); glEnd();
    glLineWidth(1.0f);
    (void)L;
}

static void draw_title(void)
{
    glBegin(GL_QUADS);
    glColor3f(0.05f,0.04f,0.02f); glVertex2i(0,0);
    glColor3f(0.05f,0.04f,0.02f); glVertex2i(SCREEN_W,0);
    glColor3f(0.10f,0.08f,0.03f); glVertex2i(SCREEN_W,SCREEN_H);
    glColor3f(0.10f,0.08f,0.03f); glVertex2i(0,SCREEN_H);
    glEnd();
    for(int i=0;i<SCREEN_H;i+=3){
        float t=(float)i/SCREEN_H;
        glColor4f(0.7f*t,0.6f*t,0.1f*t,0.12f);
        glBegin(GL_LINES); glVertex2i(0,i); glVertex2i(SCREEN_W,i); glEnd();
    }

    void *big=GLUT_BITMAP_TIMES_ROMAN_24;
    void *med=GLUT_BITMAP_HELVETICA_18;
    void *sml=GLUT_BITMAP_9_BY_15;

    const char *logo[]={
        "  ___    _    ___ _  _  ___  ___   ___  __  __  ___",
        " | _ )  /_\\  / __| || |/ _ \\/ _ \\ / _ \\|  \\/  |/ __|",
        " | _ \\ / _ \\| (__| __ | (_) \\(_) | (_) | |\\/| |\\__ \\",
        " |___//_/ \\_\\___|_||_|\\___/ \\___/ \\___/|_|  |_||___/",
        NULL
    };
    glColor3f(0.85f,0.78f,0.20f);
    int ly=SCREEN_H/2-160;
    for(int i=0;logo[i];i++){
        int lw=0; for(const char*p=logo[i];*p;p++) lw+=glutBitmapWidth(sml,*p);
        draw_str(SCREEN_W/2-lw/2,ly,sml,logo[i]); ly+=18;
    }

    glColor3f(0.60f,0.55f,0.25f);
    const char *sub="un voyage a travers les etages interdits";
    int sw=0; for(const char*p=sub;*p;p++) sw+=glutBitmapWidth(med,*p);
    draw_str(SCREEN_W/2-sw/2,ly+10,med,sub);

    const char *lvls[]={
        "[ Niveau 0 ]  The Lobby          — Moquette jaune. Neons. Infini.",
        "[ Niveau 1 ]  The Habitable Zone — Béton. Obscurité. Hounds.",
        "[ Niveau 2 ]  Pipe Dreams        — Tuyaux rouillés. Vapeur. Clumps.",
        "[ Niveau 3 ]  The Poolrooms      — Eau calme. Carrelage. Silence.",
    };
    float cols[]={0.7f,0.5f,0.4f,0.55f};
    for(int i=0;i<4;i++){
        glColor3f(cols[i]*0.9f,cols[i]*0.8f,cols[i]*0.3f);
        int lw=0; for(const char*p=lvls[i];*p;p++) lw+=glutBitmapWidth(sml,*p);
        draw_str(SCREEN_W/2-lw/2,SCREEN_H/2-20+i*22,sml,lvls[i]);
    }

    glColor3f(0.45f,0.40f,0.20f);
    const char *instr[]={
        "W/S : avancer / reculer     A/D : tourner     Fleches G/D : strafe",
        "E : ouvrir porte / lire note     ESPACE : tirer     ESC : quitter",
        "Trouver la CLE, atteindre l'EXIT de chaque etage pour progresser",
        NULL
    };
    int iy=SCREEN_H/2+115;
    for(int i=0;instr[i];i++){
        int lw=0; for(const char*p=instr[i];*p;p++) lw+=glutBitmapWidth(sml,*p);
        draw_str(SCREEN_W/2-lw/2,iy,sml,instr[i]); iy+=20;
    }

    glColor3f(0.95f,0.85f,0.15f);
    const char *go=">>> ENTREE pour commencer <<<";
    int gw=0; for(const char*p=go;*p;p++) gw+=glutBitmapWidth(big,*p);
    draw_str(SCREEN_W/2-gw/2,SCREEN_H/2+190,big,go);
}

static void draw_transition(void)
{
    float t=(float)transition_timer/TRANS_DURATION;
    float black= (t<0.5f)? t*2.0f : (1.0f-t)*2.0f;
    black=fmaxf(0.0f,fminf(1.0f,1.0f-black));

    glColor4f(0,0,0,black);
    glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(SCREEN_W,0);glVertex2i(SCREEN_W,SCREEN_H);glVertex2i(0,SCREEN_H);glEnd();

    if(t>0.35f&&t<0.65f){
        char msg[64];
        snprintf(msg,sizeof(msg),"Descente vers le Niveau %d...",current_level);
        void *big=GLUT_BITMAP_TIMES_ROMAN_24;
        int mw=0; for(const char*p=msg;*p;p++) mw+=glutBitmapWidth(big,*p);
        glColor4f(0.8f,0.7f,0.2f,1.0f-(fabsf(t-0.5f)*6.0f));
        draw_str(SCREEN_W/2-mw/2,SCREEN_H/2,big,msg);
        Level *L=&levels[current_level];
        int nw=0; for(const char*p=L->name;*p;p++) nw+=glutBitmapWidth(GLUT_BITMAP_HELVETICA_18,*p);
        glColor4f(0.6f,0.55f,0.2f,1.0f-(fabsf(t-0.5f)*6.0f));
        draw_str(SCREEN_W/2-nw/2,SCREEN_H/2+30,GLUT_BITMAP_HELVETICA_18,L->name);
        for(int i=0;i<4;i++){
            glColor4f(0.45f,0.40f,0.18f,0.85f-(fabsf(t-0.5f)*6.0f));
            int dw=0; for(const char*p=L->desc[i];*p;p++) dw+=glutBitmapWidth(GLUT_BITMAP_9_BY_15,*p);
            draw_str(SCREEN_W/2-dw/2,SCREEN_H/2+65+i*18,GLUT_BITMAP_9_BY_15,L->desc[i]);
        }
    }
}

static void draw_end(void)
{
    if(win_state==3){
        glBegin(GL_QUADS);
        glColor3f(0.02f,0.12f,0.08f);glVertex2i(0,0);
        glColor3f(0.02f,0.12f,0.08f);glVertex2i(SCREEN_W,0);
        glColor3f(0.05f,0.20f,0.12f);glVertex2i(SCREEN_W,SCREEN_H);
        glColor3f(0.05f,0.20f,0.12f);glVertex2i(0,SCREEN_H);
        glEnd();
        void *big=GLUT_BITMAP_TIMES_ROMAN_24;
        void *med=GLUT_BITMAP_HELVETICA_18;
        void *sml=GLUT_BITMAP_9_BY_15;
        glColor3f(0.3f,1.0f,0.4f);
        const char *v="VOUS AVEZ ESCAPE LES BACKROOMS";
        int vw=0; for(const char*p=v;*p;p++) vw+=glutBitmapWidth(big,*p);
        draw_str(SCREEN_W/2-vw/2,SCREEN_H/2-80,big,v);
        glColor3f(0.6f,0.9f,0.5f);
        const char *sub="4 etages parcourus. La realite vous accueille a nouveau.";
        int sw=0; for(const char*p=sub;*p;p++) sw+=glutBitmapWidth(med,*p);
        draw_str(SCREEN_W/2-sw/2,SCREEN_H/2-40,med,sub);
        glColor3f(0.5f,0.75f,0.4f);
        char sc[64]; snprintf(sc,sizeof(sc),"Score final : %d points",score);
        int scw=0; for(const char*p=sc;*p;p++) scw+=glutBitmapWidth(med,*p);
        draw_str(SCREEN_W/2-scw/2,SCREEN_H/2+10,med,sc);
        glColor3f(0.3f,0.55f,0.3f);
        const char *note="Les Backrooms existent toujours. Quelque part.";
        int nw=0; for(const char*p=note;*p;p++) nw+=glutBitmapWidth(sml,*p);
        draw_str(SCREEN_W/2-nw/2,SCREEN_H/2+50,sml,note);
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.12f,0.02f,0.02f);glVertex2i(0,0);
        glColor3f(0.12f,0.02f,0.02f);glVertex2i(SCREEN_W,0);
        glColor3f(0.04f,0.01f,0.01f);glVertex2i(SCREEN_W,SCREEN_H);
        glColor3f(0.04f,0.01f,0.01f);glVertex2i(0,SCREEN_H);
        glEnd();
        void *big=GLUT_BITMAP_TIMES_ROMAN_24;
        void *med=GLUT_BITMAP_HELVETICA_18;
        glColor3f(0.9f,0.1f,0.1f);
        const char *v="VOUS ETES PERDU DANS LES BACKROOMS";
        int vw=0; for(const char*p=v;*p;p++) vw+=glutBitmapWidth(big,*p);
        draw_str(SCREEN_W/2-vw/2,SCREEN_H/2-60,big,v);
        glColor3f(0.6f,0.3f,0.3f);
        char sc[64]; snprintf(sc,sizeof(sc),"Score : %d  —  Etage atteint : %d",score,current_level);
        int sw=0; for(const char*p=sc;*p;p++) sw+=glutBitmapWidth(med,*p);
        draw_str(SCREEN_W/2-sw/2,SCREEN_H/2,med,sc);
        const char *sub="Vous n'etes pas le premier. Vous ne serez pas le dernier.";
        int bw=0; for(const char*p=sub;*p;p++) bw+=glutBitmapWidth(GLUT_BITMAP_9_BY_15,*p);
        glColor3f(0.4f,0.2f,0.2f);
        draw_str(SCREEN_W/2-bw/2,SCREEN_H/2+35,GLUT_BITMAP_9_BY_15,sub);
    }
    glColor3f(0.65f,0.60f,0.35f);
    const char *rst="ENTREE : recommencer depuis le Niveau 0   ESC : quitter";
    int rw=0; for(const char*p=rst;*p;p++) rw+=glutBitmapWidth(GLUT_BITMAP_9_BY_15,*p);
    draw_str(SCREEN_W/2-rw/2,SCREEN_H/2+100,GLUT_BITMAP_9_BY_15,rst);
}

static int can_move_w(float nx,float ny)
{
    float m=10.0f;
    float pts[4][2]={{nx-m,ny-m},{nx+m,ny-m},{nx-m,ny+m},{nx+m,ny+m}};
    for(int i=0;i<4;i++){
        int gx=(int)(pts[i][0]/CELL_SIZE), gy=(int)(pts[i][1]/CELL_SIZE);
        if(cell_solid(gx,gy)) return 0;
    }
    return 1;
}

static void update_entities(void)
{
    for(int i=0;i<num_sprites;i++){
        Sprite *sp=&sprites[i];
        if(!sp->active) continue;
        if(sp->type!=S_SMILER&&sp->type!=S_HOUND&&sp->type!=S_CLUMP) continue;
        sp->anim+=dt;
        float dx=px_pos-sp->x, dy=py_pos-sp->y;
        float d=sqrtf(dx*dx+dy*dy);
        if(d<2.0f) continue;

        if(d<sp->aggro_dist){
            float nx=sp->x+dx/d*sp->speed*dt;
            float ny=sp->y+dy/d*sp->speed*dt;
            if(can_move_w(nx,sp->y)) sp->x=nx;
            if(can_move_w(sp->x,ny)) sp->y=ny;

            if(sp->type==S_SMILER && d<150.0f){
                int sdmg=(int)(15.0f*dt);
                if(sdmg<1) sdmg=1;
                sanity-=sdmg;
                scare_anim=30;
                if(sanity<0){sanity=0;}
            }
        }

        if(d<CELL_SIZE*0.55f){
            int dmg;
            switch(sp->type){
                case S_SMILER: dmg=(int)(8.0f*dt*60.0f*0.016f); break;
                case S_HOUND:  dmg=(int)(20.0f*dt*60.0f*0.016f);break;
                case S_CLUMP:  dmg=(int)(12.0f*dt*60.0f*0.016f);break;
                default: dmg=0;
            }
            if(dmg<1) dmg=1;
            health-=dmg; hurt_anim=15;
            if(health<=0){health=0;win_state=2;}
        }
    }
}

static void update_pickups(void)
{
    for(int i=0;i<num_sprites;i++){
        Sprite *sp=&sprites[i];
        if(!sp->active) continue;
        float d=vdist(px_pos,py_pos,sp->x,sp->y);
        if(d>CELL_SIZE*0.55f) continue;
        switch(sp->type){
            case S_ALMOND:
                sanity+=35; if(sanity>MAX_SANITY) sanity=MAX_SANITY;
                score+=75; sp->active=0;
                break;
            case S_HEALTH:
                health+=30; if(health>MAX_HEALTH) health=MAX_HEALTH;
                score+=50; sp->active=0;
                break;
            case S_NOTE:
                if(!showing_note){
                    showing_note=1;
                    note_text=lore_notes[sp->lore_id%8];
                    note_timer=0.0f;
                    score+=25;
                    sp->active=0;
                }
                break;
            default: break;
        }
    }
}

static void do_shoot(void)
{
    if(ammo<=0||showing_note) return;
    ammo--; shoot_anim=8;
    for(int i=0;i<num_sprites;i++){
        Sprite *sp=&sprites[i];
        if(!sp->active) continue;
        if(sp->type!=S_SMILER&&sp->type!=S_HOUND&&sp->type!=S_CLUMP) continue;
        float dx=sp->x-px_pos,dy=sp->y-py_pos;
        float d=sqrtf(dx*dx+dy*dy);
        if(d>MAX_DEPTH*CELL_SIZE*0.5f) continue;
        float sa=atan2f(dy,dx), ra=sa-pa;
        while(ra> (float)M_PI) ra-=2.0f*(float)M_PI;
        while(ra<-(float)M_PI) ra+=2.0f*(float)M_PI;
        if(fabsf(ra)<0.14f&&d<z_buffer[SCREEN_W/2]+32.0f){
            int dmg=20;
            sp->health-=dmg;
            if(sp->health<=0){sp->active=0; score+=sp->type==S_CLUMP?300:(sp->type==S_HOUND?150:100);}
        }
    }
}

static void do_interact(void)
{
    if(showing_note){showing_note=0;return;}
    float tx=px_pos+pdx*CELL_SIZE*0.7f, ty=py_pos+pdy*CELL_SIZE*0.7f;
    int mx=(int)(tx/CELL_SIZE),my=(int)(ty/CELL_SIZE);
    for(int i=0;i<num_doors;i++){
        if(doors[i].mx==mx&&doors[i].my==my){doors[i].opening=1;return;}
    }
}

static void update_game(void)
{
    if(win_state!=0) return;

    if(transition_timer>0){
        transition_timer--;
        if(transition_timer==TRANS_DURATION/2){
            load_level(current_level);
        }
        return;
    }

    total_time+=dt;
    if((int)(total_time*10)%8==0){
        sanity-=1; if(sanity<0) sanity=0;
    }

    Level *L=&levels[current_level];
    if(L->flicker){
        flicker_timer+=dt*L->flicker_freq;
        flicker_val=0.85f+0.15f*sinf(flicker_timer*6.0f)+0.05f*sinf(flicker_timer*17.0f);
        /* Coupures aléatoires */
        float r=hash((int)(total_time*100),(int)(total_time*37),99);
        if(r>0.97f) flicker_val*=0.2f;
    } else flicker_val=1.0f;

    float rs=2.0f*dt*60.0f;
    if(keys_st.a) pa=fix_ang(pa-rs*(float)M_PI/180.0f);
    if(keys_st.d) pa=fix_ang(pa+rs*(float)M_PI/180.0f);
    pdx=cosf(pa); pdy=sinf(pa);
    float sx=pdy,sy=-pdx;
    float ms=170.0f*dt;
    float nx=px_pos,ny=py_pos;
    if(keys_st.w){nx+=pdx*ms;ny+=pdy*ms;}
    if(keys_st.s){nx-=pdx*ms;ny-=pdy*ms;}
    if(keys_st.left){nx-=sx*ms*0.8f;ny-=sy*ms*0.8f;}
    if(keys_st.right){nx+=sx*ms*0.8f;ny+=sy*ms*0.8f;}
    if(can_move_w(nx,py_pos)) px_pos=nx;
    if(can_move_w(px_pos,ny)) py_pos=ny;

    for(int i=0;i<num_doors;i++){
        if(doors[i].opening&&doors[i].open<CELL_SIZE)
            doors[i].open+=DOOR_SPD*dt*60.0f;
        if(doors[i].open>=CELL_SIZE) doors[i].open=(float)CELL_SIZE;
    }

    update_entities();
    update_pickups();

    if(showing_note) note_timer+=dt*60.0f;

    if(shoot_anim>0) shoot_anim--;
    if(hurt_anim>0)  hurt_anim--;
    if(scare_anim>0) scare_anim--;

    int gx=(int)(px_pos/CELL_SIZE),gy=(int)(py_pos/CELL_SIZE);
    if(map_cur[gy][gx]==C_EXIT && has_key){
        if(current_level>=NUM_LEVELS-1){
            win_state=3;  
        } else {
            int next=current_level+1;
            current_level=next;
            transition_timer=TRANS_DURATION;
            win_state=1;  
        }
        score+=500;
    }

}

static void display(void)
{
    int now=glutGet(GLUT_ELAPSED_TIME);
    dt=(now-last_time)/1000.0f;
    if(dt>0.05f) dt=0.05f;
    last_time=now;

    fcount++; fps_timer2+=dt;
    if(fps_timer2>=0.5f){ fps_val=fcount/fps_timer2; fcount=0; fps_timer2=0; }

    glClear(GL_COLOR_BUFFER_BIT);

    if(!game_started){
        draw_title();
    } else if(win_state==3){
        draw_end();
    } else if(win_state==2){
        draw_end();
    } else {
        if(transition_timer>0 && win_state==1){
            draw_bg(); draw_walls(); draw_sprites();
            draw_crosshair(); draw_hud(); draw_minimap();
            draw_transition();
            transition_timer--;
            if(transition_timer==TRANS_DURATION/2) load_level(current_level);
            if(transition_timer<=0){win_state=0;}
        } else {
            update_game();
            draw_bg(); draw_walls(); draw_sprites();
            draw_crosshair(); draw_hud(); draw_minimap();
        }
    }

    glutSwapBuffers();
    glutPostRedisplay();
}

static void key_dn(unsigned char k,int x,int y)
{
    (void)x;(void)y;
    switch(k){
        case 'w':case'W': keys_st.w=1; break;
        case 's':case'S': keys_st.s=1; break;
        case 'a':case'A': keys_st.a=1; break;
        case 'd':case'D': keys_st.d=1; break;
        case 'e':case'E':
            if(game_started&&win_state==0) do_interact();
            break;
        case ' ':
            if(game_started&&win_state==0) do_shoot();
            break;
        case '\r':case'\n':
            if(!game_started){
                game_started=1;
                health=MAX_HEALTH; sanity=MAX_SANITY;
                score=0; ammo=20; total_time=0.0f;
                load_level(0);
            } else if(win_state==2||win_state==3){
                win_state=0; health=MAX_HEALTH; sanity=MAX_SANITY;
                score=0; ammo=20; total_time=0.0f;
                has_key=0;
                load_level(0);
            }
            break;
        case 27: exit(0);
    }
}
static void key_up2(unsigned char k,int x,int y)
{
    (void)x;(void)y;
    switch(k){
        case 'w':case'W': keys_st.w=0; break;
        case 's':case'S': keys_st.s=0; break;
        case 'a':case'A': keys_st.a=0; break;
        case 'd':case'D': keys_st.d=0; break;
    }
}
static void spc_dn(int k,int x,int y)
{ (void)x;(void)y; if(k==GLUT_KEY_LEFT)keys_st.left=1; if(k==GLUT_KEY_RIGHT)keys_st.right=1; }
static void spc_up(int k,int x,int y)
{ (void)x;(void)y; if(k==GLUT_KEY_LEFT)keys_st.left=0; if(k==GLUT_KEY_RIGHT)keys_st.right=0; }

static void init_gl(void)
{
    glClearColor(0.01f,0.01f,0.01f,1.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    gluOrtho2D(0,SCREEN_W,SCREEN_H,0);
    gen_tex();
    init_levels();
    last_time=glutGet(GLUT_ELAPSED_TIME);
    memset(&keys_st,0,sizeof(keys_st));
}

int main(int argc,char **argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(SCREEN_W,SCREEN_H);
    glutCreateWindow("The Backrooms — Raycaster");
    init_gl();
    glutDisplayFunc(display);
    glutKeyboardFunc(key_dn);
    glutKeyboardUpFunc(key_up2);
    glutSpecialFunc(spc_dn);
    glutSpecialUpFunc(spc_up);
    glutMainLoop();
    return 0;
}
