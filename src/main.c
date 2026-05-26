#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ─── Constantes fenêtre ─── */
#define SCREEN_W     1280
#define SCREEN_H     720

/* ─── Carte ─── */
#define MAP_W        16
#define MAP_H        16
#define CELL_SIZE    64

/* ─── Rendu ─── */
#define NUM_RAYS     SCREEN_W
#define FOV_DEG      66.0f
#define HALF_FOV     (FOV_DEG / 2.0f)
#define MAX_DEPTH    20

/* ─── Taille textures procédurales ─── */
#define TEX_W        64
#define TEX_H        64
#define NUM_TEXTURES  5

/* ─── Sprites ─── */
#define MAX_SPRITES  32

/* ─── Jeu ─── */
#define MAX_HEALTH   100
#define DOOR_SPEED   2.0f   /* px/frame d'ouverture */

/* ─── Types de cellules ─── */
#define CELL_EMPTY   0
#define CELL_WALL    1   /* mur béton */
#define CELL_WALL2   2   /* mur briques */
#define CELL_WALL3   3   /* mur métal */
#define CELL_DOOR    4   /* porte */
#define CELL_EXIT    5   /* sortie */

/* ─── Types de sprites ─── */
#define SPR_NONE     0
#define SPR_KEY      1
#define SPR_HEALTH   2
#define SPR_ENEMY    3
#define SPR_BARREL   4

/* ══════════════════════════════════════════════
   CARTE
   ══════════════════════════════════════════════ */
static int map[MAP_H][MAP_W] =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,2,0,0,1},
    {1,0,1,1,1,0,1,1,1,1,1,0,1,0,0,1},
    {1,0,1,0,0,0,1,0,0,0,1,0,1,0,0,1},
    {1,0,1,0,1,1,1,0,1,0,1,0,4,1,0,1},
    {1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,1},
    {1,1,1,0,1,0,3,3,3,3,3,1,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,0,0,1,0,1,0,1},
    {1,0,1,1,1,1,3,3,3,3,0,1,0,4,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,1,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,1,0,1,0,0,0,1,0,1},
    {1,1,1,1,1,1,0,1,0,1,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,5,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

/* ══════════════════════════════════════════════
   STRUCTS
   ══════════════════════════════════════════════ */
typedef struct {
    float x, y;       /* position monde */
    int   type;
    int   active;
    float anim;        /* pour ennemi : angle oscillation */
} Sprite;

typedef struct {
    int   mx, my;     /* position grille */
    float open;       /* 0.0 = fermée, 64.0 = ouverte */
    int   opening;    /* 1 = en train d'ouvrir */
} Door;

/* ══════════════════════════════════════════════
   ÉTAT GLOBAL
   ══════════════════════════════════════════════ */
static float px_pos, py_pos;  /* position joueur (pixels monde) */
static float pa;               /* angle joueur (radians) */
static float pdx, pdy;        /* direction unitaire */

static int   health      = MAX_HEALTH;
static int   score       = 0;
static int   has_key     = 0;
static int   ammo        = 30;
static int   shoot_anim  = 0;
static int   hurt_anim   = 0;
static int   win_state   = 0;   /* 0=jeu 1=victoire 2=mort */
static int   game_started= 0;

/* Textures procédurales : tableaux de couleurs RGB */
static unsigned char textures[NUM_TEXTURES][TEX_H][TEX_W][3];

/* Z-buffer (distance mur par colonne) */
static float z_buffer[SCREEN_W];

/* Sprites */
static Sprite sprites[MAX_SPRITES];
static int    num_sprites = 0;

/* Portes */
static Door   doors[16];
static int    num_doors = 0;

/* Touches */
static struct {
    int w, s, a, d, left, right, e, space;
} keys_state;

/* Timer FPS */
static int    last_time   = 0;
static float  delta_time  = 0.016f;
static int    frame_count = 0;
static float  fps         = 60.0f;

/* ══════════════════════════════════════════════
   UTILITAIRES MATH
   ══════════════════════════════════════════════ */
static float fix_angle(float a)
{
    while (a < 0.0f)        a += 2.0f * M_PI;
    while (a >= 2.0f * M_PI) a -= 2.0f * M_PI;
    return a;
}

static float dist(float ax, float ay, float bx, float by)
{
    return sqrtf((bx-ax)*(bx-ax) + (by-ay)*(by-ay));
}

/* ══════════════════════════════════════════════
   GÉNÉRATION DES TEXTURES PROCÉDURALES
   ══════════════════════════════════════════════ */
static void gen_textures(void)
{
    int x, y, i;
    unsigned char r, g, b;

    /* TEX 0 : béton (mur 1) — carreaux gris avec grout */
    for (y = 0; y < TEX_H; y++)
        for (x = 0; x < TEX_W; x++)
        {
            int bx = x / 16, by = y / 16;
            int grout = (x % 16 == 0) || (y % 16 == 0) ||
                        ((by & 1) == 0 && x % 16 == 8);
            if (grout) { r = 80; g = 78; b = 75; }
            else {
                int n = ((bx * 7 + by * 13) * 17) & 0x1F;
                r = (unsigned char)(160 + n);
                g = (unsigned char)(155 + n);
                b = (unsigned char)(140 + n);
            }
            textures[0][y][x][0] = r;
            textures[0][y][x][1] = g;
            textures[0][y][x][2] = b;
        }

    /* TEX 1 : briques rouge-brun (mur 2) */
    for (y = 0; y < TEX_H; y++)
        for (x = 0; x < TEX_W; x++)
        {
            int offset = (y / 12) & 1 ? 16 : 0;
            int bx = (x + offset) / 32;
            int grout = (y % 12 == 0) ||
                        ((x + offset) % 32 == 0) ||
                        ((x + offset) % 32 == 31);
            if (grout) { r = 55; g = 40; b = 30; }
            else {
                int n = ((bx * 11 + (y/12) * 19) * 13) & 0x1F;
                r = (unsigned char)(160 + n);
                g = (unsigned char)(70 + n/2);
                b = (unsigned char)(40 + n/3);
            }
            textures[1][y][x][0] = r;
            textures[1][y][x][1] = g;
            textures[1][y][x][2] = b;
        }

    /* TEX 2 : métal (mur 3) — plaques avec rivets */
    for (y = 0; y < TEX_H; y++)
        for (x = 0; x < TEX_W; x++)
        {
            int rivet = ((x % 16 < 2) && (y % 16 < 2));
            int seam  = (x % 16 == 0) || (y % 16 == 0);
            int n = (x ^ y ^ (x*3)) & 0x0F;
            if (rivet)     { r = 200; g = 190; b = 170; }
            else if (seam) { r = 60;  g = 58;  b = 55;  }
            else { r = (unsigned char)(100+n); g=(unsigned char)(98+n); b=(unsigned char)(88+n); }
            textures[2][y][x][0] = r;
            textures[2][y][x][1] = g;
            textures[2][y][x][2] = b;
        }

    /* TEX 3 : porte en bois */
    for (y = 0; y < TEX_H; y++)
        for (x = 0; x < TEX_W; x++)
        {
            int panel = (x / 20) & 1;
            int plank = (y / 8)  & 1;
            int edge  = (x % 20 == 0) || (y % 8 == 0);
            int n = ((x * 3 + y) * 7) & 0x0F;
            if (edge)       { r = 60;  g = 40; b = 20; }
            else if (panel) { r = (unsigned char)(120+n+plank*10); g=(unsigned char)(80+n); b=(unsigned char)(40+n/2); }
            else            { r = (unsigned char)(140+n);          g=(unsigned char)(90+n); b=(unsigned char)(45+n/2); }
            textures[3][y][x][0] = r;
            textures[3][y][x][1] = g;
            textures[3][y][x][2] = b;
        }

    /* TEX 4 : sortie (vert lumineux + cadre) */
    for (y = 0; y < TEX_H; y++)
        for (x = 0; x < TEX_W; x++)
        {
            int border = (x < 4 || x >= TEX_W-4 || y < 4 || y >= TEX_H-4);
            int arrow  = (abs(x - TEX_W/2) + abs(y - TEX_H/2) < 12);
            int n = (x ^ y) & 0x0F;
            if (border)     { r = 0; g = 200; b = 50; }
            else if (arrow) { r = 255; g = 255; b = 0; }
            else            { r = (unsigned char)(n*2); g=(unsigned char)(80+n*3); b=(unsigned char)(n); }
            textures[4][y][x][0] = r;
            textures[4][y][x][1] = g;
            textures[4][y][x][2] = b;
        }

    (void)i;
}

/* ══════════════════════════════════════════════
   INITIALISATION DU NIVEAU
   ══════════════════════════════════════════════ */
static void init_level(void)
{
    int x, y;

    px_pos = 1.5f * CELL_SIZE;
    py_pos = 1.5f * CELL_SIZE;
    pa     = 0.0f;
    pdx    = cosf(pa);
    pdy    = sinf(pa);

    num_sprites = 0;
    num_doors   = 0;

    /* Scan carte pour sprites et portes */
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
        {
            if (map[y][x] == CELL_DOOR && num_doors < 16)
            {
                doors[num_doors].mx      = x;
                doors[num_doors].my      = y;
                doors[num_doors].open    = 0.0f;
                doors[num_doors].opening = 0;
                num_doors++;
            }
        }

    /* Placement manuel des sprites */
    float sdata[][3] = {
        /* x_cell, y_cell, type */
        {3,  3,  SPR_KEY    },
        {7,  7,  SPR_HEALTH },
        {11, 3,  SPR_ENEMY  },
        {5,  9,  SPR_ENEMY  },
        {9,  13, SPR_BARREL },
        {13, 9,  SPR_HEALTH },
        {2,  13, SPR_ENEMY  },
        {10, 7,  SPR_BARREL },
    };
    int n = (int)(sizeof(sdata) / sizeof(sdata[0]));
    for (int i = 0; i < n && num_sprites < MAX_SPRITES; i++)
    {
        sprites[num_sprites].x      = (sdata[i][0] + 0.5f) * CELL_SIZE;
        sprites[num_sprites].y      = (sdata[i][1] + 0.5f) * CELL_SIZE;
        sprites[num_sprites].type   = (int)sdata[i][2];
        sprites[num_sprites].active = 1;
        sprites[num_sprites].anim   = 0.0f;
        num_sprites++;
    }
}

/* ══════════════════════════════════════════════
   DDA — test si une cellule bloque le rayon
   Retourne 0 si libre, sinon le type de mur
   ══════════════════════════════════════════════ */
static int cell_solid(int mx, int my)
{
    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return CELL_WALL;
    int v = map[my][mx];
    if (v == CELL_EMPTY) return 0;
    if (v == CELL_DOOR)
    {
        /* porte : chercher l'état d'ouverture */
        for (int i = 0; i < num_doors; i++)
            if (doors[i].mx == mx && doors[i].my == my)
                return (doors[i].open >= (float)CELL_SIZE) ? 0 : CELL_DOOR;
        return CELL_DOOR;
    }
    return v;
}

/* ══════════════════════════════════════════════
   RAYCASTING DDA
   ══════════════════════════════════════════════ */
typedef struct {
    float dist;      /* distance corrigée fisheye */
    int   tex_id;    /* indice texture */
    int   tex_x;     /* colonne dans la texture */
    int   side;      /* 0=EO 1=NS (pour ombrage) */
} RayHit;

static RayHit cast_ray(float ray_angle)
{
    RayHit hit;
    hit.dist   = MAX_DEPTH * CELL_SIZE;
    hit.tex_id = 0;
    hit.tex_x  = 0;
    hit.side   = 0;

    float rx = px_pos, ry = py_pos;
    float rdx = cosf(ray_angle);
    float rdy = sinf(ray_angle);

    /* Cellule de départ */
    int mx = (int)(rx / CELL_SIZE);
    int my = (int)(ry / CELL_SIZE);

    /* Distances jusqu'à prochaine ligne verticale/horizontale */
    float delta_x = (rdx == 0.0f) ? 1e30f : fabsf((float)CELL_SIZE / rdx);
    float delta_y = (rdy == 0.0f) ? 1e30f : fabsf((float)CELL_SIZE / rdy);

    float side_x, side_y;
    int step_x, step_y;

    if (rdx < 0.0f) {
        step_x =  -1;
        side_x = (rx - mx * CELL_SIZE) / fabsf(rdx);
    } else {
        step_x =   1;
        side_x = ((mx + 1) * CELL_SIZE - rx) / fabsf(rdx);
    }
    if (rdy < 0.0f) {
        step_y =  -1;
        side_y = (ry - my * CELL_SIZE) / fabsf(rdy);
    } else {
        step_y =   1;
        side_y = ((my + 1) * CELL_SIZE - ry) / fabsf(rdy);
    }

    int side = 0;
    int cell = 0;
    int depth = 0;

    while (depth < MAX_DEPTH * CELL_SIZE)
    {
        if (side_x < side_y) {
            side_x += delta_x;
            mx     += step_x;
            side    = 0;
        } else {
            side_y += delta_y;
            my     += step_y;
            side    = 1;
        }
        cell = cell_solid(mx, my);
        if (cell > 0) break;
        depth++;
    }

    /* Distance perpendiculaire (évite l'effet fisheye) */
    float perp;
    if (side == 0)
        perp = (mx * CELL_SIZE - rx + (step_x < 0 ? CELL_SIZE : 0)) / rdx;
    else
        perp = (my * CELL_SIZE - ry + (step_y < 0 ? CELL_SIZE : 0)) / rdy;

    if (perp < 0.01f) perp = 0.01f;

    /* Coordonnée X sur le mur frappé */
    float wall_x;
    if (side == 0) wall_x = ry + perp * rdy;
    else           wall_x = rx + perp * rdx;
    wall_x = fmodf(wall_x, (float)CELL_SIZE) / (float)CELL_SIZE;
    if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0))
        wall_x = 1.0f - wall_x;

    /* Sélection texture selon type de cellule */
    int tex;
    switch (cell) {
        case CELL_WALL:  tex = 0; break;
        case CELL_WALL2: tex = 1; break;
        case CELL_WALL3: tex = 2; break;
        case CELL_DOOR:  tex = 3; break;
        case CELL_EXIT:  tex = 4; break;
        default:         tex = 0; break;
    }

    hit.dist   = perp;
    hit.tex_id = tex;
    hit.tex_x  = (int)(wall_x * (TEX_W - 1));
    hit.side   = side;
    return hit;
}

/* ══════════════════════════════════════════════
   DESSIN FOND (ciel + sol avec dégradé)
   ══════════════════════════════════════════════ */
static void draw_background(void)
{
    /* Plafond */
    glBegin(GL_QUADS);
    glColor3ub(18, 14, 12);   glVertex2i(0, 0);
    glColor3ub(18, 14, 12);   glVertex2i(SCREEN_W, 0);
    glColor3ub(30, 24, 18);   glVertex2i(SCREEN_W, SCREEN_H/2);
    glColor3ub(30, 24, 18);   glVertex2i(0, SCREEN_H/2);
    glEnd();

    /* Sol */
    glBegin(GL_QUADS);
    glColor3ub(35, 30, 20);   glVertex2i(0, SCREEN_H/2);
    glColor3ub(35, 30, 20);   glVertex2i(SCREEN_W, SCREEN_H/2);
    glColor3ub(12, 10,  8);   glVertex2i(SCREEN_W, SCREEN_H);
    glColor3ub(12, 10,  8);   glVertex2i(0, SCREEN_H);
    glEnd();
}

/* ══════════════════════════════════════════════
   DESSIN DES MURS TEXTURÉS (colonne par colonne)
   ══════════════════════════════════════════════ */
static void draw_walls(void)
{
    int col;
    float fov_rad   = FOV_DEG * M_PI / 180.0f;
    float ray_start = pa - fov_rad / 2.0f;

    for (col = 0; col < NUM_RAYS; col++)
    {
        float ray_angle = ray_start + (float)col / NUM_RAYS * fov_rad;
        RayHit h = cast_ray(ray_angle);

        z_buffer[col] = h.dist;

        float line_h = (float)CELL_SIZE * SCREEN_H / h.dist;
        if (line_h > SCREEN_H) line_h = SCREEN_H;

        float line_top = SCREEN_H / 2.0f - line_h / 2.0f;

        /* Ombre distance */
        float shade = 1.0f - (h.dist / (MAX_DEPTH * CELL_SIZE * 0.8f));
        if (shade < 0.1f) shade = 0.1f;
        /* Ombre côté NS légèrement plus sombre */
        if (h.side == 1) shade *= 0.75f;

        /* Dessiner chaque pixel de la colonne avec la texture */
        float tex_step = (float)TEX_H / line_h;
        float tex_pos  = (line_top < 0) ? -line_top * tex_step : 0.0f;

        int draw_top    = (int)line_top;
        int draw_bottom = (int)(line_top + line_h);
        if (draw_top    < 0)        draw_top    = 0;
        if (draw_bottom > SCREEN_H) draw_bottom = SCREEN_H;

        /* On dessine la colonne pixel par pixel via GL_POINTS */
        glBegin(GL_LINES);
        for (int y = draw_top; y < draw_bottom; y++)
        {
            int ty = (int)tex_pos & (TEX_H - 1);
            int tx = h.tex_x;
            unsigned char *c = textures[h.tex_id][ty][tx];

            float r = c[0] * shade / 255.0f;
            float g = c[1] * shade / 255.0f;
            float b = c[2] * shade / 255.0f;

            glColor3f(r, g, b);
            glVertex2i(col, y);
            glVertex2i(col, y + 1);

            tex_pos += tex_step;
        }
        glEnd();
    }
}

/* ══════════════════════════════════════════════
   TRI DES SPRITES (peintre — plus loin d'abord)
   ══════════════════════════════════════════════ */
static int spr_order[MAX_SPRITES];
static float spr_dist[MAX_SPRITES];

static void sort_sprites(void)
{
    int i, j;
    for (i = 0; i < num_sprites; i++) {
        spr_order[i] = i;
        spr_dist[i]  = sprites[i].active ?
            dist(px_pos, py_pos, sprites[i].x, sprites[i].y) : -1.0f;
    }
    /* Tri à bulles (N petit) */
    for (i = 0; i < num_sprites - 1; i++)
        for (j = i + 1; j < num_sprites; j++)
            if (spr_dist[spr_order[j]] > spr_dist[spr_order[i]]) {
                int tmp = spr_order[i];
                spr_order[i] = spr_order[j];
                spr_order[j] = tmp;
            }
}

/* ══════════════════════════════════════════════
   DESSIN DES SPRITES
   ══════════════════════════════════════════════ */
static void draw_sprites(void)
{
    sort_sprites();
    float fov_rad = FOV_DEG * M_PI / 180.0f;

    int i;
    for (i = 0; i < num_sprites; i++)
    {
        Sprite *sp = &sprites[spr_order[i]];
        if (!sp->active) continue;

        /* Vecteur joueur -> sprite */
        float dx = sp->x - px_pos;
        float dy = sp->y - py_pos;
        float d  = sqrtf(dx*dx + dy*dy);
        if (d < 4.0f) continue;

        /* Angle relatif */
        float spr_angle  = atan2f(dy, dx);
        float rel_angle  = spr_angle - pa;
        /* Normaliser [-PI, PI] */
        while (rel_angle >  M_PI) rel_angle -= 2.0f * M_PI;
        while (rel_angle < -M_PI) rel_angle += 2.0f * M_PI;

        if (fabsf(rel_angle) > fov_rad * 0.75f) continue;

        /* Position écran */
        float screen_x = (0.5f + rel_angle / fov_rad) * SCREEN_W;

        float h = (float)CELL_SIZE * SCREEN_H / d;
        if (h > SCREEN_H * 2) h = SCREEN_H * 2.0f;

        float top  = SCREEN_H / 2.0f - h / 2.0f;
        float left = screen_x - h / 2.0f;

        /* Couleur du sprite selon type */
        float shade = 1.0f - d / (MAX_DEPTH * CELL_SIZE * 0.7f);
        if (shade < 0.1f) shade = 0.1f;

        float cr, cg, cb;
        switch (sp->type) {
            case SPR_KEY:    cr=1.0f; cg=0.9f; cb=0.1f; break;  /* jaune */
            case SPR_HEALTH: cr=0.1f; cg=0.9f; cb=0.2f; break;  /* vert  */
            case SPR_ENEMY:  cr=0.9f; cg=0.1f; cb=0.1f; break;  /* rouge */
            case SPR_BARREL: cr=0.5f; cg=0.4f; cb=0.3f; break;  /* marron*/
            default:         cr=1.0f; cg=1.0f; cb=1.0f; break;
        }
        cr *= shade; cg *= shade; cb *= shade;

        int x0 = (int)left, x1 = (int)(left + h);
        int y0 = (int)top,  y1 = (int)(top  + h);
        if (x0 < 0) x0 = 0;
        if (x1 >= SCREEN_W) x1 = SCREEN_W - 1;
        if (y0 < 0) y0 = 0;
        if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;

        /* Dessiner le sprite colonne par colonne avec z-test */
        int sx;
        for (sx = x0; sx <= x1; sx++)
        {
            if (d >= z_buffer[sx]) continue;  /* derrière un mur */

            /* UV sprite */
            float u = (float)(sx - left) / h;
            if (u < 0 || u > 1) continue;

            /* On dessine un trait vertical pour ce sx */
            /* Dessin simple : quadrant coloré avec liseré */
            int border_h = (int)(h * 0.08f);
            if (border_h < 1) border_h = 1;

            /* Couleur intérieure ou bordure */
            int is_border = (sx - x0 < 2 || x1 - sx < 2 ||
                             y0 - (int)top < border_h);

            float fr = is_border ? cr * 1.4f : cr;
            float fg = is_border ? cg * 1.4f : cg;
            float fb = is_border ? cb * 1.4f : cb;
            if (fr > 1.0f) fr = 1.0f;
            if (fg > 1.0f) fg = 1.0f;
            if (fb > 1.0f) fb = 1.0f;

            glColor3f(fr, fg, fb);
            glBegin(GL_LINES);
            glVertex2i(sx, y0);
            glVertex2i(sx, y1);
            glEnd();
        }
    }
}

/* ══════════════════════════════════════════════
   MINIMAP
   ══════════════════════════════════════════════ */
#define MM_CELL  8
#define MM_X     (SCREEN_W - MAP_W * MM_CELL - 10)
#define MM_Y     10

static void draw_minimap(void)
{
    int x, y;

    /* Fond semi-transparent */
    glColor4f(0, 0, 0, 0.5f);
    glBegin(GL_QUADS);
    glVertex2i(MM_X - 2, MM_Y - 2);
    glVertex2i(MM_X + MAP_W * MM_CELL + 2, MM_Y - 2);
    glVertex2i(MM_X + MAP_W * MM_CELL + 2, MM_Y + MAP_H * MM_CELL + 2);
    glVertex2i(MM_X - 2, MM_Y + MAP_H * MM_CELL + 2);
    glEnd();

    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
        {
            int v = map[y][x];
            if      (v == CELL_WALL)  glColor3ub(190,185,165);
            else if (v == CELL_WALL2) glColor3ub(160, 70, 40);
            else if (v == CELL_WALL3) glColor3ub(100, 98, 85);
            else if (v == CELL_DOOR)  glColor3ub(140, 90, 50);
            else if (v == CELL_EXIT)  glColor3ub( 20,200, 50);
            else                       glColor3ub( 25, 22, 18);

            glBegin(GL_QUADS);
            glVertex2i(MM_X + x * MM_CELL,           MM_Y + y * MM_CELL);
            glVertex2i(MM_X + x * MM_CELL + MM_CELL, MM_Y + y * MM_CELL);
            glVertex2i(MM_X + x * MM_CELL + MM_CELL, MM_Y + y * MM_CELL + MM_CELL);
            glVertex2i(MM_X + x * MM_CELL,           MM_Y + y * MM_CELL + MM_CELL);
            glEnd();
        }

    /* Sprites sur la minimap */
    for (int i = 0; i < num_sprites; i++) {
        if (!sprites[i].active) continue;
        int sx = MM_X + (int)(sprites[i].x / CELL_SIZE * MM_CELL);
        int sy = MM_Y + (int)(sprites[i].y / CELL_SIZE * MM_CELL);
        switch (sprites[i].type) {
            case SPR_KEY:    glColor3f(1.0f, 0.9f, 0.1f); break;
            case SPR_HEALTH: glColor3f(0.1f, 0.9f, 0.2f); break;
            case SPR_ENEMY:  glColor3f(0.9f, 0.1f, 0.1f); break;
            case SPR_BARREL: glColor3f(0.6f, 0.5f, 0.3f); break;
        }
        glPointSize(4);
        glBegin(GL_POINTS);
        glVertex2i(sx, sy);
        glEnd();
    }

    /* Joueur — point jaune + direction */
    int ppx = MM_X + (int)(px_pos / CELL_SIZE * MM_CELL);
    int ppy = MM_Y + (int)(py_pos / CELL_SIZE * MM_CELL);

    glColor3f(1, 1, 0);
    glPointSize(6);
    glBegin(GL_POINTS);
    glVertex2i(ppx, ppy);
    glEnd();

    glLineWidth(2);
    glBegin(GL_LINES);
    glVertex2i(ppx, ppy);
    glVertex2i(ppx + (int)(pdx * 10), ppy + (int)(pdy * 10));
    glEnd();
    glLineWidth(1);
}

/* ══════════════════════════════════════════════
   VISEUR
   ══════════════════════════════════════════════ */
static void draw_crosshair(void)
{
    int cx = SCREEN_W / 2, cy = SCREEN_H / 2;
    int sz = 10;
    glColor3f(0.9f, 0.9f, 0.7f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2i(cx - sz, cy); glVertex2i(cx - 3, cy);
    glVertex2i(cx + 3, cy); glVertex2i(cx + sz, cy);
    glVertex2i(cx, cy - sz); glVertex2i(cx, cy - 3);
    glVertex2i(cx, cy + 3); glVertex2i(cx, cy + sz);
    glEnd();
    glLineWidth(1);
}

/* ══════════════════════════════════════════════
   HUD (santé, munitions, score, messages)
   ══════════════════════════════════════════════ */
static void draw_string(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    while (*s) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s);
        s++;
    }
}

static void draw_hud(void)
{
    char buf[64];

    /* Barre santé */
    int bar_w = 200;
    int bar_h = 16;
    int bx = 20, by = SCREEN_H - 40;

    /* Fond barre */
    glColor3ub(40, 10, 10);
    glBegin(GL_QUADS);
    glVertex2i(bx, by); glVertex2i(bx + bar_w, by);
    glVertex2i(bx + bar_w, by + bar_h); glVertex2i(bx, by + bar_h);
    glEnd();

    /* Remplissage */
    float hp_frac = (float)health / MAX_HEALTH;
    int fill_w = (int)(bar_w * hp_frac);
    float rc = (1.0f - hp_frac) * 0.9f + 0.1f;
    float gc = hp_frac * 0.8f;
    glColor3f(rc, gc, 0.05f);
    glBegin(GL_QUADS);
    glVertex2i(bx, by); glVertex2i(bx + fill_w, by);
    glVertex2i(bx + fill_w, by + bar_h); glVertex2i(bx, by + bar_h);
    glEnd();

    /* Contour */
    glColor3ub(120, 100, 80);
    glBegin(GL_LINE_LOOP);
    glVertex2i(bx, by); glVertex2i(bx + bar_w, by);
    glVertex2i(bx + bar_w, by + bar_h); glVertex2i(bx, by + bar_h);
    glEnd();

    glColor3f(0.9f, 0.85f, 0.7f);
    snprintf(buf, sizeof(buf), "HP: %d", health);
    draw_string(bx + 4, by + 12, buf);

    /* Munitions */
    glColor3f(0.8f, 0.75f, 0.5f);
    snprintf(buf, sizeof(buf), "AMMO: %d", ammo);
    draw_string(bx + bar_w + 20, by + 12, buf);

    /* Score */
    glColor3f(1.0f, 0.9f, 0.2f);
    snprintf(buf, sizeof(buf), "SCORE: %d", score);
    draw_string(20, SCREEN_H - 65, buf);

    /* Clé */
    if (has_key) {
        glColor3f(1.0f, 0.85f, 0.0f);
        draw_string(20, SCREEN_H - 85, "[ CLE ]");
    }

    /* Flash tir */
    if (shoot_anim > 0) {
        float f = shoot_anim / 8.0f;
        glColor4f(1, 0.9f, 0.5f, f * 0.35f);
        glBegin(GL_QUADS);
        glVertex2i(0,0); glVertex2i(SCREEN_W,0);
        glVertex2i(SCREEN_W,SCREEN_H); glVertex2i(0,SCREEN_H);
        glEnd();
    }

    /* Flash dégâts */
    if (hurt_anim > 0) {
        float f = hurt_anim / 12.0f;
        glColor4f(0.8f, 0, 0, f * 0.4f);
        glBegin(GL_QUADS);
        glVertex2i(0,0); glVertex2i(SCREEN_W,0);
        glVertex2i(SCREEN_W,SCREEN_H); glVertex2i(0,SCREEN_H);
        glEnd();
    }

    /* Instructions en bas à droite */
    glColor3ub(80, 75, 60);
    draw_string(SCREEN_W - 260, SCREEN_H - 70, "W/S: avancer");
    draw_string(SCREEN_W - 260, SCREEN_H - 55, "A/D: tourner");
    draw_string(SCREEN_W - 260, SCREEN_H - 40, "Fleches: strafe  E: porte");
    draw_string(SCREEN_W - 260, SCREEN_H - 25, "ESPACE: tirer   ESC: quitter");

    /* FPS */
    glColor3ub(60, 55, 45);
    snprintf(buf, sizeof(buf), "%.0f fps", fps);
    draw_string(SCREEN_W / 2 - 20, 20, buf);
}

/* ══════════════════════════════════════════════
   ÉCRAN TITRE
   ══════════════════════════════════════════════ */
static void draw_title(void)
{
    glColor3ub(10, 8, 6);
    glBegin(GL_QUADS);
    glVertex2i(0,0); glVertex2i(SCREEN_W,0);
    glVertex2i(SCREEN_W,SCREEN_H); glVertex2i(0,SCREEN_H);
    glEnd();

    /* Lignes décoratives */
    for (int i = 0; i < SCREEN_H; i += 40) {
        float t = (float)i / SCREEN_H;
        glColor3f(t * 0.15f, t * 0.10f, t * 0.05f);
        glBegin(GL_LINES);
        glVertex2i(0, i); glVertex2i(SCREEN_W, i);
        glEnd();
    }

    void *big   = GLUT_BITMAP_TIMES_ROMAN_24;
    void *small = GLUT_BITMAP_HELVETICA_18;
    void *tiny  = GLUT_BITMAP_9_BY_15;

    glColor3ub(220, 180, 80);
    const char *title = "BACKROOMS";
    int tw = 0;
    for (const char *p = title; *p; p++) tw += glutBitmapWidth(big, *p);
    glRasterPos2i(SCREEN_W/2 - tw/2, SCREEN_H/2 - 80);
    for (const char *p = title; *p; p++) glutBitmapCharacter(big, *p);

    glColor3ub(160, 140, 100);
    const char *sub = "Linkin Park is the best band ever created";
    int sw = 0;
    for (const char *p = sub; *p; p++) sw += glutBitmapWidth(small, *p);
    glRasterPos2i(SCREEN_W/2 - sw/2, SCREEN_H/2 - 40);
    for (const char *p = sub; *p; p++) glutBitmapCharacter(small, *p);

    glColor3ub(90, 80, 60);
    const char *lines[] = {
        "OBJECTIF : trouver la CLE, eliminer les ennemis, atteindre la SORTIE",
        "",
        "W / S              avancer / reculer",
        "A / D              tourner gauche / droite",
        "Fleches G / D      strafe gauche / droite",
        "E                  ouvrir une porte (face a elle)",
        "ESPACE             tirer",
        "ESC                quitter",
        "",
        "Objets : CLE (jaune)  SOIN (vert)  ENNEMI (rouge)  BARIL (marron)"
    };
    int nl = sizeof(lines)/sizeof(lines[0]);
    for (int i = 0; i < nl; i++) {
        int lw = 0;
        for (const char *p = lines[i]; *p; p++) lw += glutBitmapWidth(tiny, *p);
        glRasterPos2i(SCREEN_W/2 - lw/2, SCREEN_H/2 + 20 + i * 22);
        for (const char *p = lines[i]; *p; p++) glutBitmapCharacter(tiny, *p);
    }

    glColor3ub(200, 180, 80);
    const char *start = ">>> Appuyez sur ENTREE pour commencer <<<";
    int stw = 0;
    for (const char *p = start; *p; p++) stw += glutBitmapWidth(small, *p);
    glRasterPos2i(SCREEN_W/2 - stw/2, SCREEN_H/2 + 260);
    for (const char *p = start; *p; p++) glutBitmapCharacter(small, *p);
}

/* ══════════════════════════════════════════════
   ÉCRAN FIN
   ══════════════════════════════════════════════ */
static void draw_end_screen(void)
{
    float r = (win_state == 1) ? 0.05f : 0.4f;
    float g = (win_state == 1) ? 0.35f : 0.02f;
    glColor3f(r * 0.3f, g * 0.3f, 0.02f);
    glBegin(GL_QUADS);
    glVertex2i(0,0); glVertex2i(SCREEN_W,0);
    glVertex2i(SCREEN_W,SCREEN_H); glVertex2i(0,SCREEN_H);
    glEnd();

    void *big  = GLUT_BITMAP_TIMES_ROMAN_24;
    void *med  = GLUT_BITMAP_HELVETICA_18;

    const char *msg = (win_state == 1) ? "VICTOIRE !" : "GAME OVER";
    glColor3ub(win_state==1 ? 80:220, win_state==1 ? 220:40, win_state==1 ? 80:40);
    int mw = 0;
    for (const char *p = msg; *p; p++) mw += glutBitmapWidth(big, *p);
    glRasterPos2i(SCREEN_W/2 - mw/2, SCREEN_H/2 - 60);
    for (const char *p = msg; *p; p++) glutBitmapCharacter(big, *p);

    char sc[64];
    snprintf(sc, sizeof(sc), "Score final : %d", score);
    glColor3ub(200, 180, 100);
    int sw = 0;
    for (const char *p = sc; *p; p++) sw += glutBitmapWidth(med, *p);
    glRasterPos2i(SCREEN_W/2 - sw/2, SCREEN_H/2);
    for (const char *p = sc; *p; p++) glutBitmapCharacter(med, *p);

    const char *restart = "Appuyez sur ENTREE pour rejouer  |  ESC pour quitter";
    glColor3ub(140, 130, 100);
    int rw = 0;
    for (const char *p = restart; *p; p++) rw += glutBitmapWidth(med, *p);
    glRasterPos2i(SCREEN_W/2 - rw/2, SCREEN_H/2 + 60);
    for (const char *p = restart; *p; p++) glutBitmapCharacter(med, *p);
}

/* ══════════════════════════════════════════════
   MISE À JOUR DU JEU
   ══════════════════════════════════════════════ */
static int can_move(float nx, float ny)
{
    float margin = 8.0f;  /* hitbox du joueur */
    float pts[4][2] = {
        {nx - margin, ny - margin},
        {nx + margin, ny - margin},
        {nx - margin, ny + margin},
        {nx + margin, ny + margin}
    };
    for (int i = 0; i < 4; i++) {
        int gx = (int)(pts[i][0] / CELL_SIZE);
        int gy = (int)(pts[i][1] / CELL_SIZE);
        if (cell_solid(gx, gy)) return 0;
    }
    return 1;
}

static void shoot(void)
{
    if (ammo <= 0) return;
    ammo--;
    shoot_anim = 8;

    /* Vérifier hit sur les sprites */
    for (int i = 0; i < num_sprites; i++) {
        if (!sprites[i].active) continue;
        if (sprites[i].type != SPR_ENEMY) continue;

        float dx = sprites[i].x - px_pos;
        float dy = sprites[i].y - py_pos;
        float d  = sqrtf(dx*dx + dy*dy);
        if (d > MAX_DEPTH * CELL_SIZE * 0.5f) continue;

        float sa = atan2f(dy, dx);
        float ra = sa - pa;
        while (ra >  M_PI) ra -= 2.0f*M_PI;
        while (ra < -M_PI) ra += 2.0f*M_PI;

        if (fabsf(ra) < 0.15f && d < z_buffer[SCREEN_W/2] + 32.0f) {
            sprites[i].active = 0;
            score += 100;
        }
    }
}

static void interact(void)
{
    /* Trouver la porte la plus proche devant */
    float tx = px_pos + pdx * (CELL_SIZE * 0.7f);
    float ty = py_pos + pdy * (CELL_SIZE * 0.7f);
    int mx = (int)(tx / CELL_SIZE);
    int my = (int)(ty / CELL_SIZE);

    for (int i = 0; i < num_doors; i++) {
        if (doors[i].mx == mx && doors[i].my == my) {
            doors[i].opening = 1;
            return;
        }
    }
}

static void update_game(void)
{
    float rot_speed  = 2.0f * delta_time * 60.0f;
    float move_speed = 180.0f * delta_time;

    /* Rotation */
    if (keys_state.a)     pa = fix_angle(pa - rot_speed * M_PI / 180.0f);
    if (keys_state.d)     pa = fix_angle(pa + rot_speed * M_PI / 180.0f);
    pdx = cosf(pa);
    pdy = sinf(pa);

    /* Vecteur perpendiculaire (strafe) */
    float sx =  pdy;
    float sy = -pdx;

    /* Déplacement avec collisions séparées sur X et Y */
    float nx = px_pos, ny = py_pos;

    if (keys_state.w) { nx += pdx * move_speed; ny += pdy * move_speed; }
    if (keys_state.s) { nx -= pdx * move_speed; ny -= pdy * move_speed; }
    if (keys_state.left)  { nx -= sx * move_speed * 0.8f; ny -= sy * move_speed * 0.8f; }
    if (keys_state.right) { nx += sx * move_speed * 0.8f; ny += sy * move_speed * 0.8f; }

    /* Essayer X seul, puis Y seul (collision glissante) */
    if (can_move(nx, py_pos)) px_pos = nx;
    if (can_move(px_pos, ny)) py_pos = ny;

    /* Mise à jour portes */
    for (int i = 0; i < num_doors; i++) {
        if (doors[i].opening && doors[i].open < (float)CELL_SIZE)
            doors[i].open += DOOR_SPEED * delta_time * 60.0f;
        if (doors[i].open >= (float)CELL_SIZE)
            doors[i].open = (float)CELL_SIZE;
    }

    /* Ramasser sprites */
    for (int i = 0; i < num_sprites; i++) {
        if (!sprites[i].active) continue;
        float d = dist(px_pos, py_pos, sprites[i].x, sprites[i].y);
        if (d < CELL_SIZE * 0.55f) {
            switch (sprites[i].type) {
                case SPR_KEY:
                    has_key = 1;
                    score += 200;
                    sprites[i].active = 0;
                    break;
                case SPR_HEALTH:
                    health += 25;
                    if (health > MAX_HEALTH) health = MAX_HEALTH;
                    score += 50;
                    sprites[i].active = 0;
                    break;
                case SPR_ENEMY: {
                    /* Dégâts de contact */
                    int dmg = (int)(30.0f * delta_time * 60.0f * 0.016f);
                    if (dmg < 1) dmg = 1;
                    health -= dmg;
                    hurt_anim = 12;
                    if (health <= 0) { health = 0; win_state = 2; }
                    break;
                }
                default: break;
            }
        }
    }

    /* Atteindre la sortie */
    {
        int gx = (int)(px_pos / CELL_SIZE);
        int gy = (int)(py_pos / CELL_SIZE);
        if (map[gy][gx] == CELL_EXIT && has_key)
            win_state = 1;
    }

    /* Anims tir / blessure */
    if (shoot_anim > 0) shoot_anim--;
    if (hurt_anim  > 0) hurt_anim--;

    /* Animation ennemis (oscillation) */
    for (int i = 0; i < num_sprites; i++)
        if (sprites[i].active && sprites[i].type == SPR_ENEMY)
            sprites[i].anim += delta_time;
}

/* ══════════════════════════════════════════════
   CALLBACK GLUT : display
   ══════════════════════════════════════════════ */
static void display(void)
{
    /* Delta time */
    int now  = glutGet(GLUT_ELAPSED_TIME);
    delta_time = (now - last_time) / 1000.0f;
    if (delta_time > 0.05f) delta_time = 0.05f;
    last_time = now;

    /* FPS */
    frame_count++;
    static float fps_timer = 0;
    fps_timer += delta_time;
    if (fps_timer >= 0.5f) {
        fps = frame_count / fps_timer;
        frame_count = 0;
        fps_timer   = 0;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    if (!game_started) {
        draw_title();
    } else if (win_state != 0) {
        draw_end_screen();
    } else {
        update_game();
        draw_background();
        draw_walls();
        draw_sprites();
        draw_crosshair();
        draw_hud();
        draw_minimap();
    }

    glutSwapBuffers();
    glutPostRedisplay();
}

/* ══════════════════════════════════════════════
   CALLBACKS CLAVIER
   ══════════════════════════════════════════════ */
static void key_down(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
        case 'w': case 'W': keys_state.w = 1; break;
        case 's': case 'S': keys_state.s = 1; break;
        case 'a': case 'A': keys_state.a = 1; break;
        case 'd': case 'D': keys_state.d = 1; break;
        case 'e': case 'E': if (game_started && win_state == 0) interact(); break;
        case ' ':            if (game_started && win_state == 0) shoot();   break;
        case '\r': case '\n':
            if (!game_started) { game_started = 1; init_level(); }
            else if (win_state != 0) {
                win_state = 0; health = MAX_HEALTH;
                score = 0; has_key = 0; ammo = 30;
                game_started = 1;
                init_level();
            }
            break;
        case 27: exit(0); break;
    }
}

static void key_up(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
        case 'w': case 'W': keys_state.w = 0; break;
        case 's': case 'S': keys_state.s = 0; break;
        case 'a': case 'A': keys_state.a = 0; break;
        case 'd': case 'D': keys_state.d = 0; break;
    }
}

static void special_down(int key, int x, int y)
{
    (void)x; (void)y;
    if (key == GLUT_KEY_LEFT)  keys_state.left  = 1;
    if (key == GLUT_KEY_RIGHT) keys_state.right = 1;
}

static void special_up(int key, int x, int y)
{
    (void)x; (void)y;
    if (key == GLUT_KEY_LEFT)  keys_state.left  = 0;
    if (key == GLUT_KEY_RIGHT) keys_state.right = 0;
}

/* ══════════════════════════════════════════════
   INIT OPENGL
   ══════════════════════════════════════════════ */
static void init_gl(void)
{
    glClearColor(0.02f, 0.015f, 0.01f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gluOrtho2D(0, SCREEN_W, SCREEN_H, 0);
    gen_textures();
    last_time = glutGet(GLUT_ELAPSED_TIME);
    memset(&keys_state, 0, sizeof(keys_state));
}

/* ══════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════ */
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(SCREEN_W, SCREEN_H);
    glutCreateWindow("Backrooms Raycaster");

    init_gl();

    glutDisplayFunc(display);
    glutKeyboardFunc(key_down);
    glutKeyboardUpFunc(key_up);
    glutSpecialFunc(special_down);
    glutSpecialUpFunc(special_up);

    glutMainLoop();
    return 0;
}
