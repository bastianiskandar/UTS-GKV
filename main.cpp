#ifdef _WIN32
  #include <windows.h>
  #include <GL/freeglut.h>
#else
    #include <GL/freeglut.h>
#endif

#include <cmath>
#include <string>
#include <sstream>
#include <cstdio>

// ─── KONSTANTA ──────────────────────────────────────────────
const float PI      = 3.14159265358979f;
const int   WIN_W   = 900;
const int   WIN_H   = 700;

// ─── STATE APLIKASI ─────────────────────────────────────────
float  g_tx       = 0.0f;      // translasi X
float  g_ty       = 0.0f;      // translasi Y
float  g_angle    = 0.0f;      // sudut rotasi (derajat)
float  g_scale    = 1.0f;      // skala (zoom)
float  g_time     = 0.0f;      // timer animasi background
bool   g_autoRot  = false;     // rotasi otomatis (toggle F1)
float  g_speed    = 2.0f;      // kecepatan rotasi otomatis

// ─── PARAMETER POLYGON ──────────────────────────────────────
const int   OUTER_POINTS  = 8;       // jumlah sudut luar (oktagram)
const float OUTER_RADIUS  = 150.0f;  // jari-jari luar
const float INNER_RADIUS  = 65.0f;   // jari-jari dalam (untuk efek bintang)

// ════════════════════════════════════════════════════════════
//  UTILITY
// ════════════════════════════════════════════════════════════

/** Konversi derajat → radian */
inline float deg2rad(float d) { return d * PI / 180.0f; }

/** Gambar string teks di posisi (x,y) dalam koordinat layar */
void drawText(float x, float y, const std::string& text, float r=1,float g=1,float b=1) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
}

/** Gambar string dengan font lebih besar */
void drawTextBig(float x, float y, const std::string& text, float r=1,float g=1,float b=1) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

// ════════════════════════════════════════════════════════════
//  BACKGROUND — Gradient + Dot-Grid Animasi
// ════════════════════════════════════════════════════════════
void drawBackground() {
    // ── Gradient quad: biru malam → biru gelap ──
    glBegin(GL_QUADS);
        // bawah
        glColor3f(0.02f, 0.04f, 0.12f);
        glVertex2f(-(WIN_W/2.0f), -(WIN_H/2.0f));
        glVertex2f( (WIN_W/2.0f), -(WIN_H/2.0f));
        // atas
        glColor3f(0.06f, 0.12f, 0.28f);
        glVertex2f( (WIN_W/2.0f),  (WIN_H/2.0f));
        glVertex2f(-(WIN_W/2.0f),  (WIN_H/2.0f));
    glEnd();

    // ── Dot-grid animasi (bergerak perlahan) ──
    float offset = fmod(g_time * 8.0f, 40.0f);
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for (float x = -(WIN_W/2.0f); x < (WIN_W/2.0f); x += 40.0f) {
        for (float y = -(WIN_H/2.0f); y < (WIN_H/2.0f); y += 40.0f) {
            float px = x + offset;
            float py = y + offset * 0.5f;
            // Batas layar
            if (px > WIN_W/2.0f)  px -= WIN_W;
            if (py > WIN_H/2.0f)  py -= WIN_H;
            float alpha = 0.15f + 0.1f * sinf(g_time + x * 0.02f + y * 0.02f);
            glColor4f(0.4f, 0.7f, 1.0f, alpha);
            glVertex2f(px, py);
        }
    }
    glEnd();

    // ── Garis horizon dekoratif ──
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -5; i <= 5; i++) {
        float ly = i * 60.0f;
        float bright = 0.05f + 0.05f * sinf(g_time * 0.5f + i);
        glColor4f(0.3f, 0.5f, 0.9f, bright);
        glVertex2f(-(WIN_W/2.0f), ly);
        glVertex2f( (WIN_W/2.0f), ly);
    }
    glEnd();
}

// ════════════════════════════════════════════════════════════
//  POLYGON — Bintang Segi-8 (Octagram)
//
//  Algoritma:
//    Sebuah bintang N-sudut dibentuk dengan menginterpolasi
//    dua lingkaran konsentris (outer & inner). Setiap sudut
//    ke-i pada lingkaran luar dihubungkan dengan titik tengah
//    (inner) di antara dua sudut luar berurutan.
//
//    Total titik = 2 * N = 16 titik untuk N=8
//    Sudut tiap titik = 360° / (2*N) = 22.5°
// ════════════════════════════════════════════════════════════

/** Hitung satu titik pada bintang segi-N */
void starPoint(int idx, float outerR, float innerR, int N,
               float& px, float& py) {
    float step  = PI / N;               // setengah langkah antar sudut luar
    float angle = idx * step - PI/2.0f; // mulai dari atas (−90°)
    float r     = (idx % 2 == 0) ? outerR : innerR;
    px = r * cosf(angle);
    py = r * sinf(angle);
}

/** Gambar fill polygon bintang dengan warna gradient radial */
void drawStarFilled(float outerR, float innerR, int N) {
    int totalPts = 2 * N;

    // ── Shadow / glow luar ──
    glLineWidth(12.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < totalPts; i++) {
        float px, py;
        starPoint(i, outerR + 8, innerR + 4, N, px, py);
        float t = (float)i / totalPts;
        glColor4f(0.1f + 0.2f*sinf(g_time+t*6), 0.4f, 1.0f, 0.18f);
        glVertex2f(px, py);
    }
    glEnd();

    // ── Fill: gunakan GL_TRIANGLE_FAN dari pusat ──
    glBegin(GL_TRIANGLE_FAN);
        // pusat — warna putih kebiruan
        glColor3f(0.85f, 0.92f, 1.0f);
        glVertex2f(0.0f, 0.0f);

        for (int i = 0; i <= totalPts; i++) {
            float px, py;
            starPoint(i % totalPts, outerR, innerR, N, px, py);
            // Warna: gradasi dari cyan ke biru ungu berdasarkan sudut
            float t    = (float)i / totalPts;
            float hue  = fmod(t + g_time * 0.05f, 1.0f);
            // simpel HSV-like: biru → magenta
            float r2 = 0.2f + 0.5f * sinf(hue * 2 * PI);
            float g2 = 0.5f + 0.4f * sinf(hue * 2 * PI + 2.1f);
            float b2 = 0.9f + 0.1f * sinf(hue * 2 * PI + 4.2f);
            glColor3f(r2, g2, b2);
            glVertex2f(px, py);
        }
    glEnd();

    // ── Outline terang ──
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < totalPts; i++) {
        float px, py;
        starPoint(i, outerR, innerR, N, px, py);
        glColor3f(0.7f + 0.3f*cosf(g_time + i), 0.9f, 1.0f);
        glVertex2f(px, py);
    }
    glEnd();

    // ── Titik-titik sudut (highlight) ──
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < totalPts; i++) {
        float px, py;
        starPoint(i, outerR, innerR, N, px, py);
        if (i % 2 == 0) {
            // sudut luar — putih terang
            glColor3f(1.0f, 1.0f, 1.0f);
        } else {
            // sudut dalam — biru muda
            glColor3f(0.5f, 0.8f, 1.0f);
        }
        glVertex2f(px, py);
    }
    glEnd();
}

/** Gambar garis bantu / anotasi titik koordinat (opsional) */
void drawAnnotations(float outerR, float innerR, int N) {
    int totalPts = 2 * N;
    char buf[64];
    for (int i = 0; i < totalPts; i++) {
        float px, py;
        starPoint(i, outerR, innerR, N, px, py);
        // garis dari pusat ke sudut luar saja
        if (i % 2 == 0) {
            glLineWidth(0.5f);
            glBegin(GL_LINES);
                glColor4f(1,1,1,0.12f);
                glVertex2f(0,0);
                glVertex2f(px, py);
            glEnd();
            // label koordinat
            snprintf(buf, sizeof(buf), "P%d(%.0f,%.0f)", i/2, px, py);
            drawText(px + 6, py + 4, buf, 0.7f, 0.9f, 1.0f);
        }
    }
    // garis diameter referensi
    glLineWidth(0.8f);
    glBegin(GL_LINES);
        glColor4f(1,1,0,0.15f);
        glVertex2f(-outerR, 0); glVertex2f(outerR, 0);
        glVertex2f(0, -outerR); glVertex2f(0, outerR);
    glEnd();
}

// ════════════════════════════════════════════════════════════
//  HUD — Informasi di layar
// ════════════════════════════════════════════════════════════
void drawHUD() {
    // ── Panel info kiri atas ──
    float px = -(WIN_W/2.0f) + 12;
    float py =  (WIN_H/2.0f) - 20;
    float lh = 16; // line height

    drawTextBig(px, py, "GRAFIKA KOMPUTER — UTS 2026", 0.9f, 0.9f, 1.0f);
    py -= lh + 4;
    drawText(px, py, "Universitas Sultan Ageng Tirtayasa", 0.7f,0.8f,1.0f); py -= lh;
    drawText(px, py, "Kel. 4 Kelas C | Poligon Bebas: Bintang Segi-8", 0.7f,1.0f,0.8f); py -= lh*1.5f;

    char buf[128];
    snprintf(buf,sizeof(buf),"Translasi  : (%.1f, %.1f)", g_tx, g_ty);
    drawText(px, py, buf, 1.0f, 0.9f, 0.5f); py -= lh;
    snprintf(buf,sizeof(buf),"Rotasi     : %.1f deg", fmod(g_angle,360.0f));
    drawText(px, py, buf, 1.0f, 0.9f, 0.5f); py -= lh;
    snprintf(buf,sizeof(buf),"Skala      : %.2fx", g_scale);
    drawText(px, py, buf, 1.0f, 0.9f, 0.5f); py -= lh;
    snprintf(buf,sizeof(buf),"Jumlah Sisi: %d  (Outer R=%.0f, Inner R=%.0f)",
             OUTER_POINTS*2, OUTER_RADIUS, INNER_RADIUS);
    drawText(px, py, buf, 0.8f, 1.0f, 0.9f); py -= lh*1.5f;

    std::string autoStr = g_autoRot ? "[ON]" : "[OFF]";
    drawText(px, py, "Auto-Rotasi: "+autoStr, g_autoRot?0.3f:0.6f, g_autoRot?1.0f:0.6f, 0.6f);

    // ── Panel kontrol kanan bawah ──
    float rx = (WIN_W/2.0f) - 230;
    float ry = -(WIN_H/2.0f) + 130;
    drawText(rx, ry, "── KONTROL ──", 0.8f,0.8f,1.0f); ry += lh;
    drawText(rx, ry, "W/S/A/D : Translasi",  0.7f,0.9f,1.0f); ry += lh;
    drawText(rx, ry, "Q / E   : Rotasi",     0.7f,0.9f,1.0f); ry += lh;
    drawText(rx, ry, "+  / -  : Zoom",       0.7f,0.9f,1.0f); ry += lh;
    drawText(rx, ry, "F1      : Auto-Rotasi",0.7f,0.9f,1.0f); ry += lh;
    drawText(rx, ry, "R       : Reset",      0.7f,0.9f,1.0f); ry += lh;
    drawText(rx, ry, "ESC     : Keluar",     0.7f,0.9f,1.0f);
}

// ════════════════════════════════════════════════════════════
//  DISPLAY CALLBACK
// ════════════════════════════════════════════════════════════
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // ── 1. Background (tidak terpengaruh transformasi polygon) ──
    drawBackground();

    // ── 2. Terapkan transformasi polygon ──
    //    Urutan OpenGL: push → translate → rotate → scale → gambar → pop
    glPushMatrix();
        glTranslatef(g_tx, g_ty, 0.0f);                 // TRANSLASI
        glRotatef(g_angle, 0.0f, 0.0f, 1.0f);           // ROTASI (sumbu Z)
        glScalef(g_scale, g_scale, 1.0f);               // SKALA

        // ── 3. Gambar anotasi (di balik polygon) ──
        drawAnnotations(OUTER_RADIUS, INNER_RADIUS, OUTER_POINTS);

        // ── 4. Gambar polygon bintang ──
        drawStarFilled(OUTER_RADIUS, INNER_RADIUS, OUTER_POINTS);
    glPopMatrix();

    // ── 5. HUD (selalu di atas, koordinat tetap) ──
    drawHUD();

    glutSwapBuffers();
}

// ════════════════════════════════════════════════════════════
//  TIMER — Update animasi background & auto-rotasi
// ════════════════════════════════════════════════════════════
void timer(int /*val*/) {
    g_time += 0.016f; // ~60 fps
    if (g_autoRot) {
        g_angle += g_speed;
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ════════════════════════════════════════════════════════════
//  KEYBOARD CALLBACK
// ════════════════════════════════════════════════════════════
const float TRANS_STEP = 10.0f;
const float ROT_STEP   = 5.0f;
const float SCALE_STEP = 0.05f;

void keyboard(unsigned char key, int /*x*/, int /*y*/) {
    switch (key) {
        // ── Translasi ──
        case 'w': case 'W': g_ty += TRANS_STEP; break;
        case 's': case 'S': g_ty -= TRANS_STEP; break;
        case 'a': case 'A': g_tx -= TRANS_STEP; break;
        case 'd': case 'D': g_tx += TRANS_STEP; break;
        // ── Rotasi ──
        case 'q': case 'Q': g_angle -= ROT_STEP; break;
        case 'e': case 'E': g_angle += ROT_STEP; break;
        // ── Zoom ──
        case '+': case '=': g_scale += SCALE_STEP; break;
        case '-': case '_': if (g_scale > 0.1f) g_scale -= SCALE_STEP; break;
        // ── Reset ──
        case 'r': case 'R':
            g_tx = 0; g_ty = 0; g_angle = 0; g_scale = 1.0f;
            break;
        // ── ESC ──
        case 27: exit(0); break;
        default: break;
    }
    glutPostRedisplay();
}

void keyboardSpecial(int key, int /*x*/, int /*y*/) {
    switch (key) {
        case GLUT_KEY_F1: g_autoRot = !g_autoRot; break;
        case GLUT_KEY_UP:    g_ty += TRANS_STEP; break;
        case GLUT_KEY_DOWN:  g_ty -= TRANS_STEP; break;
        case GLUT_KEY_LEFT:  g_tx -= TRANS_STEP; break;
        case GLUT_KEY_RIGHT: g_tx += TRANS_STEP; break;
        default: break;
    }
    glutPostRedisplay();
}

// ════════════════════════════════════════════════════════════
//  RESHAPE CALLBACK
// ════════════════════════════════════════════════════════════
void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Koordinat berbasis pusat layar
    float hw = w / 2.0f, hh = h / 2.0f;
    gluOrtho2D(-hw, hw, -hh, hh);
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
    glutCreateWindow("UTS Grafika Komputer — Kel.4 Poligon Bebas: Bintang Segi-8");

    // Aktifkan blending untuk transparansi
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glClearColor(0.02f, 0.04f, 0.12f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboardSpecial);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
