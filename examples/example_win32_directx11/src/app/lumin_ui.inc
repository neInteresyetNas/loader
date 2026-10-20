// Lumin UI: intro -> license screen -> product menu -> success screen.
// Included from main.cpp inside the frame loop (needs: scale, ent, authA, style, io, cw, ch, win_w, win_h,
// font_medium, font_semibold, g_glass_renderer, hwnd, logged_in, gShakeX, gUiCapture, appearance, profStatus, profTheme).
//
// UI ONLY. The "Inject" button is a visual stub: it plays a fake progress bar and shows the success
// screen. It does not touch any process or file.
{
    // ---- editable content --------------------------------------------------------------------
    struct Prod { const char* name; const char* game; const char* version; const char* status; ImU32 statusCol; int days; };
    static const Prod prods[3] = {
        { "Alt:V",    "alt:V Multiplayer", "release", "Available", IM_COL32(110, 231, 130, 255), 29 },
        { "Rage:MP",  "RAGE Multiplayer",  "release", "Updating",  IM_COL32(255, 200,  80, 255), 29 },
        { "Deadlock", "Deadlock",          "beta",    "Available", IM_COL32(110, 231, 130, 255), 29 },
    };
    static const char* kUserName   = "username";
    static const char* kUserTag    = "29 days";
    static const char* kDemoKey    = "LUMIN-DEMO";      // the only key the demo accepts
    static const char* kActionText = "Inject";          // button label (stub)
    static const char* kVersion    = "v1.2.0";
    // -------------------------------------------------------------------------------------------

    static int   stage = 0;                 // 0 license, 1 menu, 2 success
    static int   sel = 0, lastSel = 0;
    static char  licKey[64] = "";
    static bool  licErr = false;
    static bool  remember = false;
    static int   ver = 0;                   // license check: 0 idle, 1 checking, 2 accepted
    static float verT = 0.0f;
    static float shake = 0.0f;
    static bool  busy = false; static float busyT = 0.0f;
    static float okT = 0.0f;
    static int   lastStage = 0; static float subT = 0.0f;
    static float swapT = 1.0f;
    static bool  closing = false; static float closeT = 0.0f;
    static bool  loadedSaved = false, welcomePending = false;
    static float introT = 0.0f;
    // dynamic island
    static float islandClock = 0.0f, islandE = 0.0f, islandAppear = 0.0f, islandProg = 0.0f, islandDoneT = 0.0f;
    static int   islandState = 0;           // 0 offer, 1 installing, 2 done
    static bool  islandGone = false, islandHide = false;

    const float dt   = io.DeltaTime;
    const float tNow = (float)ImGui::GetTime();
    const float kPi  = 3.14159265f;
    gUiCapture = false;

    const ImU32 ink   = Glass::InkColor();       // follows the light / dark theme
    const ImU32 soft  = Glass::InkSoftColor();
    const ImU32 pink  = IM_COL32(232, 104, 128, 255);
    const ImU32 violet = IM_COL32(140, 110, 255, 255);
    const ImU32 okc   = IM_COL32(52, 209, 88, 255);
    const ImU32 white = IM_COL32(255, 255, 255, 255);

    // ---- tiny helpers ------------------------------------------------------------------------
    auto sat01 = [](float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };
    auto eo3   = [&](float x) { x = sat01(x); float u = 1.0f - x; return 1.0f - u * u * u; };
    auto sm    = [&](float x) { x = sat01(x); return x * x * (3.0f - 2.0f * x); };
    auto eob   = [&](float x) { x = sat01(x); const float c1 = 1.70158f, c3 = c1 + 1.0f; float u = x - 1.0f; return 1.0f + c3 * u * u * u + c1 * u * u; };
    // colour with extra alpha multiplier AND the current style alpha (raw draw-list colours ignore it)
    auto fa = [&](ImU32 c, float a = 1.0f) {
        float al = (float)((c >> 24) & 0xFF) / 255.0f * a * style.Alpha;
        al = sat01(al);
        return (ImU32)((c & 0x00FFFFFF) | ((ImU32)(al * 255.0f + 0.5f) << 24));
    };
    auto mulA = [&](ImU32 c, float k) {     // alpha only
        float al = (float)((c >> 24) & 0xFF) * k;
        al = al < 0.0f ? 0.0f : (al > 255.0f ? 255.0f : al);
        return (ImU32)((c & 0x00FFFFFF) | ((ImU32)(al + 0.5f) << 24));
    };
    auto mix = [&](ImU32 a, ImU32 b, float t) {
        t = sat01(t);
        auto chn = [&](int sh) { float x = (float)((a >> sh) & 0xFF), y = (float)((b >> sh) & 0xFF); return (ImU32)(x + (y - x) * t + 0.5f); };
        return (ImU32)((chn(0)) | (chn(8) << 8) | (chn(16) << 16) | (chn(24) << 24));
    };
    auto textW = [&](ImFont* f, float size, const char* t) { return f->CalcTextSizeA(size, 99999.0f, 0.0f, t).x; };

    // ---- the Lumin mark: two broken rings + a spark of light that lands in the gap -------------
    // p = build progress 0..1 (1 = finished logo). cm = main colour, ca = accent colour.
    auto drawMark = [&](ImDrawList* d, ImVec2 c, float R, float p, ImU32 cm, ImU32 ca, float flash) {
        const float gap = 0.52f, dirA = -kPi * 0.25f;
        float th = R * 0.15f; if (th < 1.8f) th = 1.8f;
        float po = eo3(p / 0.55f);
        if (po > 0.004f) {
            float a0 = dirA + gap, a1 = a0 + (2.0f * kPi - 2.0f * gap) * po;
            d->PathArcTo(c, R, a0, a1, 64);
            d->PathStroke(cm, 0, th);
        }
        float pi2 = eo3((p - 0.2f) / 0.5f);
        if (pi2 > 0.004f) {
            float g2 = 0.62f, b0 = dirA + kPi + g2, b1 = b0 + (2.0f * kPi - 2.0f * g2) * pi2;
            d->PathArcTo(c, R * 0.58f, b0, b1, 48);
            d->PathStroke(mulA(ca, 0.95f), 0, th * 0.62f);
        }
        float q = sat01((p - 0.55f) / 0.35f);
        if (q > 0.0f) {
            float dist = R * (1.0f + (1.0f - eo3(q)) * 2.6f);
            ImVec2 dp(c.x + std::cos(dirA) * dist, c.y + std::sin(dirA) * dist);
            float rr = R * 0.17f * (0.55f + 0.45f * eo3(q));
            for (int k = 0; k < 4; ++k)
                d->AddCircleFilled(dp, rr * (1.8f + k * 1.1f), mulA(ca, (0.16f - k * 0.035f) * (0.5f + 0.5f * q + flash)), 32);
            d->AddCircleFilled(dp, rr, cm, 24);
        }
        float o2 = eob((p - 0.8f) / 0.2f);
        if (o2 > 0.0f) d->AddCircleFilled(c, R * 0.12f * o2, ca, 20);
    };

    // ---- our own sidebar icons (hand-drawn vectors, one per product) ---------------------------
    auto prodIcon = [&](ImDrawList* d, int i, ImVec2 c, float s, ImU32 col) {
        const float th = 2.1f;
        if (i == 0) {                                   // Alt:V - hexagon with a V
            for (int k = 0; k < 6; ++k) {
                float a = -kPi * 0.5f + k * kPi / 3.0f;
                d->PathLineTo(ImVec2(c.x + std::cos(a) * s, c.y + std::sin(a) * s));
            }
            d->PathStroke(col, ImDrawFlags_Closed, th);
            ImVec2 l(c.x - 0.40f * s, c.y - 0.22f * s), m(c.x, c.y + 0.38f * s), r(c.x + 0.40f * s, c.y - 0.22f * s);
            d->AddLine(l, m, col, th); d->AddLine(m, r, col, th);
        } else if (i == 1) {                            // Rage:MP - lightning bolt
            static const float bx[6] = { 0.18f, -0.55f, -0.02f, -0.20f, 0.55f, 0.04f };
            static const float by[6] = { -1.00f, 0.12f, 0.12f, 1.00f, -0.18f, -0.18f };
            for (int k = 0; k < 6; ++k) d->PathLineTo(ImVec2(c.x + bx[k] * s, c.y + by[k] * s));
            d->PathFillConcave(col);
        } else {                                        // Deadlock - padlock
            d->AddRect(ImVec2(c.x - 0.78f * s, c.y - 0.05f * s), ImVec2(c.x + 0.78f * s, c.y + 0.95f * s), col, 0.22f * s, 0, th);
            d->PathArcTo(ImVec2(c.x, c.y - 0.05f * s), 0.5f * s, kPi, 2.0f * kPi, 16);
            d->PathStroke(col, 0, th);
            d->AddCircleFilled(ImVec2(c.x, c.y + 0.42f * s), 0.13f * s + 0.6f, col, 12);
        }
    };

    // ---- state machines ----------------------------------------------------------------------
    if (introT < 4.0f) introT += dt;
    // click / Enter skips the intro once it is running
    if (introT > 0.7f && introT < 3.0f && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
        introT = 3.0f;
    const float introGate = sm((introT - 3.0f) / 0.7f);   // the card stays hidden until the splash has faded

    // "Remember me": restore a saved key once, at launch
    if (!loadedSaved) {
        loadedSaved = true;
        if (LuminStore::Load(licKey, (int)sizeof(licKey))) {
            remember = true;
            if (_stricmp(licKey, kDemoKey) == 0) { stage = 1; logged_in = true; welcomePending = true; }
        }
    }
    if (welcomePending && introT > 3.4f) {
        welcomePending = false;
        Glass::Notify(Glass::Icon::CheckCircle, IM_COL32(52, 209, 88, 255), "Lumin", "Welcome back", "You are signed in.");
    }

    if (stage != lastStage) { lastStage = stage; subT = 0.0f; }
    subT += dt * 3.2f; if (subT > 1.0f) subT = 1.0f;
    const float subA = sm(subT);

    if (sel != lastSel) { lastSel = sel; swapT = 0.0f; }
    swapT += dt * 2.6f; if (swapT > 1.0f) swapT = 1.0f;

    if (closing) { closeT += dt * 4.5f; if (closeT >= 1.0f) { closeT = 1.0f; ::PostMessage(hwnd, WM_CLOSE, 0, 0); closing = false; } }
    const float closeA = 1.0f - sm(closeT);

    shake -= dt * 2.2f; if (shake < 0.0f) shake = 0.0f;
    gShakeX = std::sin(shake * 38.0f) * 11.0f * shake;

    auto lumin_center = [&](const char* t, ImU32 col) {
        float tw = ImGui::CalcTextSize(t).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - tw) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(t);
        ImGui::PopStyleColor();
    };

    // =========================================================================================
    //  INTRO: the mark builds itself, a spark lands in the gap, LUMIN rises letter by letter
    // =========================================================================================
    {
        const float sa = 1.0f - sm((introT - 3.0f) / 0.6f);
        if (sa > 0.003f) {
            ImDrawList* fd = ImGui::GetForegroundDrawList();
            const float t = introT;
            const float pcx = (float)cw * 0.5f, pcy = (float)ch * 0.5f;
            auto A = [&](ImU32 c, float a = 1.0f) {   // splash alpha only (the splash ignores style.Alpha)
                float al = (float)((c >> 24) & 0xFF) / 255.0f * a * sa;
                al = sat01(al);
                return (ImU32)((c & 0x00FFFFFF) | ((ImU32)(al * 255.0f + 0.5f) << 24));
            };
            const float panelIn = eo3(t / 0.55f);
            const float pscale = (0.955f + 0.045f * panelIn) + 0.03f * sm((t - 3.0f) / 0.6f);
            const float W = 470.0f * pscale, H = 350.0f * pscale;
            const ImVec2 pmin(pcx - W * 0.5f, pcy - H * 0.5f), pmax(pcx + W * 0.5f, pcy + H * 0.5f);

            // soft drop shadow + panel
            for (int k = 3; k >= 1; --k)
                fd->AddRectFilled(ImVec2(pmin.x - k * 5.0f, pmin.y - k * 3.0f), ImVec2(pmax.x + k * 5.0f, pmax.y + k * 7.0f),
                                  A(IM_COL32(0, 0, 0, 255), 0.07f * panelIn), 34.0f + k * 4.0f);
            fd->AddRectFilled(pmin, pmax, A(IM_COL32(9, 10, 14, 247), panelIn), 32.0f);

            const ImVec2 lc(pcx, pcy - 52.0f);
            const float R = 44.0f;

            // drifting aurora inside the panel (clipped to it)
            fd->PushClipRect(pmin, pmax, true);
            {
                float gi = sat01(t / 1.0f);
                ImVec2 g1(pcx - 80.0f + 26.0f * std::sin(t * 0.9f), lc.y + 6.0f + 12.0f * std::cos(t * 0.7f));
                ImVec2 g2(pcx + 90.0f + 24.0f * std::cos(t * 0.8f), lc.y + 40.0f + 14.0f * std::sin(t * 0.6f));
                for (int k = 0; k < 9; ++k) {
                    float f = 1.0f - k / 9.0f;
                    fd->AddCircleFilled(g1, 36.0f + k * 12.0f, A(pink, 0.030f * f * gi), 48);
                    fd->AddCircleFilled(g2, 30.0f + k * 11.0f, A(violet, 0.026f * f * gi), 48);
                }
            }
            fd->PopClipRect();
            fd->AddRect(pmin, pmax, A(white, 0.10f * panelIn), 32.0f, 0, 1.0f);
            fd->AddLine(ImVec2(pmin.x + 34.0f, pmin.y + 1.0f), ImVec2(pmax.x - 34.0f, pmin.y + 1.0f), A(white, 0.16f * panelIn), 1.0f);

            // orbiting specks of light
            {
                float vis = sat01((t - 0.9f) / 0.8f);
                for (int i = 0; i < 12; ++i) {
                    float sp = 0.25f + (i % 4) * 0.09f;
                    float ang = i * 0.5236f * 1.7f + t * sp * (i % 2 ? 1.0f : -1.0f);
                    float rad = R * (1.85f + (i % 5) * 0.28f);
                    float tw = 0.5f + 0.5f * std::sin(t * 2.0f + i * 1.3f);
                    fd->AddCircleFilled(ImVec2(lc.x + std::cos(ang) * rad, lc.y + std::sin(ang) * rad * 0.8f), 1.1f + (i % 3) * 0.5f,
                                        A((i % 3 == 0) ? violet : pink, (0.18f + 0.40f * tw) * vis), 10);
                }
            }

            // the mark
            const float p = sat01((t - 0.15f) / 1.45f);
            const float tLand = 0.15f + 1.45f * 0.9f;
            float flash = (t > tLand) ? std::exp(-5.0f * (t - tLand)) * 1.4f : 0.0f;
            {   // glow behind the mark
                float gin = sat01(t / 1.1f) * (0.80f + 0.20f * std::sin(t * 3.0f)) + flash * 0.5f;
                for (int k = 0; k < 7; ++k)
                    fd->AddCircleFilled(lc, R + 6.0f + k * 9.0f, A(pink, 0.070f * (1.0f - k / 7.0f) * gin), 48);
            }
            drawMark(fd, lc, R, p, A(white, 0.97f), A(pink), flash);
            {   // shockwave when the spark lands
                float tt = (t - tLand) / 0.8f;
                if (tt > 0.0f && tt < 1.0f) {
                    fd->AddCircle(lc, R * 1.05f + eo3(tt) * R * 2.8f, A(pink, (1.0f - tt) * 0.45f), 72, 2.0f);
                    fd->AddCircle(lc, R * 1.05f + eo3(tt - 0.12f) * R * 2.1f, A(white, (1.0f - tt) * 0.16f), 72, 1.2f);
                }
            }

            // LUMIN, letter by letter (tracking tightens as it settles)
            {
                static const char* letters[5] = { "L", "U", "M", "I", "N" };
                const float fs = 36.0f;
                const float spacing = 11.0f + 12.0f * (1.0f - eo3((t - 1.2f) / 1.1f));
                float wl[5], total = 0.0f;
                for (int i = 0; i < 5; ++i) { wl[i] = textW(font_semibold, fs, letters[i]); total += wl[i]; }
                total += spacing * 4.0f;
                float x = pcx - total * 0.5f, y = pcy + 34.0f;
                for (int i = 0; i < 5; ++i) {
                    float e = eo3((t - 1.2f - i * 0.085f) / 0.55f);
                    if (e > 0.003f)
                        fd->AddText(font_semibold, fs, ImVec2(x, y + (1.0f - e) * 20.0f), A(IM_COL32(242, 243, 248, 255), e), letters[i]);
                    x += wl[i] + spacing;
                }
            }
            // progress hairline + caption
            {
                const float bw = 196.0f, bx = pcx - bw * 0.5f, by = pcy + 104.0f;
                float fin = eo3((t - 1.45f) / 0.4f);
                float prog = eo3((t - 1.45f) / 1.55f);
                fd->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 3.0f), A(white, 0.10f * fin), 2.0f);
                if (prog > 0.003f)
                    fd->AddRectFilledMultiColor(ImVec2(bx, by), ImVec2(bx + bw * prog, by + 3.0f), A(pink, fin), A(violet, fin), A(violet, fin), A(pink, fin));
                const char* cap = prog < 0.34f ? "Loading modules" : (prog < 0.72f ? "Checking environment" : (prog < 0.985f ? "Almost ready" : "Ready"));
                float cw3 = textW(font_medium, 14.0f, cap);
                fd->AddText(font_medium, 14.0f, ImVec2(pcx - cw3 * 0.5f, by + 13.0f), A(IM_COL32(150, 153, 168, 255), fin), cap);
            }
        }
    }

    // =========================================================================================
    //  LICENSE
    // =========================================================================================
    if (stage == 0) {
        if (Glass::BeginCard("license", Glass::Material::Regular, ImVec2(404.0f * scale, 0))) {
            const float fade = authA * subA * closeA * introGate;
            g_glass_renderer.SetSubmitFade(fade);
            ImGui::BeginDisabled(introGate < 0.95f);      // no clicks while the splash is up (alpha is set right after, so no dimming)
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float cwid = ImGui::GetContentRegionAvail().x;
            const float rise = (1.0f - introGate) * 20.0f;
            ImGui::Dummy(ImVec2(0, rise));

            // header: our mark on a glass badge, title, subtitle (centred)
            {
                ImVec2 c0 = ImGui::GetCursorScreenPos();
                const float bs = 78.0f;
                const ImVec2 bc(c0.x + cwid * 0.5f, c0.y + bs * 0.5f + 2.0f);
                float breath = 0.6f + 0.4f * std::sin(tNow * 1.8f);
                for (int k = 0; k < 5; ++k)
                    dl->AddCircleFilled(bc, bs * 0.56f + k * 9.0f, fa(pink, (0.12f - k * 0.022f) * breath), 56);
                Glass::Primitive badge; badge.cx = bc.x; badge.cy = bc.y;
                badge.hw = bs * 0.5f; badge.hh = bs * 0.5f; badge.corner_radius = 25.0f; badge.fade = 1.0f;
                badge.material = Glass::Material::Accent;
                g_glass_renderer.Submit(badge);
                drawMark(dl, bc, 25.0f, 1.0f, fa(white), fa(white, 0.62f), 0.0f);
                const char* title = "Lumin";
                const char* sub   = "Enter your license key to continue";
                float tw1 = textW(font_semibold, 28.0f, title), tw2 = textW(font_medium, 15.0f, sub);
                dl->AddText(font_semibold, 28.0f, ImVec2(c0.x + (cwid - tw1) * 0.5f, c0.y + bs + 14.0f), fa(ink), title);
                dl->AddText(font_medium, 15.0f, ImVec2(c0.x + (cwid - tw2) * 0.5f, c0.y + bs + 52.0f), fa(soft), sub);
                ImGui::Dummy(ImVec2(cwid, bs + 52.0f + 18.0f + 24.0f));
            }

            const bool locked = (ver != 0);

            // key field (the library field floats its label above the text once something is typed)
            bool enter = false;
            {
                const float gw = ImGui::GetContentRegionAvail().x, fh = 64.0f;
                ImVec2 gc = ImGui::GetCursorScreenPos();
                ImGui::BeginDisabled(locked);
                if (Glass::FieldRaw("lic", licKey, (int)sizeof(licKey), "License key", Glass::Icon::Key, false, fh))
                    licErr = false;
                if (Glass::FieldWasFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) enter = true;
                ImGui::EndDisabled();
                if (ver == 1) {   // indeterminate sweep along the bottom edge while the key is being checked
                    float u = std::fmod(tNow * 0.85f, 1.0f), segW = gw * 0.34f;
                    float x = gc.x - segW + (gw + segW) * u;
                    dl->PushClipRect(ImVec2(gc.x + 14.0f, gc.y), ImVec2(gc.x + gw - 14.0f, gc.y + fh), true);
                    dl->AddRectFilledMultiColor(ImVec2(x, gc.y + fh - 3.0f), ImVec2(x + segW * 0.5f, gc.y + fh - 1.0f),
                                                fa(pink, 0.0f), fa(pink, 0.95f), fa(pink, 0.95f), fa(pink, 0.0f));
                    dl->AddRectFilledMultiColor(ImVec2(x + segW * 0.5f, gc.y + fh - 3.0f), ImVec2(x + segW, gc.y + fh - 1.0f),
                                                fa(pink, 0.95f), fa(pink, 0.0f), fa(pink, 0.0f), fa(pink, 0.95f));
                    dl->PopClipRect();
                }
                ImGui::Dummy(ImVec2(gw, 12.0f));
            }

            // row: remember me (left) + error message (right)
            {
                ImVec2 rp = ImGui::GetCursorScreenPos();
                ImGui::BeginDisabled(locked);
                Glass::Checkbox("Remember me", &remember);
                ImGui::EndDisabled();
                static float errA = 0.0f;
                errA += ((licErr ? 1.0f : 0.0f) - errA) * (dt * 12.0f > 1.0f ? 1.0f : dt * 12.0f);
                if (errA > 0.01f) {
                    const char* em = "Invalid license key";
                    float ew = textW(font_medium, 15.0f, em);
                    float slide = (1.0f - errA) * 8.0f;
                    ImU32 col = fa(pink, errA);
                    float ex = rp.x + cwid - ew - slide;
                    Glass::DrawIcon(dl, Glass::Icon::Key, ImVec2(ex - 16.0f, rp.y + 12.0f), 15.0f, col, 2.0f);
                    dl->AddText(font_medium, 15.0f, ImVec2(ex, rp.y + 3.0f), col, em);
                }
                ImGui::SetCursorScreenPos(ImVec2(rp.x, rp.y + 24.0f));
                ImGui::Dummy(ImVec2(cwid, 18.0f));
            }

            // activate button: label + icon follow the check (Activate -> Checking -> Activated)
            const char* btnLabel = ver == 0 ? "Activate" : (ver == 1 ? "Checking license..." : "Activated");
            bool go = Glass::Button(btnLabel, true, ImVec2(ImGui::GetContentRegionAvail().x, 52.0f));
            {
                ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
                float tw = ImGui::CalcTextSize(btnLabel).x;
                ImVec2 ic((mn.x + mx.x) * 0.5f - tw * 0.5f - 20.0f, (mn.y + mx.y) * 0.5f);
                if (ver == 0) {
                    Glass::DrawIcon(dl, Glass::Icon::CheckCircle, ic, 20.0f, fa(white), 2.4f);
                } else if (ver == 1) {      // spinner
                    float a0 = tNow * 7.0f;
                    dl->AddCircle(ic, 8.5f, fa(white, 0.25f), 24, 2.2f);
                    dl->PathArcTo(ic, 8.5f, a0, a0 + kPi * 1.25f, 20);
                    dl->PathStroke(fa(white), 0, 2.4f);
                } else {                    // check mark pops in
                    float q = eob(verT / 0.32f);
                    dl->AddCircleFilled(ic, 10.0f * q, fa(white), 24);
                    float s1 = sat01((verT - 0.08f) / 0.12f), s2 = sat01((verT - 0.18f) / 0.16f);
                    ImVec2 a(ic.x - 4.5f, ic.y + 0.5f), b(ic.x - 1.0f, ic.y + 4.0f), c2(ic.x + 5.0f, ic.y - 3.5f);
                    if (s1 > 0.0f) dl->AddLine(a, ImVec2(a.x + (b.x - a.x) * s1, a.y + (b.y - a.y) * s1), fa(pink), 2.4f);
                    if (s2 > 0.0f) dl->AddLine(b, ImVec2(b.x + (c2.x - b.x) * s2, b.y + (c2.y - b.y) * s2), fa(pink), 2.4f);
                }
                if (ver == 0) {   // light sweep across the idle button every few seconds
                    float ph = std::fmod(tNow, 3.6f) / 3.6f;
                    if (ph < 0.4f) {
                        float u = ph / 0.4f;
                        float bw2 = 26.0f, x = mn.x - 2.0f * bw2 + (mx.x - mn.x + 4.0f * bw2) * u;
                        dl->PushClipRect(ImVec2(mn.x + 4.0f, mn.y + 4.0f), ImVec2(mx.x - 4.0f, mx.y - 4.0f), true);
                        dl->AddRectFilledMultiColor(ImVec2(x, mn.y), ImVec2(x + bw2, mx.y), fa(IM_COL32(255, 255, 255, 0)), fa(IM_COL32(255, 255, 255, 46)),
                                                    fa(IM_COL32(255, 255, 255, 46)), fa(IM_COL32(255, 255, 255, 0)));
                        dl->AddRectFilledMultiColor(ImVec2(x + bw2, mn.y), ImVec2(x + 2.0f * bw2, mx.y), fa(IM_COL32(255, 255, 255, 46)), fa(IM_COL32(255, 255, 255, 0)),
                                                    fa(IM_COL32(255, 255, 255, 0)), fa(IM_COL32(255, 255, 255, 46)));
                        dl->PopClipRect();
                    }
                }
            }
            // footer
            {
                ImVec2 fp = ImGui::GetCursorScreenPos();
                char foot[64]; std::snprintf(foot, sizeof(foot), "Lumin  -  %s", kVersion);
                float fw = textW(font_medium, 13.0f, foot);
                dl->AddText(font_medium, 13.0f, ImVec2(fp.x + (cwid - fw) * 0.5f, fp.y + 14.0f), fa(soft, 0.7f), foot);
                ImGui::Dummy(ImVec2(cwid, 38.0f));
            }

            // ---- the check itself: a short, smooth "verifying" phase, then the result ----
            if (ver == 0 && (go || enter)) {
                ver = 1; verT = 0.0f; licErr = false;
                Glass::PlaySfx(Glass::Sfx::Tap);
            } else if (ver == 1) {
                verT += dt;
                if (verT >= 1.45f) {
                    if (_stricmp(licKey, kDemoKey) == 0) {
                        ver = 2; verT = 0.0f;
                        Glass::PlaySfx(Glass::Sfx::Success);
                    } else {
                        ver = 0; licErr = true; shake = 1.0f;
                        Glass::PlaySfx(Glass::Sfx::Error);
                    }
                }
            } else if (ver == 2) {
                verT += dt;
                if (verT >= 0.7f) {
                    ver = 0; stage = 1; logged_in = true;
                    if (remember) LuminStore::Save(licKey); else LuminStore::Clear();
                    Glass::Notify(Glass::Icon::CheckCircle, IM_COL32(52, 209, 88, 255), "Lumin", "License activated", "Welcome back.");
                }
            }
            g_glass_renderer.SetSubmitFade(1.0f);
            ImGui::EndDisabled();
            style.Alpha = ent;
        }
        Glass::EndCard();
    } else {
        // =====================================================================================
        //  MENU / SUCCESS
        // =====================================================================================
        const float dashW = (float)win_w - 92.0f;
        const float dashH = (float)win_h - 92.0f;
        if (Glass::BeginCard("dash", Glass::Material::Regular, ImVec2(dashW, dashH))) {
            const float fade = authA * subA * closeA * introGate;
            g_glass_renderer.SetSubmitFade(fade);
            ImGui::BeginDisabled(introGate < 0.95f);      // no clicks while the splash is up (alpha is set right after, so no dimming)
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 o = ImGui::GetCursorScreenPos();
            const float innerW = ImGui::GetContentRegionAvail().x;
            const float innerH = ImGui::GetContentRegionAvail().y;
            const float slide = (1.0f - subA) * 22.0f;

            // title bar: our mark (fades out while the island is open), window buttons on the right
            {
                ImVec2 lc(o.x + innerW * 0.5f, o.y + 17.0f);
                float la = 1.0f - 0.92f * islandE;
                drawMark(dl, lc, 11.0f, 1.0f, fa(ink, la), fa(pink, la), 0.0f);
                dl->AddLine(ImVec2(o.x - 8.0f, o.y + 42.0f), ImVec2(o.x + innerW + 8.0f, o.y + 42.0f), fa(ink, 0.10f), 1.0f);
            }

            if (stage == 1) {
                const float sideW = 168.0f;
                const float topY  = o.y + 56.0f;

                // ---------------- sidebar with our own icons ----------------
                {
                    const float itemH = 46.0f, navW = sideW - 8.0f;
                    const ImVec2 nb(o.x - slide, topY);
                    ImGui::PushID("lumin_nav");
                    ImGuiID sid = ImGui::GetID("sel");
                    Glass::Spring& sp = Glass::g->springs().Get((uint32_t)sid, 10, Glass::SpringStyle::Critical, (float)sel);
                    sp.target = (float)sel; sp.Tick(dt);
                    Glass::Spring& tail = Glass::g->springs().Get((uint32_t)sid, 11, Glass::SpringStyle::Overdamped, sp.x);
                    tail.target = sp.x; tail.Tick(dt);
                    {   // the glass "pill" that follows the selection
                        float phw = navW * 0.5f, phh = itemH * 0.5f - 3.0f;
                        float cyH = nb.y + (sp.x + 0.5f) * itemH, cyT = nb.y + (tail.x + 0.5f) * itemH;
                        if (std::fabs(cyH - cyT) > 1.5f) {
                            Glass::SubmitBlob(nb.x + phw, cyH, phw, phh, nb.x + phw, cyT, phw, phh, 12.0f, 16.0f, 1.0f, Glass::Material::Thin);
                        } else {
                            Glass::Primitive hl; hl.cx = nb.x + phw; hl.cy = cyH; hl.hw = phw; hl.hh = phh;
                            hl.corner_radius = 12.0f; hl.fade = 1.0f; hl.material = Glass::Material::Thin;
                            g_glass_renderer.Submit(hl);
                        }
                    }
                    for (int i = 0; i < 3; ++i) {
                        ImGui::PushID(i);
                        ImVec2 ip(nb.x, nb.y + i * itemH);
                        ImGui::SetCursorScreenPos(ip);
                        ImGui::InvisibleButton("##nav", ImVec2(navW, itemH));
                        bool hov = ImGui::IsItemHovered(), act = ImGui::IsItemActive();
                        if (ImGui::IsItemDeactivated() && hov && i != sel) { sel = i; busy = false; Glass::PlaySfx(Glass::Sfx::TabChange); }
                        uint32_t nid = (uint32_t)ImGui::GetID("##nav");
                        Glass::Spring& prs = Glass::g->springs().Get(nid, 18, Glass::SpringStyle::Critical, 1.0f);
                        prs.target = act ? 0.92f : 1.0f; prs.Tick(dt);
                        Glass::Spring& lif = Glass::g->springs().Get(nid, 19, Glass::SpringStyle::Critical, 0.0f);
                        lif.target = (hov || act) ? 1.0f : 0.0f; lif.Tick(dt);
                        Glass::Spring& pop = Glass::g->springs().Get(nid, 20, Glass::SpringStyle::Critical, 0.0f);
                        pop.target = (i == sel) ? 1.0f : 0.0f; pop.Tick(dt);
                        float press = prs.x < 0.78f ? 0.78f : (prs.x > 1.12f ? 1.12f : prs.x);
                        float lift  = sat01(lif.x);
                        float popv  = pop.x < 0.0f ? 0.0f : (pop.x > 1.2f ? 1.2f : pop.x);
                        float iScale = press * (1.0f + 0.10f * lift + 0.10f * popv);
                        float shift  = lift * 5.0f;
                        ImU32 col = mix(soft, ink, lift > popv ? lift : popv);
                        prodIcon(dl, i, ImVec2(ip.x + 25.0f + shift, ip.y + itemH * 0.5f), 10.5f * iScale, fa(col));
                        ImVec2 lts = ImGui::CalcTextSize(prods[i].name);
                        dl->AddText(ImVec2(ip.x + 52.0f + shift, ip.y + (itemH - lts.y) * 0.5f), fa(col), prods[i].name);
                        if (i == sel)
                            Glass::DrawIcon(dl, Glass::Icon::ChevronR, ImVec2(ip.x + navW - 12.0f + lift * -3.0f, ip.y + itemH * 0.5f),
                                            13.0f * (1.0f + 0.08f * popv), fa(IM_COL32(150, 150, 160, 200)), 2.0f);
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }

                // divider
                const float sepX = o.x + sideW + 10.0f;
                dl->AddLine(ImVec2(sepX, o.y + 54.0f), ImVec2(sepX, o.y + innerH - 4.0f), fa(IM_COL32(0, 0, 0, 28)), 1.0f);
                dl->AddLine(ImVec2(sepX + 1.0f, o.y + 54.0f), ImVec2(sepX + 1.0f, o.y + innerH - 4.0f), fa(IM_COL32(255, 255, 255, 36)), 1.0f);

                // ---------------- product information (slides in from the right) ----------------
                const float cx  = o.x + sideW + 32.0f + slide;
                const float cw2 = innerW - sideW - 32.0f;
                dl->AddText(font_medium, 17.0f, ImVec2(cx, topY + 2.0f), fa(ink), "Product Information");

                const Prod& p = prods[sel];
                const float rowH = 38.0f, panelY = topY + 34.0f, panelH = rowH * 4 + 16.0f;
                {
                    Glass::Primitive pane; pane.cx = cx + cw2 * 0.5f; pane.cy = panelY + panelH * 0.5f;
                    pane.hw = cw2 * 0.5f; pane.hh = panelH * 0.5f; pane.corner_radius = 16.0f; pane.fade = 1.0f;
                    pane.material = Glass::Material::Thin;
                    g_glass_renderer.Submit(pane);
                }
                const char* labels[4] = { "Game", "Version", "Status", "Till Expiry" };
                for (int i = 0; i < 4; ++i) {
                    // each row swaps in with a small stagger when the product changes
                    float e  = eo3((swapT - i * 0.12f) / 0.55f);
                    float ry = panelY + 8.0f + rowH * i + (rowH - 17.0f) * 0.5f;
                    dl->AddText(font_medium, 16.0f, ImVec2(cx + 22.0f, ry), fa(soft), labels[i]);

                    char buf[48];
                    const char* val = buf;
                    if (i == 0) val = p.game;
                    else if (i == 1) val = p.version;
                    else if (i == 2) val = p.status;
                    else { std::snprintf(buf, sizeof(buf), "%d days", (int)(p.days * e + 0.5f)); }

                    ImVec2 vs = font_medium->CalcTextSizeA(16.0f, 99999.0f, 0.0f, val);
                    float vx = cx + cw2 - 22.0f - vs.x + (1.0f - e) * 18.0f;
                    ImU32 vcol = (i == 2) ? p.statusCol : ink;
                    dl->AddText(font_medium, 16.0f, ImVec2(vx, ry), fa(vcol, e), val);

                    if (i == 2) {   // pulsing status dot
                        ImVec2 dc(vx - 14.0f, ry + 9.0f);
                        float ph = std::fmod(tNow, 1.7f) / 1.7f;
                        dl->AddCircleFilled(dc, 3.6f, fa(p.statusCol, e), 16);
                        dl->AddCircle(dc, 3.6f + ph * 8.0f, fa(p.statusCol, (1.0f - ph) * 0.55f * e), 24, 1.4f);
                    }
                }

                // action button (stub)
                ImGui::SetCursorScreenPos(ImVec2(cx, panelY + panelH + 14.0f));
                const bool disabled = busy || strcmp(p.status, "Available") != 0;
                if (!busy) {
                    bool clicked = Glass::Button(kActionText, true, ImVec2(cw2, 48.0f));
                    ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
                    if (!disabled) {   // light sweep
                        float ph = std::fmod(tNow, 3.4f) / 3.4f;
                        if (ph < 0.38f) {
                            float u = ph / 0.38f;
                            float bw2 = 34.0f, x = mn.x - 2.0f * bw2 + (mx.x - mn.x + 4.0f * bw2) * u;
                            dl->PushClipRect(ImVec2(mn.x + 4.0f, mn.y + 4.0f), ImVec2(mx.x - 4.0f, mx.y - 4.0f), true);
                            dl->AddRectFilledMultiColor(ImVec2(x, mn.y), ImVec2(x + bw2, mx.y), fa(IM_COL32(255, 255, 255, 0)), fa(IM_COL32(255, 255, 255, 40)),
                                                        fa(IM_COL32(255, 255, 255, 40)), fa(IM_COL32(255, 255, 255, 0)));
                            dl->AddRectFilledMultiColor(ImVec2(x + bw2, mn.y), ImVec2(x + 2.0f * bw2, mx.y), fa(IM_COL32(255, 255, 255, 40)), fa(IM_COL32(255, 255, 255, 0)),
                                                        fa(IM_COL32(255, 255, 255, 0)), fa(IM_COL32(255, 255, 255, 40)));
                            dl->PopClipRect();
                        }
                    }
                    if (clicked) {
                        if (!disabled) { busy = true; busyT = 0.0f; }
                        else Glass::Notify(Glass::Icon::WarnTriangle, IM_COL32(255, 159, 10, 255), "Lumin", "Not available", "This product is not ready yet.");
                    }
                } else {
                    busyT += dt * 0.55f;
                    float pr = busyT > 1.0f ? 1.0f : busyT;
                    Glass::ProgressBar(pr, ImVec2(cw2, 10.0f));
                    ImGui::Dummy(ImVec2(0, 4));
                    const char* msg = pr < 0.34f ? "Checking license..." : (pr < 0.72f ? "Preparing..." : "Almost done...");
                    ImVec2 ts = ImGui::CalcTextSize(msg);
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cw2 - ts.x) * 0.5f);
                    ImGui::PushStyleColor(ImGuiCol_Text, soft);
                    ImGui::TextUnformatted(msg);
                    ImGui::PopStyleColor();
                    if (busyT >= 1.1f) { busy = false; stage = 2; okT = 0.0f; Glass::PlaySfx(Glass::Sfx::Success); }
                }

                // ---------------- profile row (bottom-left): theme + sign out only ----------------
                {
                    float pbH = 56.0f, pbX = o.x + 6.0f - slide, pbW = sideW - 12.0f;
                    float pbY = o.y + innerH - pbH - 6.0f;
                    Glass::ProfileAction pa = Glass::ProfileMenu("acct", kUserName, kUserTag, "U", IM_COL32(232, 104, 128, 255),
                                                                 pbX, pbY, pbW, &profStatus, &profTheme, fade, false);
                    if (pa == Glass::ProfileAction::SignOut) {
                        stage = 0; logged_in = false; licKey[0] = 0; licErr = false; ver = 0; busy = false;
                        islandClock = 0.0f; islandE = 0.0f; islandAppear = 0.0f; islandState = 0; islandProg = 0.0f;
                        islandGone = false; islandHide = false;
                        LuminStore::Clear();
                    } else if (pa == Glass::ProfileAction::ThemeChanged) {
                        appearance = (profTheme == 1) ? 1 : 0;      // 0 dark, 1 light
                        Glass::Notify(Glass::Icon::Display, IM_COL32(94, 92, 230, 255), "Appearance", profTheme == 1 ? "Light" : "Dark", "Theme updated.");
                    }
                }

                // ---------------- Dynamic Island: "Update available" -> Install -> Updated ----------------
                if (!islandGone && subA > 0.6f && introGate > 0.95f) {
                    islandClock += dt;
                    const ImVec2 wp = ImGui::GetWindowPos();
                    const float wW = ImGui::GetWindowWidth();
                    const float icx = wp.x + wW * 0.5f;
                    const float itop = wp.y - 34.0f;

                    islandAppear += (((islandClock > 0.7f && !islandHide) ? 1.0f : 0.0f) - islandAppear) * sat01(dt * 6.0f);
                    float w = 176.0f + (336.0f - 176.0f) * islandE;
                    float h = 32.0f + (68.0f - 32.0f) * islandE;
                    float yo = (1.0f - islandAppear) * -22.0f;
                    ImVec2 pmin(icx - w * 0.5f, itop + yo), pmax(icx + w * 0.5f, itop + h + yo);

                    bool hovI = islandAppear > 0.6f && ImGui::IsMouseHoveringRect(ImVec2(pmin.x - 6.0f, pmin.y - 6.0f), ImVec2(pmax.x + 6.0f, pmax.y + 6.0f));
                    if (hovI) gUiCapture = true;
                    bool wantExp = hovI || (islandClock > 1.3f && islandClock < 6.3f) || islandState != 0;
                    islandE += ((wantExp ? 1.0f : 0.0f) - islandE) * sat01(dt * 8.0f);
                    const float e = islandE, ap = islandAppear, cA = (1.0f - e) * (1.0f - e), eA = e * e;
                    const float mid = (pmin.y + pmax.y) * 0.5f;

                    ImDrawList* fd = ImGui::GetForegroundDrawList();
                    for (int k = 3; k >= 1; --k)
                        fd->AddRectFilled(ImVec2(pmin.x - k * 3.0f, pmin.y - k * 2.0f), ImVec2(pmax.x + k * 3.0f, pmax.y + k * 4.0f),
                                          fa(IM_COL32(0, 0, 0, 255), 0.05f * ap), h * 0.5f + k * 3.0f);
                    fd->AddRectFilled(pmin, pmax, fa(IM_COL32(8, 8, 11, 250), ap), h * 0.5f);
                    fd->AddRect(pmin, pmax, fa(white, 0.10f * ap), h * 0.5f, 0, 1.0f);
                    if (e > 0.02f) fd->AddRect(pmin, pmax, fa(pink, 0.30f * e * ap), h * 0.5f, 0, 1.2f);

                    fd->PushClipRect(pmin, pmax, true);
                    // compact: icon + text + pulsing dot
                    if (cA > 0.01f) {
                        float k = cA * ap;
                        fd->AddCircleFilled(ImVec2(pmin.x + 18.0f, mid), 9.5f, fa(pink, k), 20);
                        Glass::DrawIcon(fd, Glass::Icon::Download, ImVec2(pmin.x + 18.0f, mid), 11.0f, fa(white, k), 2.0f);
                        fd->AddText(font_medium, 13.5f, ImVec2(pmin.x + 35.0f, mid - 8.0f), fa(white, k), "Update available");
                        float ph = std::fmod(tNow, 1.6f) / 1.6f;
                        ImVec2 dc(pmax.x - 14.0f, mid);
                        fd->AddCircleFilled(dc, 3.2f, fa(pink, k), 12);
                        fd->AddCircle(dc, 3.2f + ph * 6.0f, fa(pink, (1.0f - ph) * 0.6f * k), 20, 1.2f);
                    }
                    // expanded
                    if (eA > 0.01f) {
                        float k = eA * ap;
                        const ImVec2 ic(pmin.x + 14.0f + 17.0f, mid);
                        fd->AddCircleFilled(ic, 17.0f, fa(pink, k), 28);
                        Glass::DrawIcon(fd, islandState == 2 ? Glass::Icon::Check : Glass::Icon::Download, ic, 17.0f, fa(white, k), 2.2f);
                        const float tx = pmin.x + 14.0f + 34.0f + 12.0f;
                        const char* title = islandState == 0 ? "Update available" : (islandState == 1 ? "Updating..." : "Update installed");
                        fd->AddText(font_semibold, 15.5f, ImVec2(tx, mid - 21.0f), fa(white, k), title);
                        ImVec2 bmin(pmax.x - 14.0f - 74.0f, mid - 15.0f), bmax(pmax.x - 14.0f, mid + 15.0f);
                        if (islandState == 0) {
                            char sub[64]; std::snprintf(sub, sizeof(sub), "Lumin %s is ready", kVersion);
                            fd->AddText(font_medium, 12.5f, ImVec2(tx, mid + 2.0f), fa(IM_COL32(150, 153, 168, 255), k), sub);
                            bool hovB = ImGui::IsMouseHoveringRect(bmin, bmax);
                            fd->AddRectFilled(bmin, bmax, fa(hovB ? IM_COL32(246, 128, 150, 255) : pink, k), 15.0f);
                            float iw = textW(font_medium, 14.0f, "Install");
                            fd->AddText(font_medium, 14.0f, ImVec2((bmin.x + bmax.x) * 0.5f - iw * 0.5f, mid - 8.0f), fa(white, k), "Install");
                            if (hovB) gUiCapture = true;
                            if (hovB && e > 0.85f && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                islandState = 1; islandProg = 0.0f;
                                Glass::PlaySfx(Glass::Sfx::Tap);
                            }
                        } else if (islandState == 1) {
                            islandProg += dt * 0.45f;
                            float pr = sat01(islandProg);
                            float bx0 = tx, bx1 = bmin.x - 6.0f, by0 = mid + 5.0f;
                            fd->AddRectFilled(ImVec2(bx0, by0), ImVec2(bx1, by0 + 4.0f), fa(white, 0.14f * k), 2.0f);
                            fd->AddRectFilled(ImVec2(bx0, by0), ImVec2(bx0 + (bx1 - bx0) * pr, by0 + 4.0f), fa(pink, k), 2.0f);
                            char pct[16]; std::snprintf(pct, sizeof(pct), "%d%%", (int)(pr * 100.0f));
                            float pw = textW(font_medium, 14.0f, pct);
                            fd->AddText(font_medium, 14.0f, ImVec2(pmax.x - 16.0f - pw, mid - 8.0f), fa(IM_COL32(200, 202, 212, 255), k), pct);
                            if (islandProg >= 1.0f) { islandState = 2; islandDoneT = 0.0f; Glass::PlaySfx(Glass::Sfx::Success); }
                        } else {
                            fd->AddText(font_medium, 12.5f, ImVec2(tx, mid + 2.0f), fa(IM_COL32(150, 153, 168, 255), k), "You are up to date");
                            islandDoneT += dt;
                            if (islandDoneT > 1.4f) islandHide = true;
                        }
                    }
                    fd->PopClipRect();
                    if (islandHide && islandAppear < 0.02f) islandGone = true;
                }
            } else {
                // ---- success screen: ring draws, check strokes in, particles burst ----
                okT += dt;
                const ImVec2 cc(o.x + innerW * 0.5f, o.y + innerH * 0.5f - 98.0f);
                const float R = 30.0f;

                for (int k = 0; k < 5; ++k)
                    dl->AddCircleFilled(cc, R + 4.0f + k * 8.0f, fa(okc, 0.07f * (1.0f - k / 5.0f) * eo3(okT / 0.6f)), 40);
                float rp = eo3(okT / 0.6f);
                dl->AddCircle(cc, R, fa(ink, 0.12f), 64, 3.0f);
                if (rp > 0.004f) {
                    dl->PathArcTo(cc, R, -kPi * 0.5f, -kPi * 0.5f + 2.0f * kPi * rp, 64);
                    dl->PathStroke(fa(okc), 0, 3.2f);
                }
                {   // check mark, two strokes
                    float q = eo3((okT - 0.5f) / 0.4f);
                    ImVec2 a(cc.x - 11.0f, cc.y + 1.0f), b(cc.x - 3.0f, cc.y + 9.0f), c2(cc.x + 12.0f, cc.y - 8.0f);
                    float s1 = sat01(q / 0.4f), s2 = sat01((q - 0.4f) / 0.6f);
                    if (s1 > 0.0f) dl->AddLine(a, ImVec2(a.x + (b.x - a.x) * s1, a.y + (b.y - a.y) * s1), fa(okc), 3.6f);
                    if (s2 > 0.0f) dl->AddLine(b, ImVec2(b.x + (c2.x - b.x) * s2, b.y + (c2.y - b.y) * s2), fa(okc), 3.6f);
                }
                {   // particles
                    float tt = sat01((okT - 0.55f) / 1.0f);
                    if (tt > 0.0f && tt < 1.0f) {
                        for (int i = 0; i < 16; ++i) {
                            float ang = (float)i / 16.0f * 2.0f * kPi + (i % 2) * 0.2f;
                            float spd = 46.0f + (float)((i * 7) % 5) * 12.0f;
                            float rr = R + 8.0f + eo3(tt) * spd;
                            ImVec2 pp(cc.x + std::cos(ang) * rr, cc.y + std::sin(ang) * rr);
                            dl->AddCircleFilled(pp, 1.0f + 2.2f * (1.0f - tt), fa((i % 3 == 0) ? pink : okc, 1.0f - tt), 12);
                        }
                    }
                }

                float tin = eo3((okT - 0.35f) / 0.5f);
                ImGui::SetCursorScreenPos(ImVec2(o.x, o.y + innerH * 0.5f - 44.0f + (1.0f - tin) * 12.0f));
                ImGui::PushFont(font_semibold);
                lumin_center("Successfully!", fa(ink, tin));
                ImGui::PopFont();
                ImGui::Dummy(ImVec2(0, 8));
                lumin_center("Now you can go back to the game and have fun!", fa(soft, tin));
                lumin_center("This window will close automatically.", fa(soft, tin));

                // countdown hairline
                {
                    float bw = 120.0f, bx = o.x + (innerW - bw) * 0.5f, by = o.y + innerH - 26.0f;
                    float left = 1.0f - sat01(okT / 3.2f);
                    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 3.0f), fa(ink, 0.10f), 2.0f);
                    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw * left, by + 3.0f), fa(pink), 2.0f);
                }
                if (okT > 3.2f) stage = 1;     // demo: return to the menu instead of quitting
            }

            // window buttons (drawn last so they stay clickable); close plays a fade-out first
            {
                float tlx = o.x + innerW - (3 * 13.0f + 2 * 8.0f) - 4.0f;
                int tl = Glass::TrafficLights("wtl", tlx, o.y + 8.0f);
                if (tl == 0) closing = true;
                else if (tl == 1) ::ShowWindow(hwnd, SW_MINIMIZE);
            }
            g_glass_renderer.SetSubmitFade(1.0f);
            ImGui::EndDisabled();
            style.Alpha = ent;
        }
        Glass::EndCard();
    }
}
