/**
 * ============================================================
 *  GRAFIKA KOMPUTER DAN VISUALISASI — UTS 2026
 *  Universitas Sultan Ageng Tirtayasa — Fakultas Teknik
 *  Jurusan Informatika
 * ------------------------------------------------------------
 *  Kelompok 4 — Kelas C
 *  Bangun Datar : Poligon Bebas — Bintang Segi-8 (Octagram)
 *  Transformasi : Translasi + Rotasi (interaktif keyboard)
 *  Background   : Gambar / Tekstur (JPG atau PNG)
 *  Pengampu     : Muchtar Ali Setyo Yudono, S.T., M.T.
 * ============================================================
 *
 *  PERSIAPAN:
 *    1. Taruh file gambar background di folder yang sama
 *       dengan main.cpp, beri nama: background.jpg
 *       (atau background.png — sesuaikan #define BG_FILE)
 *    2. Taruh stb_image.h di folder yang sama
 *       Download: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
 *
 *  BUILD (Windows + FreeGLUT):
 *    g++ main.cpp -o polygon.exe -lfreeglut -lopengl32 -lglu32
 *
 *  BUILD (Linux):
 *    g++ main.cpp -o polygon -lGL -lGLU -lglut
 *
 *  KONTROL KEYBOARD:
 *    W / S / A / D   → Translasi  (atas/bawah/kiri/kanan)
 *    Q / E           → Rotasi     (kiri/kanan)
 *    R               → Reset posisi & sudut
 *    +/-             → Zoom in/out
 *    F1              → Toggle auto-rotasi
 *    ESC             → Keluar
 * ============================================================
 */

 #ifdef _WIN32
 #include <windows.h>
 #include <GL/freeglut.h>
#else
 #include <GL/freeglut.h>
#endif

// ── STB Image: loader gambar JPG/PNG tanpa library tambahan ──
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>

// ─── NAMA FILE BACKGROUND ───────────────────────────────────
// Ganti sesuai nama file gambar kamu (harus ada di folder yang sama)
#define BG_FILE "background.jpg"
// Jika pakai PNG: #define BG_FILE "background.png"

// ─── KONSTANTA ──────────────────────────────────────────────
const float PI    = 3.14159265358979f;
const int   WIN_W = 900;
const int   WIN_H = 700;

// ─── STATE APLIKASI ─────────────────────────────────────────
float  g_tx      = 0.0f;
float  g_ty      = 0.0f;
float  g_angle   = 0.0f;
float  g_scale   = 1.0f;
float  g_time    = 0.0f;
bool   g_autoRot = false;
float  g_speed   = 2.0f;

// ─── TEXTURE ────────────────────────────────────────────────
GLuint g_bgTexID = 0;      // OpenGL texture ID untuk background
bool   g_bgLoaded = false; // flag apakah gambar berhasil di-load

// ─── PARAMETER POLYGON ──────────────────────────────────────
const int   OUTER_POINTS = 5;
const float OUTER_RADIUS = 100.0f;
const float INNER_RADIUS = 40.0f;

// ════════════════════════════════════════════════════════════
//  LOAD TEXTURE BACKGROUND
// ════════════════════════════════════════════════════════════
/**
* Memuat gambar dari file menggunakan stb_image,
* lalu mengunggahnya ke GPU sebagai OpenGL texture.
*
* Mendukung: JPG, PNG, BMP, TGA
*/
void loadBackgroundTexture(const char* filename) {
   int width, height, channels;

   // stb_image: load gambar, paksa 4 channel (RGBA)
   // flip vertikal karena koordinat Y OpenGL terbalik
   stbi_set_flip_vertically_on_load(true);
   unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);

   if (!data) {
       printf("[WARNING] Gagal load gambar: %s\n", filename);
       printf("          Pastikan file ada di folder yang sama dengan main.cpp\n");
       printf("          Program tetap jalan dengan background hitam.\n");
       g_bgLoaded = false;
       return;
   }

   printf("[INFO] Background loaded: %s (%dx%d, %d ch)\n",
          filename, width, height, channels);

   // Generate texture ID
   glGenTextures(1, &g_bgTexID);
   glBindTexture(GL_TEXTURE_2D, g_bgTexID);

   // Parameter texture: linear filtering, clamp ke tepi
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

   // Upload pixel data ke GPU
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                width, height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, data);

   // Bebaskan memori CPU setelah upload
   stbi_image_free(data);

   glBindTexture(GL_TEXTURE_2D, 0);
   g_bgLoaded = true;
}

// ════════════════════════════════════════════════════════════
//  UTILITY
// ════════════════════════════════════════════════════════════
void drawText(float x, float y, const std::string& text,
             float r=1, float g=1, float b=1) {
   glColor3f(r, g, b);
   glRasterPos2f(x, y);
   for (char c : text)
       glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
}

void drawTextBig(float x, float y, const std::string& text,
                float r=1, float g=1, float b=1) {
   glColor3f(r, g, b);
   glRasterPos2f(x, y);
   for (char c : text)
       glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

// ════════════════════════════════════════════════════════════
//  BACKGROUND — Gambar Texture
// ════════════════════════════════════════════════════════════
/**
* Menggambar gambar background sebagai quad penuh layar.
* Menggunakan texture mapping 2D.
* Jika gambar gagal di-load, fallback ke gradient gelap.
*/
void drawBackground() {
   float hw = WIN_W / 2.0f;
   float hh = WIN_H / 2.0f;

   if (g_bgLoaded) {
       // ── Gambar texture sebagai quad penuh layar ──
       glEnable(GL_TEXTURE_2D);
       glBindTexture(GL_TEXTURE_2D, g_bgTexID);

       // Warna putih agar warna texture tidak terpengaruh
       glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

       glBegin(GL_QUADS);
           // (texCoord U,V) → (vertex X,Y)
           glTexCoord2f(0.0f, 0.0f); glVertex2f(-hw, -hh); // kiri bawah
           glTexCoord2f(1.0f, 0.0f); glVertex2f( hw, -hh); // kanan bawah
           glTexCoord2f(1.0f, 1.0f); glVertex2f( hw,  hh); // kanan atas
           glTexCoord2f(0.0f, 1.0f); glVertex2f(-hw,  hh); // kiri atas
       glEnd();

       glBindTexture(GL_TEXTURE_2D, 0);
       glDisable(GL_TEXTURE_2D);

       // ── Overlay gelap semi-transparan ──
       // Agar polygon dan teks tetap terlihat jelas di atas gambar
       glBegin(GL_QUADS);
           glColor4f(0.0f, 0.0f, 0.05f, 0.45f);
           glVertex2f(-hw, -hh);
           glVertex2f( hw, -hh);
           glVertex2f( hw,  hh);
           glVertex2f(-hw,  hh);
       glEnd();

   } else {
       // ── Fallback: gradient biru gelap ──
       glBegin(GL_QUADS);
           glColor3f(0.02f, 0.04f, 0.12f);
           glVertex2f(-hw, -hh);
           glVertex2f( hw, -hh);
           glColor3f(0.06f, 0.12f, 0.28f);
           glVertex2f( hw,  hh);
           glVertex2f(-hw,  hh);
       glEnd();

       // Dot-grid fallback
       float offset = fmod(g_time * 8.0f, 40.0f);
       glPointSize(1.8f);
       glBegin(GL_POINTS);
       for (float x = -hw; x < hw; x += 40.0f) {
           for (float y = -hh; y < hh; y += 40.0f) {
               float px = x + offset;
               float py = y + offset * 0.5f;
               if (px > hw) px -= WIN_W;
               if (py > hh) py -= WIN_H;
               float alpha = 0.15f + 0.1f * sinf(g_time + x*0.02f + y*0.02f);
               glColor4f(0.4f, 0.7f, 1.0f, alpha);
               glVertex2f(px, py);
           }
       }
       glEnd();
   }
}

// ════════════════════════════════════════════════════════════
//  POLYGON — Bintang Segi-8
// ════════════════════════════════════════════════════════════
void starPoint(int idx, float outerR, float innerR, int N,
              float& px, float& py) {
   float step  = PI / N;
   float angle = idx * step - PI / 2.0f;
   float r     = (idx % 2 == 0) ? outerR : innerR;
   px = r * cosf(angle);
   py = r * sinf(angle);
}

void drawStarFilled(float outerR, float innerR, int N) {
   int totalPts = 2 * N;

   // Glow luar
   glLineWidth(14.0f);
   glBegin(GL_LINE_LOOP);
   for (int i = 0; i < totalPts; i++) {
       float px, py;
       starPoint(i, outerR + 10, innerR + 5, N, px, py);
       float t = (float)i / totalPts;
       glColor4f(0.1f + 0.2f * sinf(g_time + t * 6), 0.4f, 1.0f, 0.25f);
       glVertex2f(px, py);
   }
   glEnd();

   // Fill dengan TRIANGLE_FAN
   glBegin(GL_TRIANGLE_FAN);
       glColor4f(0.85f, 0.92f, 1.0f, 0.95f);
       glVertex2f(0.0f, 0.0f);
       for (int i = 0; i <= totalPts; i++) {
           float px, py;
           starPoint(i % totalPts, outerR, innerR, N, px, py);
           float t   = (float)i / totalPts;
           float hue = fmod(t + g_time * 0.05f, 1.0f);
           float r2  = 0.2f + 0.5f * sinf(hue * 2 * PI);
           float g2  = 0.5f + 0.4f * sinf(hue * 2 * PI + 2.1f);
           float b2  = 0.9f + 0.1f * sinf(hue * 2 * PI + 4.2f);
           glColor4f(r2, g2, b2, 0.92f);
           glVertex2f(px, py);
       }
   glEnd();

   // Outline
   glLineWidth(2.5f);
   glBegin(GL_LINE_LOOP);
   for (int i = 0; i < totalPts; i++) {
       float px, py;
       starPoint(i, outerR, innerR, N, px, py);
       glColor3f(0.7f + 0.3f * cosf(g_time + i), 0.9f, 1.0f);
       glVertex2f(px, py);
   }
   glEnd();

   // Titik sudut
   glPointSize(2.0f);
   glBegin(GL_POINTS);
   for (int i = 0; i < totalPts; i++) {
       float px, py;
       starPoint(i, outerR, innerR, N, px, py);
       glColor3f(i % 2 == 0 ? 1.0f : 0.5f,
                 i % 2 == 0 ? 1.0f : 0.8f,
                 1.0f);
       glVertex2f(px, py);
   }
   glEnd();
}

void drawAnnotations(float outerR, float innerR, int N) {
   int totalPts = 2 * N;
   char buf[64];
   for (int i = 0; i < totalPts; i++) {
       float px, py;
       starPoint(i, outerR, innerR, N, px, py);
       if (i % 2 == 0) {
           glLineWidth(0.5f);
           glBegin(GL_LINES);
               glColor4f(1, 1, 1, 0.15f);
               glVertex2f(0, 0); glVertex2f(px, py);
           glEnd();
           snprintf(buf, sizeof(buf), "P%d(%.0f,%.0f)", i / 2, px, py);
           drawText(px + 6, py + 4, buf, 0.7f, 0.95f, 1.0f);
       }
   }
   glLineWidth(0.8f);
   glBegin(GL_LINES);
       glColor4f(1, 1, 0, 0.12f);
       glVertex2f(-outerR, 0); glVertex2f(outerR, 0);
       glVertex2f(0, -outerR); glVertex2f(0, outerR);
   glEnd();
}

// ════════════════════════════════════════════════════════════
//  HUD
// ════════════════════════════════════════════════════════════
/**
* Panel teks semi-transparan agar tetap terbaca
* di atas gambar background apapun.
*/
void drawPanelBg(float x, float y, float w, float h) {
   glBegin(GL_QUADS);
       glColor4f(0.0f, 0.02f, 0.08f, 0.65f);
       glVertex2f(x,   y);
       glVertex2f(x+w, y);
       glVertex2f(x+w, y+h);
       glVertex2f(x,   y+h);
   glEnd();
   // border tipis
   glLineWidth(1.0f);
   glBegin(GL_LINE_LOOP);
       glColor4f(0.4f, 0.6f, 1.0f, 0.4f);
       glVertex2f(x,   y);
       glVertex2f(x+w, y);
       glVertex2f(x+w, y+h);
       glVertex2f(x,   y+h);
   glEnd();
}

void drawHUD() {
   float lh = 16.0f;
   float pw = 310.0f; // lebar panel

   // ── Panel kiri atas ──
   float px = -(WIN_W / 2.0f) + 8;
   float py =  (WIN_H / 2.0f) - 170;
   drawPanelBg(px - 4, py - 8, pw, 168);

   float ty = (WIN_H / 2.0f) - 24;
   drawTextBig(px, ty, "GRAFIKA KOMPUTER  UTS 2026", 0.9f, 0.95f, 1.0f); ty -= lh + 4;
   drawText(px, ty, "Univ. Sultan Ageng Tirtayasa", 0.7f, 0.85f, 1.0f);  ty -= lh;
   drawText(px, ty, "Kel.4 Kelas C | Bintang Segi-8", 0.5f, 1.0f, 0.8f); ty -= lh * 1.4f;

   char buf[128];
   snprintf(buf, sizeof(buf), "Translasi  : (%.1f, %.1f)", g_tx, g_ty);
   drawText(px, ty, buf, 1.0f, 0.9f, 0.4f); ty -= lh;
   snprintf(buf, sizeof(buf), "Rotasi     : %.1f deg", fmod(g_angle, 360.0f));
   drawText(px, ty, buf, 1.0f, 0.9f, 0.4f); ty -= lh;
   snprintf(buf, sizeof(buf), "Skala      : %.2fx", g_scale);
   drawText(px, ty, buf, 1.0f, 0.9f, 0.4f); ty -= lh;
   snprintf(buf, sizeof(buf), "Sisi       : %d  (R_out=%.0f, R_in=%.0f)",
            OUTER_POINTS * 2, OUTER_RADIUS, INNER_RADIUS);
   drawText(px, ty, buf, 0.7f, 1.0f, 0.85f); ty -= lh;
   drawText(px, ty, "Auto-Rotasi: " + std::string(g_autoRot ? "[ON] " : "[OFF]"),
            g_autoRot ? 0.2f : 0.6f, g_autoRot ? 1.0f : 0.6f, 0.5f);

   // ── Panel kontrol kanan bawah ──
   float rx = (WIN_W / 2.0f) - 238;
   float ry = -(WIN_H / 2.0f) + 8;
   drawPanelBg(rx - 4, ry - 4, 234, 118);

   float cy = ry + 108;
   drawText(rx, cy, "KONTROL KEYBOARD", 0.8f, 0.85f, 1.0f);  cy -= lh;
   drawText(rx, cy, "W/S/A/D : Translasi",   0.7f, 0.9f, 1.0f); cy -= lh;
   drawText(rx, cy, "Q / E   : Rotasi",      0.7f, 0.9f, 1.0f); cy -= lh;
   drawText(rx, cy, "+  / -  : Zoom",        0.7f, 0.9f, 1.0f); cy -= lh;
   drawText(rx, cy, "F1 : Auto-Rotasi | R : Reset | ESC : Keluar", 0.7f, 0.9f, 1.0f);

   // ── Label nama file background ──
   float bx = -(WIN_W / 2.0f) + 8;
   float by = -(WIN_H / 2.0f) + 8;
   std::string bgInfo = g_bgLoaded
       ? std::string("Background: ") + BG_FILE
       : "Background: [tidak ditemukan - fallback gradient]";
   drawText(bx, by, bgInfo, 0.5f, 0.7f, 0.5f);
}

// ════════════════════════════════════════════════════════════
//  DISPLAY CALLBACK
// ════════════════════════════════════════════════════════════
void display() {
   glClear(GL_COLOR_BUFFER_BIT);
   glLoadIdentity();

   // 1. Background gambar
   drawBackground();

   // 2. Polygon dengan transformasi
   glPushMatrix();
       glTranslatef(g_tx, g_ty, 0.0f);
       glRotatef(g_angle, 0.0f, 0.0f, 1.0f);
       glScalef(g_scale, g_scale, 1.0f);
       drawAnnotations(OUTER_RADIUS, INNER_RADIUS, OUTER_POINTS);
       drawStarFilled(OUTER_RADIUS, INNER_RADIUS, OUTER_POINTS);
   glPopMatrix();

   // 3. HUD
   drawHUD();

   glutSwapBuffers();
}

// ════════════════════════════════════════════════════════════
//  TIMER
// ════════════════════════════════════════════════════════════
void timer(int) {
   g_time += 0.016f;
   if (g_autoRot) g_angle += g_speed;
   glutPostRedisplay();
   glutTimerFunc(16, timer, 0);
}

// ════════════════════════════════════════════════════════════
//  KEYBOARD
// ════════════════════════════════════════════════════════════
const float TRANS_STEP = 10.0f;
const float ROT_STEP   = 5.0f;
const float SCALE_STEP = 0.05f;

void keyboard(unsigned char key, int, int) {
   switch (key) {
       case 'w': case 'W': g_ty += TRANS_STEP; break;
       case 's': case 'S': g_ty -= TRANS_STEP; break;
       case 'a': case 'A': g_tx -= TRANS_STEP; break;
       case 'd': case 'D': g_tx += TRANS_STEP; break;
       case 'q': case 'Q': g_angle -= ROT_STEP; break;
       case 'e': case 'E': g_angle += ROT_STEP; break;
       case '+': case '=': g_scale += SCALE_STEP; break;
       case '-': case '_': if (g_scale > 0.1f) g_scale -= SCALE_STEP; break;
       case 'r': case 'R': g_tx=0; g_ty=0; g_angle=0; g_scale=1.0f; break;
       case 27: exit(0);
   }
   glutPostRedisplay();
}

void keyboardSpecial(int key, int, int) {
   switch (key) {
       case GLUT_KEY_F1:    g_autoRot = !g_autoRot;  break;
       case GLUT_KEY_UP:    g_ty += TRANS_STEP;       break;
       case GLUT_KEY_DOWN:  g_ty -= TRANS_STEP;       break;
       case GLUT_KEY_LEFT:  g_tx -= TRANS_STEP;       break;
       case GLUT_KEY_RIGHT: g_tx += TRANS_STEP;       break;
   }
   glutPostRedisplay();
}

// ════════════════════════════════════════════════════════════
//  RESHAPE
// ════════════════════════════════════════════════════════════
void reshape(int w, int h) {
   if (h == 0) h = 1;
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(-w/2.0, w/2.0, -h/2.0, h/2.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

// ════════════════════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════════════════════
int main(int argc, char** argv) {
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
   glutInitWindowSize(WIN_W, WIN_H);
   glutInitWindowPosition(100, 80);
   glutCreateWindow("UTS Grafika Komputer - Kel.4 Poligon Bebas: Bintang Segi-8");

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_POINT_SMOOTH);
   glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

   // Load gambar background
   loadBackgroundTexture(BG_FILE);

   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(keyboardSpecial);
   glutTimerFunc(16, timer, 0);

   glutMainLoop();

   // Bersihkan texture saat keluar
   if (g_bgLoaded) glDeleteTextures(1, &g_bgTexID);
   return 0;
}