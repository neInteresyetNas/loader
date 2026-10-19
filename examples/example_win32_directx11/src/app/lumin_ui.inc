// Lumin UI: intro -> license screen -> product menu -> success screen.
// Included from main.cpp inside the frame loop (needs: scale, ent, authA, style, io, cw, ch, win_w, win_h,
// font_medium, font_semibold, g_glass_renderer, hwnd, logged_in, gShakeX).
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
    static const char* kDemoKey    = "LUMIN-DEMO";      // the only key the demo accepts
    static const char* kActionText = "Inject";          // button label (stub)
    // -------------------------------------------------------------------------------------------

    static int   stage = 0;                 // 0 license, 1 menu, 2 success
    static int   sel = 0, lastSel = 0;
    static char  licKey[64] = "";
    static bool  licErr = false;
    static float shake = 0.0f;
    static bool  busy = false; static float busyT = 0.0f;
    static float okT = 0.0f;
    static int   lastStage = 0; static float subT = 1.0f;
    static float introT = 0.0f; static bool introSfx = false;
    static float swapT = 1.0f;
    static bool  closing = false; static float closeT = 0.0f;

    const float dt   = io.DeltaTime;
    const float tNow = (float)ImGui::GetTime();
    const float kPi   = 3.14159265f;

    const ImU32 ink  = IM_COL32(236, 237, 243, 255);
    const ImU32 soft = IM_COL32(150, 153, 168, 255);
    const ImU32 pink = IM_COL32(232, 104, 128, 255);
    const ImU32 okc  = IM_COL32(52, 209, 88, 255);

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

    // ---- state machines ----------------------------------------------------------------------
    if (stage != lastStage) { lastStage = stage; subT = 0.0f; }
    subT += dt * 3.2f; if (subT > 1.0f) subT = 1.0f;
    const float subA = sm(subT);

    if (introT < 3.6f) introT += dt;
    if (!introSfx && introT > 1.05f) { introSfx = true; Glass::PlaySfx(Glass::Sfx::Charm); }
    const float introGate = (stage == 0) ? sm((introT - 2.65f) / 0.75f) : 1.0f;

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
    //  INTRO  (compass draws itself, needle swings in, LUMIN types in, loading hairline)
    // =========================================================================================
    if (stage == 0) {
        const float sa = 1.0f - sm((introT - 2.75f) / 0.6f);
        if (sa > 0.003f) {
            ImDrawList* fd = ImGui::GetForegroundDrawList();
            const float t = introT;
            const float pcx = (float)cw * 0.5f, pcy = (float)ch * 0.5f;
            const float W = 440.0f, H = 330.0f;
            auto A = [&](ImU32 c, float a = 1.0f) {   // alpha for the splash only (not style.Alpha)
                float al = (float)((c >> 24) & 0xFF) / 255.0f * a * sa;
                al = sat01(al);
                return (ImU32)((c & 0x00FFFFFF) | ((ImU32)(al * 255.0f + 0.5f) << 24));
            };
            const float panelIn = eo3(t / 0.45f);
            fd->AddRectFilled(ImVec2(pcx - W * 0.5f, pcy - H * 0.5f), ImVec2(pcx + W * 0.5f, pcy + H * 0.5f),
                              A(IM_COL32(10, 11, 15, 246), panelIn), 30.0f);
            fd->AddRect(ImVec2(pcx - W * 0.5f, pcy - H * 0.5f), ImVec2(pcx + W * 0.5f, pcy + H * 0.5f),
                        A(IM_COL32(255, 255, 255, 26), panelIn), 30.0f, 0, 1.0f);

            const ImVec2 lc(pcx, pcy - 40.0f);
            const float R = 40.0f;

            // soft pulsing glow
            {
                float glowIn = sat01(t / 1.2f) * (0.82f + 0.18f * std::sin(t * 3.2f));
                for (int k = 0; k < 7; ++k)
                    fd->AddCircleFilled(lc, R + 6.0f + k * 9.0f, A(pink, 0.075f * (1.0f - k / 7.0f) * glowIn), 48);
            }
            // ripples
            for (int i = 0; i < 2; ++i) {
                float tt = (t - 0.15f - i * 0.38f) / 1.4f;
                if (tt > 0.0f && tt < 1.0f)
                    fd->AddCircle(lc, R * 0.7f + eo3(tt) * 120.0f, A(pink, (1.0f - tt) * 0.42f), 64, 1.6f);
            }
            // ring track + drawing ring
            const float p = eo3((t - 0.1f) / 1.0f);
            fd->AddCircle(lc, R, A(IM_COL32(255, 255, 255, 255), 0.11f), 96, 3.0f);
            if (p > 0.004f) {
                fd->PathArcTo(lc, R, -kPi * 0.5f, -kPi * 0.5f + 2.0f * kPi * p, 96);
                fd->PathStroke(A(IM_COL32(255, 255, 255, 240)), 0, 3.2f);
            }
            // ticks following the ring
            for (int i = 0; i < 12; ++i) {
                float f = (float)i / 12.0f;
                if (f > p) break;
                float ang = -kPi * 0.5f + f * 2.0f * kPi;
                bool major = (i % 3) == 0;
                float r0 = R + 8.0f, r1 = R + (major ? 15.0f : 12.0f);
                fd->AddLine(ImVec2(lc.x + std::cos(ang) * r0, lc.y + std::sin(ang) * r0),
                            ImVec2(lc.x + std::cos(ang) * r1, lc.y + std::sin(ang) * r1),
                            A(IM_COL32(255, 255, 255, 255), major ? 0.55f : 0.28f), 1.6f);
            }
            // needle: pops in, then swings with a damped overshoot
            if (t > 0.7f) {
                float u = t - 0.7f;
                float s = eob(u / 0.5f);
                float ang = (-kPi * 0.25f) + 2.7f * std::exp(-3.0f * u) * std::cos(9.5f * u);   // settles pointing NE
                ImVec2 dir(std::sin(ang), -std::cos(ang));
                ImVec2 per(-dir.y, dir.x);
                float L = R * 0.66f * s, w = R * 0.15f * s;
                ImVec2 tip(lc.x + dir.x * L, lc.y + dir.y * L), tail(lc.x - dir.x * L, lc.y - dir.y * L);
                ImVec2 a(lc.x + per.x * w, lc.y + per.y * w), b(lc.x - per.x * w, lc.y - per.y * w);
                fd->AddTriangleFilled(tip, a, b, A(IM_COL32(255, 255, 255, 255)));
                fd->AddTriangleFilled(tail, a, b, A(pink));
                fd->AddCircleFilled(lc, 3.4f * s, A(IM_COL32(10, 11, 15, 255)), 20);
                fd->AddCircle(lc, 3.4f * s, A(IM_COL32(255, 255, 255, 255), 0.9f), 20, 1.2f);
            }
            // LUMIN, letter by letter
            {
                static const char* letters[5] = { "L", "U", "M", "I", "N" };
                const float fs = 34.0f, spacing = 12.0f;
                float wl[5], total = 0.0f;
                for (int i = 0; i < 5; ++i) { wl[i] = font_semibold->CalcTextSizeA(fs, 99999.0f, 0.0f, letters[i]).x; total += wl[i]; }
                total += spacing * 4.0f;
                float x = pcx - total * 0.5f, y = pcy + 40.0f;
                for (int i = 0; i < 5; ++i) {
                    float e = eo3((t - 1.1f - i * 0.09f) / 0.5f);
                    if (e > 0.003f)
                        fd->AddText(font_semibold, fs, ImVec2(x, y + (1.0f - e) * 16.0f), A(IM_COL32(240, 241, 247, 255), e), letters[i]);
                    x += wl[i] + spacing;
                }
            }
            // loading hairline + caption
            {
                float bw = 170.0f, bx = pcx - bw * 0.5f, by = pcy + 108.0f;
                float fin = eo3((t - 1.3f) / 0.4f);
                float prog = eo3((t - 1.3f) / 1.4f);
                fd->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 3.0f), A(IM_COL32(255, 255, 255, 255), 0.10f * fin), 2.0f);
                fd->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw * prog, by + 3.0f), A(pink, fin), 2.0f);
                const char* cap = prog < 0.985f ? "Initializing" : "Ready";
                ImVec2 ts = font_medium->CalcTextSizeA(14.0f, 99999.0f, 0.0f, cap);
                fd->AddText(font_medium, 14.0f, ImVec2(pcx - ts.x * 0.5f, by + 12.0f), A(IM_COL32(150, 153, 168, 255), fin), cap);
            }
        }
    }

    // =========================================================================================
    //  LICENSE
    // =========================================================================================
    if (stage == 0) {
        if (Glass::BeginCard("license", Glass::Material::Regular, ImVec2(404.0f * scale, 0))) {
            const float fade = authA * subA * introGate * closeA;
            g_glass_renderer.SetSubmitFade(fade);
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float cwid = ImGui::GetContentRegionAvail().x;
            const float rise = (1.0f - introGate) * 22.0f;
            ImGui::Dummy(ImVec2(0, rise));

            // header: badge + title
            {
                ImVec2 c = ImGui::GetCursorScreenPos();
                const float bs = 56.0f;
                Glass::Primitive badge; badge.cx = c.x + bs * 0.5f; badge.cy = c.y + bs * 0.5f;
                badge.hw = bs * 0.5f; badge.hh = bs * 0.5f; badge.corner_radius = 16.0f; badge.fade = 1.0f;
                badge.material = Glass::Material::Accent;
                g_glass_renderer.Submit(badge);
                // slow breathing glow behind the badge
                for (int k = 0; k < 3; ++k)
                    dl->AddCircleFilled(ImVec2(badge.cx, badge.cy), bs * 0.62f + k * 7.0f,
                                        fa(pink, (0.10f - k * 0.03f) * (0.6f + 0.4f * std::sin(tNow * 1.8f))), 40);
                Glass::DrawIcon(dl, Glass::Icon::Compass, ImVec2(badge.cx, badge.cy), bs * 0.56f, fa(IM_COL32(255, 255, 255, 255)), 2.8f);
                dl->AddText(font_semibold, 24.0f, ImVec2(c.x + bs + 16.0f, c.y + 5.0f), fa(ink), "Lumin");
                dl->AddText(font_medium, 15.0f, ImVec2(c.x + bs + 16.0f, c.y + 34.0f), fa(soft), "License access");
                ImGui::Dummy(ImVec2(cwid, bs + 26.0f));
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ink);
            ImGui::TextUnformatted("License key");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 8));

            // key field on a thin glass pane
            bool enter = false;
            {
                const float gw = ImGui::GetContentRegionAvail().x, fh = 54.0f;
                ImVec2 gc = ImGui::GetCursorScreenPos();
                Glass::Primitive box; box.cx = gc.x + gw * 0.5f; box.cy = gc.y + fh * 0.5f;
                box.hw = gw * 0.5f; box.hh = fh * 0.5f; box.corner_radius = 17.0f; box.fade = 1.0f;
                box.material = Glass::Material::Thin;
                g_glass_renderer.Submit(box);
                dl->AddLine(ImVec2(gc.x + 16.0f, gc.y + 1.0f), ImVec2(gc.x + gw - 16.0f, gc.y + 1.0f), fa(IM_COL32(255, 255, 255, 20)), 1.0f);
                if (Glass::FieldRaw("lic", licKey, (int)sizeof(licKey), "XXXX-XXXX-XXXX", Glass::Icon::Key, false, fh))
                    licErr = false;
                if (Glass::FieldWasFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) enter = true;
                ImGui::SetCursorScreenPos(ImVec2(gc.x, gc.y + fh));
                ImGui::Dummy(ImVec2(gw, 0));
            }

            // error line (space is always reserved so the layout never jumps)
            {
                ImVec2 c = ImGui::GetCursorScreenPos();
                static float errA = 0.0f;
                errA += ((licErr ? 1.0f : 0.0f) - errA) * (dt * 12.0f > 1.0f ? 1.0f : dt * 12.0f);
                if (errA > 0.01f) {
                    float slide = (1.0f - errA) * 8.0f;
                    ImU32 col = fa(pink, errA);
                    Glass::DrawIcon(dl, Glass::Icon::Key, ImVec2(c.x + 10.0f, c.y + 14.0f - slide), 15.0f, col, 2.0f);
                    dl->AddText(font_medium, 15.0f, ImVec2(c.x + 28.0f, c.y + 5.0f - slide), col, "Invalid license key");
                }
                ImGui::Dummy(ImVec2(cwid, 30.0f));
            }

            bool go = Glass::Button("Activate", true, ImVec2(ImGui::GetContentRegionAvail().x, 50.0f));
            {
                ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
                float tw = ImGui::CalcTextSize("Activate").x;
                Glass::DrawIcon(dl, Glass::Icon::CheckCircle, ImVec2((mn.x + mx.x) * 0.5f - tw * 0.5f - 20.0f, (mn.y + mx.y) * 0.5f),
                                20.0f, fa(IM_COL32(255, 255, 255, 255)), 2.4f);
                // light sweep across the button every few seconds
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
            ImGui::Dummy(ImVec2(0, 6));

            if ((go || enter) && introGate > 0.9f) {
                if (_stricmp(licKey, kDemoKey) == 0) {
                    licErr = false; stage = 1; logged_in = true;
                    Glass::PlaySfx(Glass::Sfx::Success);
                    Glass::Notify(Glass::Icon::CheckCircle, IM_COL32(52, 209, 88, 255), "Lumin", "License activated", "Welcome back.");
                } else {
                    licErr = true; shake = 1.0f;
                    Glass::PlaySfx(Glass::Sfx::Error);
                }
            }
            g_glass_renderer.SetSubmitFade(1.0f);
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
            const float fade = authA * subA * closeA;
            g_glass_renderer.SetSubmitFade(fade);
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 o = ImGui::GetCursorScreenPos();
            const float innerW = ImGui::GetContentRegionAvail().x;
            const float innerH = ImGui::GetContentRegionAvail().y;
            const float slide = (1.0f - subA) * 22.0f;

            // title bar: breathing logo in the centre, window buttons on the right
            {
                ImVec2 lc(o.x + innerW * 0.5f, o.y + 16.0f);
                float br = 0.6f + 0.4f * std::sin(tNow * 1.8f);
                for (int k = 0; k < 4; ++k)
                    dl->AddCircleFilled(lc, 12.0f + k * 6.0f, fa(pink, (0.10f - k * 0.022f) * br), 32);
                Glass::DrawIcon(dl, Glass::Icon::Compass, lc, 28.0f, fa(IM_COL32(255, 255, 255, 255)), 2.6f);
                dl->AddLine(ImVec2(o.x - 8.0f, o.y + 42.0f), ImVec2(o.x + innerW + 8.0f, o.y + 42.0f), fa(IM_COL32(255, 255, 255, 22)), 1.0f);
            }

            if (stage == 1) {
                const float sideW = 168.0f;
                const float topY  = o.y + 56.0f;

                // sidebar (slides in from the left)
                static const Glass::Icon navIcons[3] = { Glass::Icon::Gamepad, Glass::Icon::Rocket, Glass::Icon::Crosshairs };
                const char* navLabels[3] = { prods[0].name, prods[1].name, prods[2].name };
                static const ImU32 navTints[3] = { IM_COL32(232, 104, 128, 255), IM_COL32(232, 104, 128, 255), IM_COL32(232, 104, 128, 255) };
                ImGui::SetCursorScreenPos(ImVec2(o.x - slide, topY));
                int r = Glass::SidebarNav("nav", navIcons, navLabels, navTints, 3, sel, sideW - 8.0f, 46.0f);
                if (r >= 0 && r != sel) { sel = r; busy = false; }

                // divider
                const float sepX = o.x + sideW + 10.0f;
                dl->AddLine(ImVec2(sepX, o.y + 54.0f), ImVec2(sepX, o.y + innerH - 4.0f), fa(IM_COL32(0, 0, 0, 28)), 1.0f);
                dl->AddLine(ImVec2(sepX + 1.0f, o.y + 54.0f), ImVec2(sepX + 1.0f, o.y + innerH - 4.0f), fa(IM_COL32(255, 255, 255, 36)), 1.0f);

                // user chip, bottom-left
                {
                    ImVec2 up(o.x + 4.0f - slide, o.y + innerH - 44.0f);
                    ImGui::SetCursorScreenPos(up);
                    Glass::Avatar("", 32.0f, IM_COL32(58, 60, 68, 255));
                    // online dot
                    dl->AddCircleFilled(ImVec2(up.x + 27.0f, up.y + 27.0f), 4.5f, fa(IM_COL32(20, 21, 26, 255)), 16);
                    dl->AddCircleFilled(ImVec2(up.x + 27.0f, up.y + 27.0f), 3.2f, fa(okc), 16);
                    dl->AddText(font_medium, 16.0f, ImVec2(up.x + 44.0f, up.y + 6.0f), fa(ink), kUserName);
                }

                // product information (slides in from the right)
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
            } else {
                // ---- success screen: ring draws, check strokes in, particles burst ----
                okT += dt;
                const ImVec2 cc(o.x + innerW * 0.5f, o.y + innerH * 0.5f - 98.0f);
                const float R = 30.0f;

                for (int k = 0; k < 5; ++k)
                    dl->AddCircleFilled(cc, R + 4.0f + k * 8.0f, fa(okc, 0.07f * (1.0f - k / 5.0f) * eo3(okT / 0.6f)), 40);
                float rp = eo3(okT / 0.6f);
                dl->AddCircle(cc, R, fa(IM_COL32(255, 255, 255, 255), 0.10f), 64, 3.0f);
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
                    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 3.0f), fa(IM_COL32(255, 255, 255, 255), 0.10f), 2.0f);
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
            style.Alpha = ent;
        }
        Glass::EndCard();
    }
}
