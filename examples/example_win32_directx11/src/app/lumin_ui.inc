// ECLIPSE - loader UI: intro splash -> license screen -> RAGE:MP product screen -> success screen.
// Included from main.cpp inside the frame loop (needs: scale, ent, authA, style, io, cw, ch, win_w, win_h,
// font_medium, font_semibold, g_glass_renderer, hwnd, logged_in, gShakeX, gUiCapture, appearance, profStatus, profTheme).
//
// UI ONLY. The "Inject" button is a visual stub: a fake progress bar and a success screen.
// It does not touch any process or file. The ping indicator is simulated too.
{
    // ---- editable content --------------------------------------------------------------------
    static const char* kBrand      = "ECLIPSE";
    static const char* kProdName   = "CS2";
    static const char* kProdGame   = "Counter-Strike 2";
    static const char* kProdVer    = "release";
    static const char* kProdStatus = "Available";
    static const ImU32 kStatusCol  = IM_COL32(110, 231, 130, 255);
    static const wchar_t* kProcName = L"cs2.exe";       // process the loader looks for (visual demo: nothing is injected)
    static const int   kDays       = 29;
    static const char* kUserName   = "username";
    static const char* kUserTag    = "29 days";
    static const char* kDemoKey    = "ECLIPSE-DEMO";    // demo key (the old LUMIN-DEMO is still accepted)
    static const char* kActionText = "Inject";          // button label (stub)
    static const char* kVersion    = "v2.0.0";
    // -------------------------------------------------------------------------------------------

    static int   stage = 0;                 // 0 license, 1 menu, 2 success
    static char  licKey[64] = "";
    static bool  licErr = false;
    static bool  remember = false;
    static int   ver = 0;                   // license check: 0 idle, 1 checking, 2 accepted
    static float verT = 0.0f;
    static float shake = 0.0f;
    static bool  busy = false; static float busyT = 0.0f; static int busyPhase = 0; static bool notFound = false; static float nfE = 0.0f; static float pollT = 0.0f, gameE = 0.0f; static bool gameUp = false;
    static float okT = 0.0f;
    static int   lastStage = 0; static float subT = 0.0f;
    static float loadT = 0.0f;              // skeleton -> values reveal
    static float contentT = 1.0f; static int lastGrp = 0;
    static int   fails = 0; static float lockT = 0.0f;
    static char  errMsg[40] = "Invalid license key";
    static bool  pasteActive = false; static int pasteIdx = 0, pasteLen = 0; static float pasteAcc = 0.0f, pastedFlash = 0.0f;
    static char  pasteBuf[48] = "";
    static bool  autoFocus = false;
    static float licW = 0.0f, licH = 0.0f;  // last measured size of the license card (the intro grows into it)
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
    const bool lightMode = (appearance == 1);

    const ImU32 ink    = Glass::InkColor();       // follows the light / dark theme
    const ImU32 soft   = Glass::InkSoftColor();
    const ImU32 pink   = IM_COL32(232, 38, 66, 255);
    const ImU32 violet = IM_COL32(168, 34, 112, 255);
    const ImU32 amber  = IM_COL32(255, 122, 46, 255);
    const ImU32 okc    = IM_COL32(52, 209, 88, 255);
    const ImU32 white  = IM_COL32(255, 255, 255, 255);

    const float dashW = (float)win_w - 92.0f;
    const float dashH = (float)win_h - 92.0f;
    if (licW <= 0.0f) { licW = 404.0f * scale; licH = 470.0f * scale; }

    // ---- tiny helpers ------------------------------------------------------------------------
    auto sat01 = [](float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };
    auto eo3   = [&](float x) { x = sat01(x); float u = 1.0f - x; return 1.0f - u * u * u; };
    auto sm    = [&](float x) { x = sat01(x); return x * x * (3.0f - 2.0f * x); };
    auto eob   = [&](float x) { x = sat01(x); const float c1 = 1.70158f, c3 = c1 + 1.0f; float u = x - 1.0f; return 1.0f + c3 * u * u * u + c1 * u * u; };
    auto lerpf = [](float a, float b, float t) { return a + (b - a) * t; };
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
    // letter-spaced text, centred on cx
    auto trackedC = [&](ImDrawList* d, ImFont* f, float size, float cx, float y, const char* t, float sp, ImU32 col) {
        float total = 0.0f; int n = 0;
        for (const char* q = t; *q; ++q) { char b[2] = { *q, 0 }; total += textW(f, size, b) + sp; ++n; }
        total -= sp;
        float x = cx - total * 0.5f;
        for (const char* q = t; *q; ++q) { char b[2] = { *q, 0 }; d->AddText(f, size, ImVec2(x, y), col, b); x += textW(f, size, b) + sp; }
    };

    // ---- the Eclipse mark: a chrome blade star (4 long + 4 short faceted spikes, crimson gem, red glow) ----
    // yaw tilts it like a turntable (3D), rot spins it, glow scales the red aura.
    auto drawStar = [&](ImDrawList* d, ImVec2 c, float R, float yaw, float rot, float al, float glow) {
        if (R < 1.0f || al < 0.004f) return;
        const float xs = std::max(0.42f, std::fabs(std::cos(yaw))), sy = std::sin(yaw);
        const float beat = 0.78f + 0.22f * std::sin(tNow * 2.2f);
        for (int k = 0; k < 7; ++k)
            d->AddCircleFilled(c, R * (0.85f + k * 0.24f), mulA(pink, 0.065f * (1.0f - k / 7.0f) * glow * beat * al), 40);
        const int NS = 8;
        const float r0 = R * 0.20f, hb = kPi / (float)NS;
        auto pt = [&](float ang, float rad, float ox, float oy) { return ImVec2(c.x + std::cos(ang) * rad * xs + ox, c.y + std::sin(ang) * rad + oy); };
        // thickness: a stack of dark copies pushed back
        for (int l = 5; l >= 1; --l) {
            float ox = -sy * R * 0.028f * l, oy = R * 0.016f * l;
            ImU32 col = mulA(mix(IM_COL32(26, 4, 10, 255), IM_COL32(112, 14, 32, 255), 1.0f - l / 5.0f), al);
            for (int sp = 0; sp < NS; ++sp) {
                float ang = rot + sp * 2.0f * hb, len = (sp % 2 == 0) ? R : R * 0.60f;
                d->AddTriangleFilled(pt(ang, len, ox, oy), pt(ang - hb, r0, ox, oy), pt(ang + hb, r0, ox, oy), col);
                d->AddTriangleFilled(pt(0, 0, ox, oy), pt(ang - hb, r0, ox, oy), pt(ang + hb, r0, ox, oy), col);
            }
        }
        // chrome facets: each spike = a lit and a shaded half, brightness follows a moving light
        const float lightA = -2.3f + std::sin(tNow * 0.55f) * 0.7f - sy * 0.6f;
        for (int sp = 0; sp < NS; ++sp) {
            float ang = rot + sp * 2.0f * hb, len = (sp % 2 == 0) ? R : R * 0.60f;
            ImVec2 tip = pt(ang, len, 0, 0), bl = pt(ang - hb, r0, 0, 0), br = pt(ang + hb, r0, 0, 0), ce = pt(0, 0, 0, 0);
            for (int side = 0; side < 2; ++side) {
                float fang = ang + (side ? hb : -hb) * 0.5f;
                float v = 0.5f + 0.5f * std::cos(fang - lightA);
                v = v * v * (sp % 2 == 0 ? 1.0f : 0.8f);
                ImU32 col = mix(IM_COL32(18, 19, 30, 255), IM_COL32(244, 247, 255, 255), v);
                col = mix(col, IM_COL32(130, 12, 34, 255), (1.0f - v) * 0.38f);
                d->AddTriangleFilled(tip, side ? br : bl, ce, mulA(col, al));
            }
            d->AddLine(ce, tip, mulA(white, 0.30f * al), 1.0f);
            d->AddLine(tip, bl, mulA(white, 0.10f * al), 1.0f);
            d->AddLine(tip, br, mulA(IM_COL32(0, 0, 0, 255), 0.35f * al), 1.0f);
        }
        // crimson gem in the middle
        const float gr = R * 0.15f;
        d->AddCircleFilled(c, gr * 1.55f, mulA(IM_COL32(14, 8, 14, 255), al), 24);
        d->AddCircleFilled(c, gr * 1.1f, mulA(pink, al), 24);
        d->AddCircleFilled(ImVec2(c.x - gr * 0.28f, c.y - gr * 0.32f), gr * 0.38f, mulA(white, 0.85f * al), 12);
        d->AddCircle(c, gr * 1.55f, mulA(white, 0.35f * al), 24, 1.0f);
    };
    // brand mark with a build-in animation (p = 0..1): the star spins in and settles
    auto drawEclipse = [&](ImDrawList* d, ImVec2 c, float R, float p, float al) {
        const float ps = eob(p / 0.75f);
        drawStar(d, c, R * 1.08f * ps, std::sin(tNow * 0.5f) * 0.18f, (1.0f - eo3(p)) * kPi * 0.75f + tNow * 0.05f, al, ps);
    };

    // ---- 3D product emblem: the chrome blade star hovering over a red-lit floor, turning with the cursor, embers rising ----
    auto drawEmblem = [&](ImDrawList* d, ImVec2 ec, float Rs, float px, float py, float al) {
        const float bob = std::sin(tNow * 1.3f) * 4.0f;
        const ImVec2 sc(ec.x + px * 6.0f, ec.y + bob + py * 5.0f);
        {   // floor shadow / red under-glow (shrinks while the star floats up)
            float sh = 1.0f - bob * 0.02f;
            for (int k = 0; k < 5; ++k)
                d->AddEllipseFilled(ImVec2(ec.x + px * 2.0f, ec.y + Rs * 1.75f), ImVec2(Rs * (1.30f - k * 0.18f) * sh, 9.0f - k * 1.5f), mulA(pink, 0.075f * al));
        }
        drawStar(d, sc, Rs * 1.32f, std::sin(tNow * 0.55f) * 0.5f + px * 0.55f, tNow * 0.12f + px * 0.25f, al, 1.0f);
        // embers drifting up from the star
        for (int i = 0; i < 16; ++i) {
            float ph = std::fmod(tNow * (0.10f + 0.05f * (i % 4)) + i * 0.37f, 1.0f);
            float x = sc.x + std::sin(i * 2.7f) * Rs * 1.15f + std::sin(tNow * 0.9f + i) * 6.0f;
            float y = sc.y + Rs * 1.1f - ph * Rs * 2.5f;
            float a = std::sin(ph * kPi);
            d->AddCircleFilled(ImVec2(x, y), 0.9f + (i % 3) * 0.7f, mulA((i % 2) ? amber : pink, 0.85f * a * al), 8);
        }
    };

    // dark / light tinted glass base + soft drifting glow orbs and dust (card decor)
    auto drawTint = [&](ImDrawList* d, ImVec2 mn, ImVec2 mx, float rad) {
        const ImU32 bgc = lightMode ? IM_COL32(246, 247, 252, 255) : IM_COL32(10, 11, 18, 255);
        d->AddRectFilled(mn, mx, fa(bgc, lightMode ? 0.55f : 0.64f), rad);
        d->AddRectFilledMultiColor(ImVec2(mn.x + rad * 0.5f, mn.y + 1.0f), ImVec2(mx.x - rad * 0.5f, mn.y + (mx.y - mn.y) * 0.30f),
                                   fa(white, lightMode ? 0.30f : 0.045f), fa(white, lightMode ? 0.30f : 0.045f), fa(white, 0.0f), fa(white, 0.0f));
        d->AddRect(mn, mx, fa(lightMode ? IM_COL32(0, 0, 0, 255) : white, lightMode ? 0.10f : 0.11f), rad, 0, 1.0f);
    };
    auto drawAmbient = [&](ImDrawList* d, ImVec2 mn, ImVec2 mx) {
        const float w = mx.x - mn.x, h = mx.y - mn.y;
        d->PushClipRect(mn, mx, true);
        for (int o = 0; o < 3; ++o) {
            ImVec2 c(mn.x + w * (0.22f + 0.28f * o + 0.07f * std::sin(tNow * (0.21f + 0.05f * o) + o * 2.1f)),
                     mn.y + h * (0.30f + 0.22f * (o % 2) + 0.08f * std::cos(tNow * (0.17f + 0.04f * o) + o * 1.3f)));
            const ImU32 col = (o == 1) ? violet : pink;
            const float rad = (w < h ? w : h) * (0.34f + 0.05f * o);
            for (int k = 0; k < 8; ++k)
                d->AddCircleFilled(c, rad * (0.30f + k * 0.10f), fa(col, 0.024f * (1.0f - k / 8.0f)), 40);
        }
        for (int i = 0; i < 24; ++i) {
            float r1 = std::fmod(std::fabs(std::sin(i * 12.9898f) * 43758.5f), 1.0f);
            float r2 = std::fmod(std::fabs(std::sin(i * 78.233f) * 12345.6f), 1.0f);
            float r3 = std::fmod(std::fabs(std::sin(i * 39.346f) * 9876.5f), 1.0f);
            float x = std::fmod(r1 + tNow * (0.004f + 0.006f * r3), 1.0f);
            float y = std::fmod(r2 - tNow * (0.010f + 0.014f * r3) + 4.0f, 1.0f);
            float tw = 0.5f + 0.5f * std::sin(tNow * (0.8f + r3 * 1.6f) + i);
            float edge = std::sin(y * kPi);
            d->AddCircleFilled(ImVec2(mn.x + x * w, mn.y + y * h), 0.9f + r3 * 1.3f, fa((i % 3 == 0) ? violet : pink, (0.10f + 0.30f * tw) * edge), 8);
        }
        d->PopClipRect();
    };

    // primary button drawn by us (soft glow, top highlight, hover / press springs). Returns true on click.
    auto pbtn = [&](const char* id, ImVec2 pos, ImVec2 sz, bool enabled, ImVec2& omn, ImVec2& omx) {
        ImGui::SetCursorScreenPos(pos);
        ImGui::PushID(id);
        bool clicked = ImGui::InvisibleButton("##b", sz);
        bool hov = ImGui::IsItemHovered() && enabled, act = ImGui::IsItemActive() && enabled;
        ImGuiStorage* stg = ImGui::GetStateStorage();
        float& hv = *stg->GetFloatRef(ImGui::GetID("hv"), 0.0f);
        float& pr = *stg->GetFloatRef(ImGui::GetID("pr"), 0.0f);
        float k = sat01(dt * 14.0f);
        hv += ((hov ? 1.0f : 0.0f) - hv) * k; pr += ((act ? 1.0f : 0.0f) - pr) * k;
        ImDrawList* d = ImGui::GetWindowDrawList();
        const float sh = pr * 2.2f;
        ImVec2 mn(pos.x + sh, pos.y + sh * 0.6f), mx(pos.x + sz.x - sh, pos.y + sz.y - sh * 0.6f);
        const float r = 17.0f;
        if (enabled) {
            for (int q = 1; q <= 4; ++q)
                d->AddRectFilled(ImVec2(mn.x - q * 3.0f, mn.y - q * 2.0f + 3.0f), ImVec2(mx.x + q * 3.0f, mx.y + q * 3.0f + 3.0f), fa(pink, (0.045f + 0.07f * hv) * (1.0f - q / 5.0f)), r + q * 3.0f);
            d->AddRectFilled(mn, mx, fa(mix(IM_COL32(186, 22, 46, 255), IM_COL32(246, 70, 56, 255), 0.25f + 0.55f * hv)), r);
            d->AddRectFilled(mn, ImVec2(mx.x, (mn.y + mx.y) * 0.5f), fa(white, 0.15f), r, ImDrawFlags_RoundCornersTop);
            d->AddRect(mn, mx, fa(white, 0.28f), r, 0, 1.0f);
        } else {
            d->AddRectFilled(mn, mx, fa(ink, 0.10f), r);
            d->AddRect(mn, mx, fa(ink, 0.10f), r, 0, 1.0f);
        }
        ImGui::PopID();
        omn = mn; omx = mx;
        return clicked && enabled;
    };
    auto sweep = [&](ImDrawList* d, ImVec2 mn, ImVec2 mx, float period, float frac) {
        float ph = std::fmod(tNow, period) / period;
        if (ph >= frac) return;
        float u = ph / frac, bw2 = 30.0f, x = mn.x - 2.0f * bw2 + (mx.x - mn.x + 4.0f * bw2) * u;
        d->PushClipRect(ImVec2(mn.x + 4.0f, mn.y + 4.0f), ImVec2(mx.x - 4.0f, mx.y - 4.0f), true);
        d->AddRectFilledMultiColor(ImVec2(x, mn.y), ImVec2(x + bw2, mx.y), fa(white, 0.0f), fa(white, 0.18f), fa(white, 0.18f), fa(white, 0.0f));
        d->AddRectFilledMultiColor(ImVec2(x + bw2, mn.y), ImVec2(x + 2.0f * bw2, mx.y), fa(white, 0.18f), fa(white, 0.0f), fa(white, 0.0f), fa(white, 0.18f));
        d->PopClipRect();
    };

    // is a process with this exe name running? (read-only snapshot of the process list)
    auto procRunning = [&](const wchar_t* exe) {
        bool found = false;
        HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
            if (::Process32FirstW(snap, &pe)) do { if (::_wcsicmp(pe.szExeFile, exe) == 0) { found = true; break; } } while (::Process32NextW(snap, &pe));
            ::CloseHandle(snap);
        }
        return found;
    };

    // ---- state machines ----------------------------------------------------------------------
    if (introT < 4.0f) introT += dt;
    // click / Enter skips to the grow-into-window part of the intro
    if (introT > 0.7f && introT < 2.5f && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsKeyPressed(ImGuiKey_Enter, false)))
        introT = 2.5f;
    const float introGate = sm((introT - 3.2f) / 0.7f);   // the card fades in while the splash panel melts into it

    // "Remember me": restore a saved key once, at launch
    if (!loadedSaved) {
        loadedSaved = true;
        if (LuminStore::Load(licKey, (int)sizeof(licKey))) {
            remember = true;
            if (_stricmp(licKey, kDemoKey) == 0 || _stricmp(licKey, "LUMIN-DEMO") == 0) { stage = 1; logged_in = true; welcomePending = true; }
        }
    }
    if (welcomePending && introT > 3.6f) {
        welcomePending = false;
        Glass::Notify(Glass::Icon::CheckCircle, IM_COL32(52, 209, 88, 255), "Eclipse", "Welcome back", "You are signed in.");
    }

    if (stage != lastStage) {
        lastStage = stage; contentT = 0.0f;
        if (stage == 1) loadT = 0.0f;
        int grp = (stage == 0) ? 0 : 1;
        if (grp != lastGrp) { lastGrp = grp; subT = 0.0f; }
    }
    subT += dt * 3.2f; if (subT > 1.0f) subT = 1.0f;
    const float subA = sm(subT);
    contentT += dt * 3.0f; if (contentT > 1.0f) contentT = 1.0f;
    const float contentA = sm(contentT);
    loadT += dt * 1.25f; if (loadT > 2.0f) loadT = 2.0f;

    if (lockT > 0.0f) { lockT -= dt; if (lockT <= 0.0f) { lockT = 0.0f; licErr = false; } }
    if (stage == 1) {                     // watch the game process (read-only) so the screen can show its state live
        pollT -= dt;
        if (pollT <= 0.0f) { gameUp = procRunning(kProcName); pollT = 2.0f; if (gameUp) notFound = false; }
    }
    gameE += ((gameUp ? 1.0f : 0.0f) - gameE) * sat01(dt * 6.0f);
    nfE += (((notFound && !busy) ? 1.0f : 0.0f) - nfE) * sat01(dt * 9.0f);
    pastedFlash -= dt * 2.0f; if (pastedFlash < 0.0f) pastedFlash = 0.0f;
    if (pasteActive) {      // clipboard text "types itself" into the field
        pasteAcc += dt;
        while (pasteAcc >= 0.022f && pasteIdx < pasteLen) {
            licKey[pasteIdx] = pasteBuf[pasteIdx]; ++pasteIdx; licKey[pasteIdx] = 0; pasteAcc -= 0.022f;
        }
        if (pasteIdx >= pasteLen) { pasteActive = false; pastedFlash = 1.0f; licErr = false; }
    }

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
    //  INTRO: the mark builds itself, ECLIPSE rises letter by letter, then the dark panel
    //  grows until it fills the window and becomes the app background.
    // =========================================================================================
    {
        const float sa = 1.0f - sm((introT - 3.3f) / 0.6f);
        if (sa > 0.003f) {
            ImDrawList* fd = ImGui::GetForegroundDrawList();
            const float t = introT;
            const float pcx = (float)cw * 0.5f, pcy = (float)ch * 0.5f;
            const float cf = 1.0f - sm((t - 2.5f) / 0.4f);                 // content fades while the panel grows
            auto A = [&](ImU32 c, float a = 1.0f) {                         // splash alpha only (the splash ignores style.Alpha)
                float al = (float)((c >> 24) & 0xFF) / 255.0f * a * sa;
                al = sat01(al);
                return (ImU32)((c & 0x00FFFFFF) | ((ImU32)(al * 255.0f + 0.5f) << 24));
            };
            const float panelIn = eo3(t / 0.55f);
            const float ex = sm((t - 2.55f) / 0.95f);
            const float tgtW = (stage == 0) ? 404.0f * scale : dashW;
            const float tgtH = (stage == 0) ? licH : dashH;
            const float pscale = 0.955f + 0.045f * panelIn;
            const float W = lerpf(470.0f * pscale, tgtW, ex), H = lerpf(350.0f * pscale, tgtH, ex);
            const float rad = lerpf(32.0f, 26.0f * scale, ex);
            const ImVec2 pmin(pcx - W * 0.5f, pcy - H * 0.5f), pmax(pcx + W * 0.5f, pcy + H * 0.5f);

            for (int k = 3; k >= 1; --k)
                fd->AddRectFilled(ImVec2(pmin.x - k * 5.0f, pmin.y - k * 3.0f), ImVec2(pmax.x + k * 5.0f, pmax.y + k * 7.0f),
                                  A(IM_COL32(0, 0, 0, 255), 0.07f * panelIn * (1.0f - ex)), rad + k * 4.0f);
            fd->AddRectFilled(pmin, pmax, A(IM_COL32(9, 10, 16, 247), panelIn), rad);

            const ImVec2 lc(pcx, pcy - 52.0f);
            const float R = 44.0f;

            fd->PushClipRect(pmin, pmax, true);
            {
                float gi = sat01(t / 1.0f) * (1.0f - 0.6f * ex);
                ImVec2 g1(pcx - 80.0f + 26.0f * std::sin(t * 0.9f), lc.y + 6.0f + 12.0f * std::cos(t * 0.7f));
                ImVec2 g2(pcx + 90.0f + 24.0f * std::cos(t * 0.8f), lc.y + 40.0f + 14.0f * std::sin(t * 0.6f));
                for (int k = 0; k < 9; ++k) {
                    float f = 1.0f - k / 9.0f;
                    fd->AddCircleFilled(g1, 36.0f + k * 12.0f, A(pink, 0.030f * f * gi), 48);
                    fd->AddCircleFilled(g2, 30.0f + k * 11.0f, A(violet, 0.026f * f * gi), 48);
                }
            }
            fd->PopClipRect();
            fd->AddRect(pmin, pmax, A(white, 0.10f * panelIn), rad, 0, 1.0f);

            if (cf > 0.003f) {
                {   // orbiting specks of light
                    float vis = sat01((t - 0.9f) / 0.8f) * cf;
                    for (int i = 0; i < 12; ++i) {
                        float sp = 0.25f + (i % 4) * 0.09f;
                        float ang = i * 0.5236f * 1.7f + t * sp * (i % 2 ? 1.0f : -1.0f);
                        float rr = R * (1.85f + (i % 5) * 0.28f);
                        float tw = 0.5f + 0.5f * std::sin(t * 2.0f + i * 1.3f);
                        fd->AddCircleFilled(ImVec2(lc.x + std::cos(ang) * rr, lc.y + std::sin(ang) * rr * 0.8f), 1.1f + (i % 3) * 0.5f,
                                            A((i % 3 == 0) ? violet : pink, (0.18f + 0.40f * tw) * vis), 10);
                    }
                }
                const float p = sat01((t - 0.15f) / 1.5f);
                const float tLand = 0.15f + 1.5f * 0.8f;
                drawEclipse(fd, lc, R, p, sa * cf);
                {   // shockwave when the flare lands
                    float tt = (t - tLand) / 0.8f;
                    if (tt > 0.0f && tt < 1.0f) {
                        fd->AddCircle(lc, R * 1.1f + eo3(tt) * R * 2.8f, A(pink, (1.0f - tt) * 0.45f * cf), 72, 2.0f);
                        fd->AddCircle(lc, R * 1.1f + eo3(tt - 0.12f) * R * 2.1f, A(white, (1.0f - tt) * 0.16f * cf), 72, 1.2f);
                    }
                }
                {   // ECLIPSE, letter by letter (tracking tightens as it settles)
                    const int n = (int)strlen(kBrand);
                    const float fs = 32.0f;
                    const float spacing = 9.0f + 12.0f * (1.0f - eo3((t - 1.2f) / 1.1f));
                    float wl[16], total = 0.0f;
                    for (int i = 0; i < n; ++i) { char b[2] = { kBrand[i], 0 }; wl[i] = textW(font_semibold, fs, b); total += wl[i]; }
                    total += spacing * (n - 1);
                    float x = pcx - total * 0.5f, y = pcy + 34.0f;
                    for (int i = 0; i < n; ++i) {
                        char b[2] = { kBrand[i], 0 };
                        float e = eo3((t - 1.2f - i * 0.075f) / 0.55f);
                        if (e > 0.003f)
                            fd->AddText(font_semibold, fs, ImVec2(x, y + (1.0f - e) * 20.0f), A(IM_COL32(242, 243, 248, 255), e * cf), b);
                        x += wl[i] + spacing;
                    }
                }
                {   // progress hairline + caption
                    const float bw = 196.0f, bx = pcx - bw * 0.5f, by = pcy + 104.0f;
                    float fin = eo3((t - 1.45f) / 0.4f) * cf;
                    float prog = eo3((t - 1.45f) / 1.05f);
                    fd->AddRectFilled(ImVec2(bx, by), ImVec2(bx + bw, by + 3.0f), A(white, 0.10f * fin), 2.0f);
                    if (prog > 0.003f)
                        fd->AddRectFilledMultiColor(ImVec2(bx, by), ImVec2(bx + bw * prog, by + 3.0f), A(amber, fin), A(pink, fin), A(violet, fin), A(pink, fin));
                    const char* cap = prog < 0.34f ? "Loading modules" : (prog < 0.72f ? "Checking environment" : (prog < 0.985f ? "Almost ready" : "Ready"));
                    float cw3 = textW(font_medium, 14.0f, cap);
                    fd->AddText(font_medium, 14.0f, ImVec2(pcx - cw3 * 0.5f, by + 13.0f), A(IM_COL32(150, 153, 168, 255), fin), cap);
                }
            }
        }
    }

    // =========================================================================================
    //  LICENSE
    // =========================================================================================
    if (stage == 0) {
        const bool isOpen = Glass::BeginCard("license", Glass::Material::Regular, ImVec2(404.0f * scale, 0));
        const float fade = authA * subA * closeA * introGate;
        Glass::SetCardFade(fade);
        if (isOpen) {
            g_glass_renderer.SetSubmitFade(fade);
            ImGui::BeginDisabled(introGate < 0.95f);      // no clicks while the splash is up (alpha is set right after, so no dimming)
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            {
                const ImVec2 wp0 = ImGui::GetWindowPos(), ws0 = ImGui::GetWindowSize();
                licW = ws0.x; licH = ws0.y;
                drawTint(dl, wp0, ImVec2(wp0.x + ws0.x, wp0.y + ws0.y), 26.0f * scale);
                drawAmbient(dl, ImVec2(wp0.x + 12.0f, wp0.y + 12.0f), ImVec2(wp0.x + ws0.x - 12.0f, wp0.y + ws0.y - 12.0f));
            }
            const float cwid = ImGui::GetContentRegionAvail().x;
            ImGuiStorage* stg = ImGui::GetStateStorage();

            // header: the mark, the wordmark, a one-line subtitle
            {
                ImVec2 c0 = ImGui::GetCursorScreenPos();
                const ImVec2 bc(c0.x + cwid * 0.5f, c0.y + 46.0f);
                drawEclipse(dl, bc, 34.0f, 1.0f, style.Alpha);
                trackedC(dl, font_semibold, 25.0f, c0.x + cwid * 0.5f, c0.y + 96.0f, kBrand, 7.0f, fa(ink));
                const char* sub = "Enter your license key to continue";
                dl->AddText(font_medium, 15.0f, ImVec2(c0.x + (cwid - textW(font_medium, 15.0f, sub)) * 0.5f, c0.y + 132.0f), fa(soft), sub);
                ImGui::Dummy(ImVec2(cwid, 168.0f));
            }

            const bool locked = (ver != 0) || pasteActive || lockT > 0.0f;

            // key field: our own box, a transparent ImGui input inside it, a floating label, a Paste chip
            bool enter = false;
            {
                const float gw = ImGui::GetContentRegionAvail().x, fh = 62.0f;
                const ImVec2 gc = ImGui::GetCursorScreenPos(), gmx(gc.x + gw, gc.y + fh);
                float& focT = *stg->GetFloatRef(ImGui::GetID("lic_f"), 0.0f);
                float& labT = *stg->GetFloatRef(ImGui::GetID("lic_l"), 0.0f);
                for (int q = 1; q <= 3; ++q)
                    if (focT > 0.01f) dl->AddRect(ImVec2(gc.x - q * 2.5f, gc.y - q * 2.5f), ImVec2(gmx.x + q * 2.5f, gmx.y + q * 2.5f),
                                                  fa(pink, 0.12f * focT * (1.0f - q / 4.0f)), 18.0f + q * 2.5f, 0, 2.0f);
                dl->AddRectFilled(gc, gmx, fa(ink, 0.055f + 0.03f * focT), 18.0f);
                const ImU32 bcol = licErr ? fa(IM_COL32(255, 85, 100, 255), 0.75f) : fa(mix(ink, pink, focT), lerpf(0.14f, 0.75f, focT));
                dl->AddRect(gc, gmx, bcol, 18.0f, 0, 1.4f);
                if (pastedFlash > 0.01f) dl->AddRect(gc, gmx, fa(pink, 0.9f * pastedFlash), 18.0f, 0, 2.0f);
                Glass::DrawIcon(dl, Glass::Icon::Key, ImVec2(gc.x + 27.0f, gc.y + fh * 0.5f), 20.0f, fa(mix(soft, pink, focT)), 2.0f);
                if (labT < 0.99f)
                    dl->AddText(font_medium, 16.0f, ImVec2(gc.x + 52.0f, gc.y + fh * 0.5f - 7.0f), fa(soft, 0.80f * (1.0f - labT)), "License key");
                if (labT > 0.01f)
                    dl->AddText(font_medium, 11.0f, ImVec2(gc.x + 52.0f, gc.y + 8.0f + (1.0f - labT) * 10.0f), fa(mix(soft, pink, focT), labT), "LICENSE KEY");

                const float rightPad = (licKey[0] == 0 && !locked) ? 90.0f : 16.0f;
                ImGui::SetCursorScreenPos(ImVec2(gc.x + 52.0f, gc.y + 5.0f));
                ImGui::SetNextItemWidth(gw - 52.0f - rightPad);
                if (!autoFocus && introGate > 0.98f && !locked) { ImGui::SetKeyboardFocusHere(); autoFocus = true; }
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, (fh - 5.0f - ImGui::GetFontSize()) * 0.5f));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_Text, ink);
                char prev[64]; memcpy(prev, licKey, sizeof(prev));
                ImGui::BeginDisabled(locked);
                bool ent2 = ImGui::InputText("##lic", licKey, (int)sizeof(licKey),
                                             ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
                const bool active = ImGui::IsItemActive();
                ImGui::EndDisabled();
                ImGui::PopStyleColor(5);
                ImGui::PopStyleVar();
                if (strcmp(prev, licKey) != 0) licErr = false;
                if (ent2) enter = true;
                const float kk = sat01(dt * 12.0f);
                focT += ((active ? 1.0f : 0.0f) - focT) * kk;
                labT += (((active || licKey[0]) ? 1.0f : 0.0f) - labT) * kk;

                if (!locked && licKey[0] == 0) {     // "Paste" chip inside the field
                    const ImVec2 cs(66.0f, 30.0f), cmn(gmx.x - 14.0f - cs.x, gc.y + (fh - cs.y) * 0.5f);
                    ImGui::SetCursorScreenPos(cmn);
                    bool pc = ImGui::InvisibleButton("##paste", cs);
                    bool hv = ImGui::IsItemHovered(), ac = ImGui::IsItemActive();
                    ImVec2 cmx(cmn.x + cs.x, cmn.y + cs.y);
                    dl->AddRectFilled(cmn, cmx, fa(pink, ac ? 0.34f : (hv ? 0.26f : 0.15f)), 15.0f);
                    dl->AddRect(cmn, cmx, fa(pink, hv ? 0.60f : 0.30f), 15.0f, 0, 1.0f);
                    float pw = textW(font_medium, 13.5f, "Paste");
                    dl->AddText(font_medium, 13.5f, ImVec2((cmn.x + cmx.x) * 0.5f - pw * 0.5f, (cmn.y + cmx.y) * 0.5f - 8.0f), fa(pink), "Paste");
                    if (pc) {
                        const char* cb = ImGui::GetClipboardText();
                        int n = 0;
                        if (cb) for (const char* q = cb; *q && n < 40; ++q) if ((unsigned char)*q >= 33 && (unsigned char)*q <= 126) pasteBuf[n++] = *q;
                        pasteBuf[n] = 0;
                        if (n == 0) Glass::Notify(Glass::Icon::WarnTriangle, IM_COL32(255, 159, 10, 255), "Clipboard", "Nothing to paste", "Copy your license key first.");
                        else { pasteActive = true; pasteIdx = 0; pasteLen = n; pasteAcc = 0.0f; licKey[0] = 0; licErr = false; Glass::PlaySfx(Glass::Sfx::Tap); }
                    }
                }
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
                ImGui::SetCursorScreenPos(ImVec2(gc.x, gc.y + fh));
                ImGui::Dummy(ImVec2(gw, 12.0f));
            }

            // row: remember me (left) + error message (right)
            {
                const ImVec2 rp = ImGui::GetCursorScreenPos();
                float& cT = *stg->GetFloatRef(ImGui::GetID("rem_c"), 0.0f);
                bool tg = ImGui::InvisibleButton("##rem", ImVec2(142.0f, 26.0f));
                bool hv = ImGui::IsItemHovered() && !locked;
                if (tg && !locked) { remember = !remember; Glass::PlaySfx(Glass::Sfx::Tap); }
                cT += ((remember ? 1.0f : 0.0f) - cT) * sat01(dt * 14.0f);
                const ImVec2 b0(rp.x + 1.0f, rp.y + 4.0f), b1(rp.x + 19.0f, rp.y + 22.0f);
                dl->AddRectFilled(b0, b1, fa(ink, 0.07f), 6.0f);
                if (cT > 0.01f) dl->AddRectFilled(b0, b1, fa(pink, cT), 6.0f);
                dl->AddRect(b0, b1, fa(mix(ink, pink, cT), 0.22f + 0.6f * cT), 6.0f, 0, 1.2f);
                {
                    float s1 = sat01(cT / 0.5f), s2 = sat01((cT - 0.5f) / 0.5f);
                    ImVec2 a(b0.x + 4.5f, b0.y + 9.5f), b(b0.x + 7.8f, b0.y + 12.8f), c2(b0.x + 13.5f, b0.y + 5.5f);
                    if (s1 > 0.0f) dl->AddLine(a, ImVec2(a.x + (b.x - a.x) * s1, a.y + (b.y - a.y) * s1), fa(white), 2.0f);
                    if (s2 > 0.0f) dl->AddLine(b, ImVec2(b.x + (c2.x - b.x) * s2, b.y + (c2.y - b.y) * s2), fa(white), 2.0f);
                }
                dl->AddText(font_medium, 15.0f, ImVec2(rp.x + 29.0f, rp.y + 3.0f), fa(mix(soft, ink, hv ? 1.0f : cT * 0.6f)), "Remember me");

                static float errA = 0.0f;
                errA += ((licErr ? 1.0f : 0.0f) - errA) * sat01(dt * 12.0f);
                if (errA > 0.01f) {
                    const char* em = errMsg;
                    float ew = textW(font_medium, 15.0f, em);
                    float slide = (1.0f - errA) * 8.0f;
                    ImU32 col = fa(IM_COL32(255, 96, 112, 255), errA);
                    float ex2 = rp.x + cwid - ew - slide;
                    Glass::DrawIcon(dl, Glass::Icon::Key, ImVec2(ex2 - 16.0f, rp.y + 12.0f), 15.0f, col, 2.0f);
                    dl->AddText(font_medium, 15.0f, ImVec2(ex2, rp.y + 3.0f), col, em);
                }
                ImGui::SetCursorScreenPos(ImVec2(rp.x, rp.y + 28.0f));
                ImGui::Dummy(ImVec2(cwid, 14.0f));
            }

            // activate button: label + icon follow the check (Activate -> Checking -> Activated)
            const char* btnLabel = ver == 0 ? "Activate" : (ver == 1 ? "Checking license..." : "Activated");
            static char lockLbl[40];
            if (lockT > 0.0f) { std::snprintf(lockLbl, sizeof(lockLbl), "Try again in %ds", (int)std::ceil(lockT)); btnLabel = lockLbl; }
            ImVec2 bmn, bmx;
            bool go = pbtn("act", ImGui::GetCursorScreenPos(), ImVec2(cwid, 50.0f), lockT <= 0.0f && !pasteActive, bmn, bmx);
            {
                const float tw = textW(font_semibold, 17.0f, btnLabel);
                const ImVec2 ic((bmn.x + bmx.x) * 0.5f - tw * 0.5f - 20.0f, (bmn.y + bmx.y) * 0.5f);
                const ImU32 tcol = (lockT > 0.0f) ? fa(soft) : fa(white);
                if (ver == 0 && lockT > 0.0f) {
                    Glass::DrawIcon(dl, Glass::Icon::Key, ic, 20.0f, tcol, 2.2f);
                    dl->AddRectFilled(ImVec2(bmn.x + 18.0f, bmx.y - 8.0f), ImVec2(bmn.x + 18.0f + (bmx.x - bmn.x - 36.0f) * (lockT / 15.0f), bmx.y - 6.0f), fa(pink, 0.8f), 2.0f);
                } else if (ver == 0) {
                    Glass::DrawIcon(dl, Glass::Icon::CheckCircle, ic, 20.0f, tcol, 2.4f);
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
                dl->AddText(font_semibold, 17.0f, ImVec2((bmn.x + bmx.x) * 0.5f - tw * 0.5f + 8.0f, (bmn.y + bmx.y) * 0.5f - 9.0f), tcol, btnLabel);
                if (ver == 0 && lockT <= 0.0f) sweep(dl, bmn, bmx, 3.6f, 0.4f);
            }
            // footer
            {
                ImVec2 fp = ImGui::GetCursorScreenPos();
                char foot[64]; std::snprintf(foot, sizeof(foot), "Eclipse  -  %s", kVersion);
                float fw = textW(font_medium, 13.0f, foot);
                dl->AddText(font_medium, 13.0f, ImVec2(fp.x + (cwid - fw) * 0.5f, fp.y + 14.0f), fa(soft, 0.7f), foot);
                ImGui::Dummy(ImVec2(cwid, 38.0f));
            }

            // ---- the check itself: a short, smooth "verifying" phase, then the result ----
            const bool isDemo = (_stricmp(licKey, kDemoKey) == 0) || (_stricmp(licKey, "LUMIN-DEMO") == 0);
            if (ver == 0 && lockT <= 0.0f && !pasteActive && (go || enter)) {
                if (licKey[0] == 0) {
                    licErr = true; shake = 0.7f; std::snprintf(errMsg, sizeof(errMsg), "Enter a license key");
                    Glass::PlaySfx(Glass::Sfx::Error);
                } else {
                    ver = 1; verT = 0.0f; licErr = false;
                    Glass::PlaySfx(Glass::Sfx::Tap);
                }
            } else if (ver == 1) {
                verT += dt;
                if (verT >= 1.45f) {
                    if (isDemo) {
                        ver = 2; verT = 0.0f; fails = 0;
                        Glass::PlaySfx(Glass::Sfx::Success);
                    } else {
                        ver = 0; licErr = true; shake = 1.0f; ++fails;
                        if (fails >= 3) { lockT = 15.0f; fails = 0; std::snprintf(errMsg, sizeof(errMsg), "Too many attempts"); }
                        else std::snprintf(errMsg, sizeof(errMsg), "Invalid key - %d left", 3 - fails);
                        Glass::PlaySfx(Glass::Sfx::Error);
                    }
                }
            } else if (ver == 2) {
                verT += dt;
                if (verT >= 0.7f) {
                    ver = 0; stage = 1; logged_in = true;
                    if (remember) LuminStore::Save(licKey); else LuminStore::Clear();
                    Glass::Notify(Glass::Icon::CheckCircle, IM_COL32(52, 209, 88, 255), "Eclipse", "License activated", "Welcome back.");
                }
            }
            g_glass_renderer.SetSubmitFade(1.0f);
            ImGui::EndDisabled();
            style.Alpha = ent;
        }
        Glass::EndCard();
        Glass::SetCardFade(1.0f);
    } else {
        // =====================================================================================
        //  MENU / SUCCESS
        // =====================================================================================
        const bool isOpen = Glass::BeginCard("dash", Glass::Material::Regular, ImVec2(dashW, dashH));
        const float fade = authA * subA * closeA * introGate;
        Glass::SetCardFade(fade);
        if (isOpen) {
            g_glass_renderer.SetSubmitFade(fade);
            ImGui::BeginDisabled(introGate < 0.95f);      // no clicks while the splash is up (alpha is set right after, so no dimming)
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            {
                const ImVec2 wp0 = ImGui::GetWindowPos(), ws0 = ImGui::GetWindowSize();
                drawTint(dl, wp0, ImVec2(wp0.x + ws0.x, wp0.y + ws0.y), 26.0f * scale);
                drawAmbient(dl, ImVec2(wp0.x + 12.0f, wp0.y + 12.0f), ImVec2(wp0.x + ws0.x - 12.0f, wp0.y + ws0.y - 12.0f));
            }
            const ImVec2 o = ImGui::GetCursorScreenPos();
            const float innerW = ImGui::GetContentRegionAvail().x;
            const float innerH = ImGui::GetContentRegionAvail().y;
            const float slide = (1.0f - subA) * 22.0f + (1.0f - contentA) * 14.0f;

            // title bar: server pill on the left, wordmark in the middle (hides while the island is open), window buttons on the right
            {
                const float la = 1.0f - 0.92f * islandE;
                const float mx0 = o.x + innerW * 0.5f;
                drawEclipse(dl, ImVec2(mx0 - 44.0f, o.y + 17.0f), 9.0f, 1.0f, style.Alpha * la);
                trackedC(dl, font_semibold, 14.0f, mx0 + 12.0f, o.y + 9.0f, kBrand, 3.2f, fa(ink, la));
                dl->AddLine(ImVec2(o.x - 8.0f, o.y + 42.0f), ImVec2(o.x + innerW + 8.0f, o.y + 42.0f), fa(ink, 0.10f), 1.0f);
                // server indicator (demo: the ping is simulated)
                int ping = (int)(24.0f + 6.0f * std::sin(tNow * 0.7f) + 3.0f * std::sin(tNow * 2.3f) + 0.5f);
                char pb[32]; std::snprintf(pb, sizeof(pb), "Online - %d ms", ping);
                float pw = textW(font_medium, 13.0f, pb);
                ImVec2 pmn(o.x + 2.0f, o.y + 4.0f), pmx(o.x + 2.0f + 30.0f + pw, o.y + 30.0f);
                dl->AddRectFilled(pmn, pmx, fa(ink, 0.06f * la), 13.0f);
                dl->AddRect(pmn, pmx, fa(ink, 0.10f * la), 13.0f, 0, 1.0f);
                ImVec2 dc(pmn.x + 14.0f, (pmn.y + pmx.y) * 0.5f);
                float ph = std::fmod(tNow, 1.8f) / 1.8f;
                dl->AddCircleFilled(dc, 3.6f, fa(okc, la), 14);
                dl->AddCircle(dc, 3.6f + ph * 7.0f, fa(okc, (1.0f - ph) * 0.5f * la), 20, 1.3f);
                dl->AddText(font_medium, 13.0f, ImVec2(pmn.x + 25.0f, pmn.y + 5.0f), fa(soft, la), pb);
            }
            const float baseAlpha = style.Alpha;
            style.Alpha = baseAlpha * contentA;       // inner content crossfades between menu and success

            if (stage == 1) {
                const float leftW = 226.0f;

                // ---------------- 3D emblem (reacts to the cursor) ----------------
                {
                    static float spx = 0.0f, spy = 0.0f;
                    const ImVec2 ec(o.x + leftW * 0.5f + slide * 0.5f, o.y + 190.0f);
                    float tx = (io.MousePos.x - ec.x) / 320.0f, ty = (io.MousePos.y - ec.y) / 320.0f;
                    tx = tx < -1.0f ? -1.0f : (tx > 1.0f ? 1.0f : tx); ty = ty < -1.0f ? -1.0f : (ty > 1.0f ? 1.0f : ty);
                    spx += (tx - spx) * sat01(dt * 7.0f); spy += (ty - spy) * sat01(dt * 7.0f);
                    const float emE = eo3(loadT / 0.7f);
                    drawEmblem(dl, ec, 56.0f * (0.62f + 0.38f * emE), spx, spy, style.Alpha * emE);
                }
                dl->AddLine(ImVec2(o.x + leftW + 6.0f, o.y + 58.0f), ImVec2(o.x + leftW + 6.0f, o.y + 352.0f), fa(ink, 0.08f), 1.0f);

                // ---------------- product information ----------------
                const float rx0 = o.x + leftW + 24.0f + slide;
                const float rw  = innerW - leftW - 24.0f - 4.0f;
                const float y0  = o.y + 60.0f;
                dl->AddText(font_medium, 11.5f, ImVec2(rx0 + 2.0f, y0), fa(soft, 0.9f), "SELECTED PRODUCT");
                dl->AddText(font_semibold, 30.0f, ImVec2(rx0, y0 + 16.0f), fa(ink), kProdName);

                {   // live game indicator (reads the process list every 2 s)
                    const char* gl = gameUp ? "Game running" : "Game not running";
                    const float gw2 = textW(font_medium, 13.0f, gl) + 34.0f;
                    const ImVec2 g0(rx0 + rw - gw2, y0 + 20.0f), g1(rx0 + rw, y0 + 46.0f);
                    const ImU32 gc2 = mix(IM_COL32(150, 153, 168, 255), okc, gameE);
                    dl->AddRectFilled(g0, g1, fa(gc2, 0.12f), 13.0f);
                    dl->AddRect(g0, g1, fa(gc2, 0.30f), 13.0f, 0, 1.0f);
                    const ImVec2 gd(g0.x + 14.0f, (g0.y + g1.y) * 0.5f);
                    dl->AddCircleFilled(gd, 3.5f, fa(gc2), 14);
                    if (gameUp) { float ph = std::fmod(tNow, 1.8f) / 1.8f; dl->AddCircle(gd, 3.5f + ph * 7.0f, fa(okc, (1.0f - ph) * 0.5f), 20, 1.3f); }
                    dl->AddText(font_medium, 13.0f, ImVec2(g0.x + 25.0f, g0.y + 5.0f), fa(gameUp ? ink : soft), gl);
                }
                const float rowH = 38.0f, panelY = y0 + 64.0f, panelH = rowH * 4 + 16.0f;
                dl->AddRectFilled(ImVec2(rx0, panelY), ImVec2(rx0 + rw, panelY + panelH), fa(ink, 0.045f), 16.0f);
                dl->AddRect(ImVec2(rx0, panelY), ImVec2(rx0 + rw, panelY + panelH), fa(ink, 0.09f), 16.0f, 0, 1.0f);
                const char* labels[4] = { "Game", "Version", "Status", "Till Expiry" };
                for (int i = 0; i < 4; ++i) {
                    // skeleton bars shimmer first, then each value reveals with a small stagger
                    const float rs = 0.50f + i * 0.15f;
                    float e  = eo3((loadT - rs) / 0.40f);
                    float sk = 1.0f - sm((loadT - rs) / 0.18f);
                    float ry = panelY + 8.0f + rowH * i + (rowH - 17.0f) * 0.5f;
                    if (i > 0) dl->AddLine(ImVec2(rx0 + 18.0f, panelY + 8.0f + rowH * i), ImVec2(rx0 + rw - 18.0f, panelY + 8.0f + rowH * i), fa(ink, 0.06f), 1.0f);
                    dl->AddText(font_medium, 16.0f, ImVec2(rx0 + 22.0f, ry), fa(soft), labels[i]);
                    const float vr = rx0 + rw - 22.0f;

                    if (sk > 0.01f) {
                        static const float skW[4] = { 132.0f, 58.0f, 76.0f, 64.0f };
                        const float sw = skW[i];
                        ImVec2 a0(vr - sw, ry + 1.0f), a1(vr, ry + 16.0f);
                        dl->AddRectFilled(a0, a1, fa(ink, 0.09f * sk), 7.0f);
                        float u = std::fmod(tNow * 0.9f + i * 0.13f, 1.5f) / 1.5f;
                        float x = a0.x - 30.0f + (sw + 60.0f) * u;
                        dl->PushClipRect(a0, a1, true);
                        dl->AddRectFilledMultiColor(ImVec2(x, a0.y), ImVec2(x + 15.0f, a1.y), fa(white, 0.0f), fa(white, 0.20f * sk), fa(white, 0.20f * sk), fa(white, 0.0f));
                        dl->AddRectFilledMultiColor(ImVec2(x + 15.0f, a0.y), ImVec2(x + 30.0f, a1.y), fa(white, 0.20f * sk), fa(white, 0.0f), fa(white, 0.0f), fa(white, 0.20f * sk));
                        dl->PopClipRect();
                    }
                    if (e <= 0.003f) continue;

                    char buf[48];
                    const char* val = buf;
                    if (i == 0) val = kProdGame;
                    else if (i == 1) val = kProdVer;
                    else if (i == 2) val = kProdStatus;
                    else { std::snprintf(buf, sizeof(buf), "%d days", (int)(kDays * e + 0.5f)); }

                    ImVec2 vs = font_medium->CalcTextSizeA(16.0f, 99999.0f, 0.0f, val);
                    float vx = vr - vs.x + (1.0f - e) * 18.0f;
                    ImU32 vcol = (i == 2) ? kStatusCol : ink;
                    dl->AddText(font_medium, 16.0f, ImVec2(vx, ry), fa(vcol, e), val);

                    if (i == 2) {   // pulsing status dot
                        ImVec2 dc(vx - 14.0f, ry + 9.0f);
                        float ph = std::fmod(tNow, 1.7f) / 1.7f;
                        dl->AddCircleFilled(dc, 3.6f, fa(kStatusCol, e), 16);
                        dl->AddCircle(dc, 3.6f + ph * 8.0f, fa(kStatusCol, (1.0f - ph) * 0.55f * e), 24, 1.4f);
                    }
                    if (i == 3) {   // "days left" ring: green >= 10, amber <= 9, red <= 3
                        ImU32 rc = kDays <= 3 ? IM_COL32(255, 85, 85, 255) : (kDays <= 9 ? IM_COL32(255, 200, 80, 255) : okc);
                        float frac = (float)kDays / 30.0f; if (frac > 1.0f) frac = 1.0f;
                        ImVec2 rcn(vx - 17.0f, ry + 9.0f);
                        dl->AddCircle(rcn, 7.0f, fa(ink, 0.14f * e), 28, 2.4f);
                        if (frac * e > 0.01f) {
                            dl->PathArcTo(rcn, 7.0f, -kPi * 0.5f, -kPi * 0.5f + 2.0f * kPi * frac * e, 28);
                            dl->PathStroke(fa(rc, e), 0, 2.4f);
                        }
                    }
                }

                // action button (stub)
                const ImVec2 abp(rx0, panelY + panelH + 18.0f);
                const bool actOk = !busy && loadT >= 1.4f && strcmp(kProdStatus, "Available") == 0;
                if (!busy && notFound) {
                    // game not running: a calm inline card instead of the button (with Retry)
                    const float e = nfE;
                    const ImVec2 n0(abp.x, abp.y + (1.0f - e) * 8.0f), n1(abp.x + rw, abp.y + 56.0f + (1.0f - e) * 8.0f);
                    const float mid = (n0.y + n1.y) * 0.5f;
                    dl->AddRectFilled(n0, n1, fa(amber, 0.085f * e), 16.0f);
                    dl->AddRect(n0, n1, fa(amber, 0.34f * e), 16.0f, 0, 1.2f);
                    const ImVec2 ic(n0.x + 30.0f, mid);
                    const float pph = std::fmod(tNow, 2.0f) / 2.0f;
                    dl->AddCircleFilled(ic, 17.0f, fa(amber, 0.16f * e), 28);
                    dl->AddCircle(ic, 17.0f + pph * 9.0f, fa(amber, (1.0f - pph) * 0.35f * e), 32, 1.4f);
                    Glass::DrawIcon(dl, Glass::Icon::Display, ic, 18.0f, fa(amber, e), 2.0f);
                    dl->AddText(font_semibold, 14.5f, ImVec2(n0.x + 58.0f, mid - 19.0f), fa(ink, e), "Counter-Strike 2 is not running");
                    dl->AddText(font_medium, 12.5f, ImVec2(n0.x + 58.0f, mid + 2.0f), fa(soft, e), "Launch the game, then press Retry");
                    const ImVec2 r0(n1.x - 14.0f - 64.0f, mid - 15.0f), r1(n1.x - 14.0f, mid + 15.0f);
                    ImGui::SetCursorScreenPos(r0);
                    bool rc = ImGui::InvisibleButton("##retry", ImVec2(64.0f, 30.0f));
                    bool rh = ImGui::IsItemHovered();
                    dl->AddRectFilled(r0, r1, fa(amber, (rh ? 0.30f : 0.18f) * e), 15.0f);
                    dl->AddRect(r0, r1, fa(amber, 0.55f * e), 15.0f, 0, 1.0f);
                    dl->AddText(font_medium, 14.0f, ImVec2((r0.x + r1.x) * 0.5f - textW(font_medium, 14.0f, "Retry") * 0.5f, mid - 8.0f), fa(amber, e), "Retry");
                    ImGui::SetCursorScreenPos(abp); ImGui::Dummy(ImVec2(rw, 56.0f));
                    if (rc && e > 0.6f) { notFound = false; busy = true; busyT = 0.0f; busyPhase = 0; }
                } else if (!busy) {
                    ImVec2 bmn, bmx;
                    bool clicked = pbtn("inj", abp, ImVec2(rw, 52.0f), actOk, bmn, bmx);
                    const float tw = textW(font_semibold, 17.0f, kActionText);
                    const ImU32 tcol = actOk ? fa(white) : fa(soft);
                    Glass::DrawIcon(dl, Glass::Icon::Bolt, ImVec2((bmn.x + bmx.x) * 0.5f - tw * 0.5f - 18.0f, (bmn.y + bmx.y) * 0.5f), 18.0f, tcol, 2.2f);
                    dl->AddText(font_semibold, 17.0f, ImVec2((bmn.x + bmx.x) * 0.5f - tw * 0.5f + 6.0f, (bmn.y + bmx.y) * 0.5f - 9.5f), tcol, kActionText);
                    if (actOk) sweep(dl, bmn, bmx, 3.4f, 0.38f);
                    if (clicked) { busy = true; busyT = 0.0f; busyPhase = 0; notFound = false; }
                } else if (busyPhase == 0) {
                    // step 1: look for the game process (spinner + sweep while "scanning")
                    busyT += dt;
                    const ImVec2 t0(abp.x, abp.y + 12.0f), t1(abp.x + rw, abp.y + 22.0f);
                    dl->AddRectFilled(t0, t1, fa(ink, 0.10f), 5.0f);
                    {
                        float u = std::fmod(tNow * 0.9f, 1.0f), segW = rw * 0.30f, x = t0.x - segW + (rw + segW) * u;
                        dl->PushClipRect(t0, t1, true);
                        dl->AddRectFilledMultiColor(ImVec2(x, t0.y), ImVec2(x + segW * 0.5f, t1.y), fa(pink, 0.0f), fa(pink, 0.95f), fa(pink, 0.95f), fa(pink, 0.0f));
                        dl->AddRectFilledMultiColor(ImVec2(x + segW * 0.5f, t0.y), ImVec2(x + segW, t1.y), fa(pink, 0.95f), fa(pink, 0.0f), fa(pink, 0.0f), fa(pink, 0.95f));
                        dl->PopClipRect();
                    }
                    char msg[64]; std::snprintf(msg, sizeof(msg), "Looking for %ls...", kProcName);
                    float mw = textW(font_medium, 15.0f, msg);
                    float a0 = tNow * 7.0f; ImVec2 sc(abp.x + (rw - mw) * 0.5f - 14.0f, abp.y + 38.0f);
                    dl->AddCircle(sc, 6.0f, fa(soft, 0.3f), 20, 2.0f);
                    dl->PathArcTo(sc, 6.0f, a0, a0 + kPi * 1.2f, 16); dl->PathStroke(fa(pink), 0, 2.2f);
                    dl->AddText(font_medium, 15.0f, ImVec2(abp.x + (rw - mw) * 0.5f + 2.0f, abp.y + 30.0f), fa(soft), msg);
                    ImGui::SetCursorScreenPos(abp); ImGui::Dummy(ImVec2(rw, 52.0f));
                    if (busyT >= 1.1f) {
                        if (procRunning(kProcName)) { busyPhase = 1; busyT = 0.0f; }
                        else { busy = false; notFound = true; nfE = 0.0f; }
                    }
                } else {
                    // step 2: process found -> loading animation (visual only)
                    busyT += dt * 0.5f;
                    float pr = busyT > 1.0f ? 1.0f : busyT;
                    const ImVec2 t0(abp.x, abp.y + 12.0f), t1(abp.x + rw, abp.y + 22.0f);
                    dl->AddRectFilled(t0, t1, fa(ink, 0.10f), 5.0f);
                    for (int q = 1; q <= 3; ++q)
                        dl->AddRectFilled(ImVec2(t0.x - q * 2.0f, t0.y - q * 2.0f), ImVec2(t0.x + rw * pr + q * 2.0f, t1.y + q * 2.0f), fa(pink, 0.06f * (1.0f - q / 4.0f)), 5.0f + q * 2.0f);
                    if (pr > 0.01f) {
                        dl->PushClipRect(t0, ImVec2(t0.x + rw * pr, t1.y), true);
                        dl->AddRectFilledMultiColor(t0, t1, fa(amber), fa(pink), fa(violet), fa(pink));
                        dl->PopClipRect();
                    }
                    const char* msg = pr < 0.25f ? "Process found" : (pr < 0.55f ? "Checking license..." : (pr < 0.85f ? "Preparing..." : "Almost done..."));
                    dl->AddText(font_medium, 15.0f, ImVec2(abp.x + (rw - textW(font_medium, 15.0f, msg)) * 0.5f, abp.y + 30.0f), fa(soft), msg);
                    ImGui::SetCursorScreenPos(abp); ImGui::Dummy(ImVec2(rw, 52.0f));
                    if (busyT >= 1.1f) { busy = false; busyPhase = 0; stage = 2; okT = 0.0f; Glass::PlaySfx(Glass::Sfx::Success); }
                }
                {   // version tag, bottom right
                    char vb[48]; std::snprintf(vb, sizeof(vb), "Eclipse  %s", kVersion);
                    dl->AddText(font_medium, 12.5f, ImVec2(o.x + innerW - textW(font_medium, 12.5f, vb) - 4.0f, o.y + innerH - 22.0f), fa(soft, 0.7f), vb);
                }

                // ---------------- profile row (bottom-left): theme + sign out only ----------------
                {
                    float pbH = 56.0f, pbX = o.x + 4.0f - slide, pbW = 188.0f;
                    float pbY = o.y + innerH - pbH - 6.0f;
                    Glass::ProfileAction pa = Glass::ProfileMenu("acct", kUserName, kUserTag, "U", IM_COL32(232, 38, 66, 255),
                                                                 pbX, pbY, pbW, &profStatus, &profTheme, fade, false);
                    if (pa == Glass::ProfileAction::SignOut) {
                        stage = 0; logged_in = false; licKey[0] = 0; licErr = false; ver = 0; busy = false; busyPhase = 0; autoFocus = false;
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
                            char sub[64]; std::snprintf(sub, sizeof(sub), "Eclipse %s is ready", kVersion);
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

            style.Alpha = baseAlpha;
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
        Glass::SetCardFade(1.0f);
    }
}
