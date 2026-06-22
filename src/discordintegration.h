#pragma once

// Discord Rich Presence for Kaentake.
//
// Uses the lightweight discord-rpc library (named-pipe IPC, no runtime DLL) vendored
// under src/discord/, plus v83-specific memory readers ported from the Orion edit.
// Presence is refreshed on a background thread that polls the client state itself
// (self-poll login detection — no bypass.cpp edit required).
//
// Installed from hook.h::AttachClientHooks() via AttachDiscordRPC().
class DiscordIntegration {
public:
    static void Start();        // called once from AttachDiscordRPC()
    static void RefreshLoop();  // background refresh thread body

    static int  runCounter;     // skip the first few ticks while the client warms up
    static bool show_charname;
    static bool show_charlevel;
    static bool show_charjob;
    static bool show_map;
};

// hook.h installer
void AttachDiscordRPC();
