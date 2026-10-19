// Lumin UI: license screen -> product menu -> success screen.
// Included from main.cpp inside the frame loop (needs: scale, ent, authA, style, io, win_w, win_h,
// font_medium, font_semibold, g_glass_renderer, hwnd, logged_in, gShakeX, appearance, profStatus, profTheme).
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
    static const char* kUserTag    = "Pro  -  29 days";
    static const char* kDemoKey    = "LUMIN-DEMO";      // the only key the demo accepts
    static const char* kActionText = "Inject";          // button label (stub)
    static const char* kUpdateText = "Update available";
    static const char* kUpdateVer  = "v1.2.0";
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
    static int   lastStage = 0; static float subT = 0.0f;    // starts at 0: the first card fades in on launch
    static float swapT = 1.0f;
    static bool  closing = false; static float closeT = 0.0f;
    static float islandClock = 0.0f; static bool islandGone = false;
    static bool  loadedSaved = false;

    const float dt   = io.DeltaTime;
    const float tNow = (float)ImGui::GetTime();
    const float kPi  = 3.14159265f;

    const ImU32 ink  = Glass::InkColor();        // follow the light / dark theme
    const ImU32 soft = Glass::InkSoftColor();
    const ImU32 pink = IM_COL32(232, 104, 128, 255);
    const ImU32 okc  = IM_COL32(52, 209, 88, 255);
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
    auto mix = [&](ImU32 a, ImU32 b, float t) {
        t = sat01(t);
        auto chn = [&](int sh) { float x = (float)((a >> sh) & 0xFF), y = (float)((b >> sh) & 0xFF); return (ImU32)(x + (y - x) * t + 0.5f); };
        return (ImU32)((chn(0)) | (chn(8) << 8) | (chn(16) << 16) | (chn(24) << 24));
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
    // "Remember me": restore a saved key once, at launch
    if (!loadedSaved) {
        loadedSaved = true;
        if (LuminStore::Load(licKey, (int)sizeof(licKey))) {
            remember = true;
            if (_stricmp(licKey, kDemoKey) == 0) {
                stage = 1; logged_in = true;
                Glass::Notify(Glass::Icon::CheckCircle, IM_COL32(52, 209, 88, 255), "Lumin", "Welcome back", "You are signed in.");
            }
        }
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
    //  LICENSE
    // =========================================================================================
    if (stage == 0) {
        if (Glass::BeginCard("license", Glass::Material::Regular, ImVec2(404.0f * scale, 0))) {
            const float fade = authA * subA * closeA;
            g_glass_renderer.SetSubmitFade(fade);
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float cwid = ImGui::GetContentRegionAvail().x;

            // header: badge + title
            {
                ImVec2 c = ImGui::GetCursorScreenPos();
                const float bs = 56.0f;
                Glass::Primitive badge; badge.cx = c.x + bs * 0.5f; badge.cy = c.y + bs * 0.5f;
                badge.hw = bs * 0.5f; badge.hh = bs * 0.5f; badge.corner_radius = 16.0f; badge.fade = 1.0f;
                badge.material = Glass::Material::Accent;
                g_glass_renderer.Submit(badge);
                for (int k = 0; k < 3; ++k)   // slow breathing glow behind the badge
                    dl->AddCircleFilled(ImVec2(badge.cx, badge.cy), bs * 0.62f + k * 7.0f,
                                        fa(pink, (0.10f - k * 0.03f) * (0.6f + 0.4f * std::sin(tNow * 1.8f))), 40);
                Glass::DrawIcon(dl, Glass::Icon::Compass, ImVec2(badge.cx, badge.cy), bs * 0.56f, fa(white), 2.8f);
                dl->AddText(font_semibold, 24.0f, ImVec2(c.x + bs + 16.0f, c.y + 5.0f), fa(ink), "Lumin");
                dl->AddText(font_medium, 15.0f, ImVec2(c.x + bs + 16.0f, c.y + 34.0f), fa(soft), "License access");
                ImGui::Dummy(ImVec2(cwid, bs + 26.0f));
            }

            ImGui::PushStyleColor(ImGuiCol_Text, ink);
            ImGui::TextUnformatted("License key");
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 8));

            const bool locked = (ver != 0);

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
                ImGui::BeginDisabled(locked);
                if (Glass::FieldRaw("lic", licKey, (int)sizeof(licKey), "XXXX-XXXX-XXXX", Glass::Icon::Key, false, fh))
                    licErr = false;
                if (Glass::FieldWasFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false)) enter = true;
                ImGui::EndDisabled();
                // indeterminate sweep along the bottom edge while the key is being checked
                if (ver == 1) {
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
                ImGui::Dummy(ImVec2(gw, 6.0f));
            }

            // remember me
            ImGui::BeginDisabled(locked);
            Glass::Checkbox("Remember me", &remember);
            ImGui::EndDisabled();

            // error line (space is always reserved so the layout never jumps)
            {
                ImVec2 c = ImGui::GetCursorScreenPos();
                static float errA = 0.0f;
                errA += ((licErr ? 1.0f : 0.0f) - errA) * (dt * 12.0f > 1.0f ? 1.0f : dt * 12.0f);
                if (errA > 0.01f) {
                    float slide = (1.0f - errA) * 8.0f;
                    ImU32 col = fa(pink, errA);
                    Glass::DrawIcon(dl, Glass::Icon::Key, ImVec2(c.x + 10.0f, c.y + 16.0f - slide), 15.0f, col, 2.0f);
                    dl->AddText(font_medium, 15.0f, ImVec2(c.x + 28.0f, c.y + 7.0f - slide), col, "Invalid license key");
                }
                ImGui::Dummy(ImVec2(cwid, 30.0f));
            }

            // activate button: label + icon follow the check (Activate -> Checking -> Activated)
            const char* btnLabel = ver == 0 ? "Activate" : (ver == 1 ? "Checking license..." : "Activated");
            bool go = Glass::Button(btnLabel, true, ImVec2(ImGui::GetContentRegionAvail().x, 50.0f));
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
            ImGui::Dummy(ImVec2(0, 6));

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
                Glass::DrawIcon(dl, Glass::Icon::Compass, lc, 28.0f, fa(ink), 2.6f);
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

                // ---------------- profile row (bottom-left) ----------------
                {
                    float pbH = 56.0f, pbX = o.x + 6.0f - slide, pbW = sideW - 12.0f;
                    float pbY = o.y + innerH - pbH - 6.0f;
                    Glass::ProfileAction pa = Glass::ProfileMenu("acct", kUserName, kUserTag, "U", IM_COL32(232, 104, 128, 255),
                                                                 pbX, pbY, pbW, &profStatus, &profTheme, fade, false);
                    switch (pa) {
                        case Glass::ProfileAction::ViewProfile:
                            Glass::Notify(Glass::Icon::Person, IM_COL32(232, 104, 128, 255), "Profile", kUserName, "Pro plan, 29 days left."); break;
                        case Glass::ProfileAction::AccountSettings:
                        case Glass::ProfileAction::GlassSettings:
                            Glass::Notify(Glass::Icon::Gear, IM_COL32(142, 142, 147, 255), "Settings", "Coming soon", "Not part of this demo yet."); break;
                        case Glass::ProfileAction::SignOut:
                            stage = 0; logged_in = false; licKey[0] = 0; licErr = false; ver = 0; busy = false;
                            islandClock = 0.0f; islandGone = false;
                            LuminStore::Clear();
                            break;
                        case Glass::ProfileAction::StatusChanged: {
                            const char* sn[4] = { "Online", "Away", "Do Not Disturb", "Invisible" };
                            ImU32 sc2[4] = { IM_COL32(52, 209, 88, 255), IM_COL32(255, 159, 10, 255), IM_COL32(255, 69, 58, 255), IM_COL32(142, 142, 147, 255) };
                            Glass::Notify(Glass::Icon::Person, sc2[profStatus], "Status", sn[profStatus], "Status updated."); break; }
                        case Glass::ProfileAction::ThemeChanged: {
                            appearance = (profTheme == 1) ? 1 : 0;      // 0 dark, 1 light, 2 auto -> dark
                            const char* tn[3] = { "Dark", "Light", "Auto" };
                            Glass::Notify(Glass::Icon::Display, IM_COL32(94, 92, 230, 255), "Appearance", tn[profTheme], "Theme updated."); break; }
                        default: break;
                    }
                }

                // ---------------- Dynamic Island: "Update available" ----------------
                if (!islandGone && subA > 0.6f) {
                    islandClock += dt;
                    float icx = o.x + innerW * 0.5f, itop = o.y - 30.0f;
                    ImVec2 hmin(icx - 95.0f, itop - 4.0f), hmax(icx + 95.0f, itop + 42.0f);
                    bool hovI = ImGui::IsMouseHoveringRect(hmin, hmax);
                    bool pop = hovI || (islandClock > 0.9f && islandClock < 5.8f);
                    if (islandClock > 0.5f)
                        Glass::DynamicIsland(Glass::Icon::Download, kUpdateText, kUpdateVer, pop, icx, itop, innerW);
                    if (hovI && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && islandClock > 0.5f) {
                        islandGone = true;
                        Glass::PlaySfx(Glass::Sfx::Tap);
                        Glass::Notify(Glass::Icon::Download, IM_COL32(52, 209, 88, 255), "Lumin", "Update scheduled", "It will install on the next launch.");
                    }
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
            style.Alpha = ent;
        }
        Glass::EndCard();
    }
}
