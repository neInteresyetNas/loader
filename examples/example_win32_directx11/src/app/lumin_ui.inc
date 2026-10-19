// Lumin UI: license screen -> product menu -> success screen.
// Included from main.cpp inside the frame loop (needs: scale, ent, authA, style, io, win_w, win_h,
// font_medium, font_semibold, g_glass_renderer, hwnd, logged_in, gShakeX).
//
// UI ONLY. The "Inject" button is a visual stub: it plays a fake progress bar and shows the success
// screen. It does not touch any process or file.
{
    // ---- editable content --------------------------------------------------------------------
    struct Prod { const char* name; const char* game; const char* version; const char* status; ImU32 statusCol; const char* expiry; };
    static const Prod prods[3] = {
        { "Alt:V",    "alt:V Multiplayer", "release", "Available",   IM_COL32(110, 231, 130, 255), "29 days" },
        { "Rage:MP",  "RAGE Multiplayer",  "release", "Updating",    IM_COL32(255, 200,  80, 255), "29 days" },
        { "Deadlock", "Deadlock",          "beta",    "Available",   IM_COL32(110, 231, 130, 255), "29 days" },
    };
    static const char* kUserName   = "username";
    static const char* kDemoKey    = "LUMIN-DEMO";      // the only key the demo accepts
    static const char* kActionText = "Inject";          // button label (stub)
    // -------------------------------------------------------------------------------------------

    static int   stage = 0;                 // 0 license, 1 menu, 2 success
    static int   sel = 0;
    static char  licKey[64] = "";
    static bool  licErr = false;
    static float shake = 0.0f;
    static bool  busy = false; static float busyT = 0.0f;
    static float okT = 0.0f;
    static int   lastStage = 0; static float subT = 1.0f;

    const float dt = io.DeltaTime;
    const ImU32 ink  = IM_COL32(236, 237, 243, 255);
    const ImU32 soft = IM_COL32(150, 153, 168, 255);
    const ImU32 pink = IM_COL32(232, 104, 128, 255);

    if (stage != lastStage) { lastStage = stage; subT = 0.0f; }
    subT += dt * 6.0f; if (subT > 1.0f) subT = 1.0f;
    const float subA = subT * subT * (3.0f - 2.0f * subT);

    // window shake on a wrong key (applied to the next frame's window position)
    shake -= dt * 2.2f; if (shake < 0.0f) shake = 0.0f;
    gShakeX = std::sin(shake * 38.0f) * 11.0f * shake;

    auto lumin_center = [&](const char* t, ImU32 col) {
        float tw = ImGui::CalcTextSize(t).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - tw) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextUnformatted(t);
        ImGui::PopStyleColor();
    };

    if (stage == 0) {
        // ============================== LICENSE ==============================
        if (Glass::BeginCard("license", Glass::Material::Regular, ImVec2(404.0f * scale, 0))) {
            const float fade = authA * subA;
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
                Glass::DrawIcon(dl, Glass::Icon::Compass, ImVec2(badge.cx, badge.cy), bs * 0.56f, IM_COL32(255, 255, 255, 255), 2.8f);
                dl->AddText(font_semibold, 24.0f, ImVec2(c.x + bs + 16.0f, c.y + 5.0f), ink, "Lumin");
                dl->AddText(font_medium, 15.0f, ImVec2(c.x + bs + 16.0f, c.y + 34.0f), soft, "License access");
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
                dl->AddLine(ImVec2(gc.x + 16.0f, gc.y + 1.0f), ImVec2(gc.x + gw - 16.0f, gc.y + 1.0f), IM_COL32(255, 255, 255, 20), 1.0f);
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
                    ImU32 col = (pink & 0x00FFFFFF) | ((ImU32)(255.0f * errA) << 24);
                    Glass::DrawIcon(dl, Glass::Icon::Key, ImVec2(c.x + 10.0f, c.y + 14.0f), 15.0f, col, 2.0f);
                    dl->AddText(font_medium, 15.0f, ImVec2(c.x + 28.0f, c.y + 5.0f), col, "Invalid license key");
                }
                ImGui::Dummy(ImVec2(cwid, 30.0f));
            }

            bool go = Glass::Button("Activate", true, ImVec2(ImGui::GetContentRegionAvail().x, 50.0f));
            {
                ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
                float tw = ImGui::CalcTextSize("Activate").x;
                Glass::DrawIcon(dl, Glass::Icon::CheckCircle, ImVec2((mn.x + mx.x) * 0.5f - tw * 0.5f - 20.0f, (mn.y + mx.y) * 0.5f),
                                20.0f, IM_COL32(255, 255, 255, 255), 2.4f);
            }
            ImGui::Dummy(ImVec2(0, 6));

            if (go || enter) {
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
        // ============================ MENU / SUCCESS =========================
        const float dashW = (float)win_w - 92.0f;
        const float dashH = (float)win_h - 92.0f;
        if (Glass::BeginCard("dash", Glass::Material::Regular, ImVec2(dashW, dashH))) {
            const float fade = authA * subA;
            g_glass_renderer.SetSubmitFade(fade);
            style.Alpha = ent * fade; if (style.Alpha > 1.0f) style.Alpha = 1.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 o = ImGui::GetCursorScreenPos();
            const float innerW = ImGui::GetContentRegionAvail().x;
            const float innerH = ImGui::GetContentRegionAvail().y;

            // title bar: logo in the centre, window buttons on the right
            Glass::DrawIcon(dl, Glass::Icon::Compass, ImVec2(o.x + innerW * 0.5f, o.y + 16.0f), 28.0f, IM_COL32(255, 255, 255, 255), 2.6f);
            dl->AddLine(ImVec2(o.x - 8.0f, o.y + 42.0f), ImVec2(o.x + innerW + 8.0f, o.y + 42.0f), IM_COL32(255, 255, 255, 22), 1.0f);

            if (stage == 1) {
                const float sideW = 168.0f;
                const float topY  = o.y + 56.0f;

                // sidebar
                static const Glass::Icon navIcons[3] = { Glass::Icon::Gamepad, Glass::Icon::Rocket, Glass::Icon::Crosshairs };
                const char* navLabels[3] = { prods[0].name, prods[1].name, prods[2].name };
                static const ImU32 navTints[3] = { IM_COL32(232, 104, 128, 255), IM_COL32(232, 104, 128, 255), IM_COL32(232, 104, 128, 255) };
                ImGui::SetCursorScreenPos(ImVec2(o.x, topY));
                int r = Glass::SidebarNav("nav", navIcons, navLabels, navTints, 3, sel, sideW - 8.0f, 46.0f);
                if (r >= 0 && r != sel) { sel = r; busy = false; }

                // divider
                const float sepX = o.x + sideW + 10.0f;
                dl->AddLine(ImVec2(sepX, o.y + 54.0f), ImVec2(sepX, o.y + innerH - 4.0f), IM_COL32(0, 0, 0, 28), 1.0f);
                dl->AddLine(ImVec2(sepX + 1.0f, o.y + 54.0f), ImVec2(sepX + 1.0f, o.y + innerH - 4.0f), IM_COL32(255, 255, 255, 36), 1.0f);

                // user chip, bottom-left
                {
                    ImVec2 up(o.x + 4.0f, o.y + innerH - 44.0f);
                    ImGui::SetCursorScreenPos(up);
                    Glass::Avatar("", 32.0f, IM_COL32(58, 60, 68, 255));
                    dl->AddText(font_medium, 16.0f, ImVec2(up.x + 44.0f, up.y + 6.0f), ink, kUserName);
                }

                // product information
                const float cx = o.x + sideW + 32.0f;
                const float cw2 = innerW - sideW - 32.0f;
                dl->AddText(font_medium, 17.0f, ImVec2(cx, topY + 2.0f), ink, "Product Information");

                const Prod& p = prods[sel];
                const float rowH = 38.0f, panelY = topY + 34.0f, panelH = rowH * 4 + 16.0f;
                {
                    Glass::Primitive pane; pane.cx = cx + cw2 * 0.5f; pane.cy = panelY + panelH * 0.5f;
                    pane.hw = cw2 * 0.5f; pane.hh = panelH * 0.5f; pane.corner_radius = 16.0f; pane.fade = 1.0f;
                    pane.material = Glass::Material::Thin;
                    g_glass_renderer.Submit(pane);
                }
                const char* labels[4] = { "Game", "Version", "Status", "Till Expiry" };
                const char* values[4] = { p.game, p.version, p.status, p.expiry };
                for (int i = 0; i < 4; ++i) {
                    float ry = panelY + 8.0f + rowH * i + (rowH - 17.0f) * 0.5f;
                    dl->AddText(font_medium, 16.0f, ImVec2(cx + 22.0f, ry), soft, labels[i]);
                    ImVec2 vs = font_medium->CalcTextSizeA(16.0f, 99999.0f, 0.0f, values[i]);
                    dl->AddText(font_medium, 16.0f, ImVec2(cx + cw2 - 22.0f - vs.x, ry), i == 2 ? p.statusCol : ink, values[i]);
                }

                // action button (stub)
                ImGui::SetCursorScreenPos(ImVec2(cx, panelY + panelH + 14.0f));
                const bool disabled = busy || strcmp(p.status, "Available") != 0;
                if (!busy) {
                    if (Glass::Button(kActionText, true, ImVec2(cw2, 48.0f)) && !disabled) {
                        busy = true; busyT = 0.0f;
                    } else if (ImGui::IsItemClicked() && disabled) {
                        Glass::Notify(Glass::Icon::WarnTriangle, IM_COL32(255, 159, 10, 255), "Lumin", "Not available", "This product is not ready yet.");
                    }
                } else {
                    busyT += dt * 0.55f;
                    Glass::ProgressBar(busyT > 1.0f ? 1.0f : busyT, ImVec2(cw2, 10.0f));
                    ImGui::Dummy(ImVec2(0, 4));
                    ImVec2 ts = ImGui::CalcTextSize("Preparing...");
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cw2 - ts.x) * 0.5f);
                    ImGui::PushStyleColor(ImGuiCol_Text, soft);
                    ImGui::TextUnformatted("Preparing...");
                    ImGui::PopStyleColor();
                    if (busyT >= 1.1f) { busy = false; stage = 2; okT = 0.0f; Glass::PlaySfx(Glass::Sfx::Success); }
                }
            } else {
                // ---- success screen ----
                okT += dt;
                ImGui::SetCursorScreenPos(ImVec2(o.x, o.y + innerH * 0.5f - 52.0f));
                ImGui::PushFont(font_semibold);
                lumin_center("Successfully!", ink);
                ImGui::PopFont();
                ImGui::Dummy(ImVec2(0, 8));
                lumin_center("Now you can go back to the game and have fun!", soft);
                lumin_center("This window will close automatically.", soft);
                if (okT > 3.2f) stage = 1;     // demo: return to the menu instead of quitting
            }

            // window buttons (drawn last so they stay clickable)
            {
                float tlx = o.x + innerW - (3 * 13.0f + 2 * 8.0f) - 4.0f;
                int tl = Glass::TrafficLights("wtl", tlx, o.y + 8.0f);
                if (tl == 0) ::PostMessage(hwnd, WM_CLOSE, 0, 0);
                else if (tl == 1) ::ShowWindow(hwnd, SW_MINIMIZE);
            }
            g_glass_renderer.SetSubmitFade(1.0f);
            style.Alpha = ent;
        }
        Glass::EndCard();
    }
}
