# CVON TeamSpeak Plugin User Guide

This guide will walk you through downloading, installing, and using the Coalition Voice Over Network (CVON) TeamSpeak plugin for enhanced radio communication in Arma Reforger.

## Table of Contents
1. [What is CVON?](#what-is-cvon)
2. [System Requirements](#system-requirements)
3. [Download and Installation](#download-and-installation)
4. [First Time Setup](#first-time-setup)
5. [Using CVON In-Game](#using-cvon-in-game)
6. [Plugin Features](#plugin-features)
7. [Troubleshooting](#troubleshooting)
8. [FAQ](#faq)

## What is CVON?

CVON (Coalition Voice Over Network) is a TeamSpeak 3 plugin that provides realistic radio communication for Arma Reforger missions using the Coalition Reforger Framework. It simulates military radio systems with:

- **Realistic Radio Effects**: Static, distance-based audio quality, and radio interference
- **Faction-Based Communication**: Separate frequencies for different military factions
- **Hierarchical Command Structure**: Squad, Platoon, and Company-level communication networks
- **Automatic Frequency Assignment**: Based on your in-game role and unit
- **3D Positional Audio**: Direct speech that varies with distance and obstacles, as well as stereo-based radio communications
- **Voice Volume Control**: Control how far your voice travels in game

### BUT WHY? VON works fine.

The base implementation of the VON (voice over network) system in reforger has issues where larger player counts fight over smaller areas. This is something that is rarely seen in conflict servers, and very common in community-oriented events.

## System Requirements

### Required Software
- **TeamSpeak 3 Client** (Latest version)
- **Arma Reforger** with Coalition Reforger Framework
- **Windows Operating System** (Windows 10/11 recommended)

### Hardware Requirements
- **Microphone** (headset recommended for best experience)
- **Speakers or Headphones**
- **Stable Internet Connection**

## Download and Installation

### Step 1: Download the Plugin

1. **Download the plugin** from the Github Release page:
   ```
   https://github.com/CoalitionArma/Coalition-VON/releases/
   ```

2. **Save the file** to a location you can easily find (e.g., Downloads folder)

### Step 2: Install the Plugin

1. **Close TeamSpeak 3** completely (check system tray)

2. **Double-click** the downloaded `.ts3_plugin` file
   - TeamSpeak 3 should automatically open and prompt you to install the plugin

3. **Alternative Installation Method**:
   - Open TeamSpeak 3
   - Go to `Tools` → `Options` → `Addons`
   - Click `Install Addon`
   - Navigate to and select the downloaded plugin file
   - Click `Open`

4. **Confirm Installation**:
   - Click `Yes` when prompted to install the plugin
   - Restart TeamSpeak 3 when prompted

### Step 3: Verify Installation

1. **Open TeamSpeak 3**
2. **Go to** `Tools` → `Options` → `Addons`
3. **Look for** "Coalition Teamspeak Plugin" in the list
4. **Ensure it's enabled** (checkbox should be checked)

## First Time Setup

### Step 1: Configure TeamSpeak Settings

#### Audio Settings
1. **Go to** `Tools` → `Options` → `Capture`
2. **Set up your microphone**:
   - Select your microphone device
   - Test microphone levels
   - Configure push-to-talk key (recommended) or voice activation

3. **Go to** `Tools` → `Options` → `Playback`
4. **Configure audio output**:
   - Select your speakers/headphones
   - Test audio levels

#### Push-to-Talk Configuration
1. **Go to** `Tools` → `Options` → `Hotkeys`
2. **Set up hotkeys for**:
   - Push-to-Talk (primary communication)
   - Whisper (if desired)
   - Toggle microphone mute

### Step 2: Plugin Configuration

1. **The plugin will automatically configure** most settings when you join a CVON-enabled server
2. **No manual configuration** is typically required for basic functionality
3. **Advanced users** can access plugin settings through the TeamSpeak addons menu

## Using CVON In-Game

### Step 1: Joining a CVON-Enabled Server

1. **Launch Arma Reforger**
2. **Join a server** running Coalition Reforger Framework with CVON enabled
3. **Start TeamSpeak 3** (if not already running)
4. **Connect to the server's TeamSpeak** (server info usually provided in-game or on server website)

### Step 2: In-Game Voice Communication

#### Automatic Setup
- **The plugin automatically detects** your in-game role and faction
- **Radio frequencies are assigned** based on your unit and position in the command structure
- **No manual tuning required** for standard roles

Your ability to speak will now come from teamspeak hotkeys rather than in-game hotkeys.

## Troubleshooting

### Common Issues and Solutions

#### Plugin Not Working
**Symptoms**: No radio effects, can't hear teammates, plugin appears inactive

**Solutions**:
1. **Verify plugin installation**:
   - Check `Tools` → `Options` → `Addons`
   - Ensure "Coalition Teamspeak Plugin" is listed and enabled
   - Restart TeamSpeak if needed

2. **Check game compatibility**:
   - Ensure you're playing on a CVON-enabled server
   - Verify Coalition Reforger Framework is active

3. **Restart both applications**:
   - Close TeamSpeak and Arma Reforger completely
   - Restart TeamSpeak first, then launch the game

#### Can't Hear Other Players
**Symptoms**: Plugin is active but no voice communication

**Solutions**:
1. **Check faction assignment**:
   - Ensure you're on the same faction as teammates you're trying to reach
   - Verify you're using the correct communication method (direct vs radio)

2. **Verify TeamSpeak connection**:
   - Confirm you're connected to the correct TeamSpeak server
   - Check that other players are also connected

3. **Test audio settings**:
   - Test microphone in TeamSpeak settings
   - Verify output device is working
   - Check volume levels

#### Radio Not Working
**Symptoms**: Direct speech works but radio communication doesn't

**Solutions**:
1. **Check in-game role**:
   - Ensure your character has radio equipment
   - Verify you're using correct in-game radio controls

2. **Frequency assignment**:
   - Wait for automatic frequency assignment (may take a few seconds)
   - Try re-slotting if frequencies don't assign

3. **Plugin synchronization**:
   - Check console for CVON-related messages
   - Restart TeamSpeak if synchronization fails

#### Poor Audio Quality
**Symptoms**: Distorted audio, excessive static, unclear voices

**Solutions**:
1. **Check internet connection**:
   - Ensure stable connection to TeamSpeak server
   - Consider connection quality to game server

2. **Audio settings**:
   - Verify microphone levels aren't too high (causing distortion)
   - Check that other applications aren't interfering with audio

3. **TeamSpeak codec**:
   - Ensure TeamSpeak is using appropriate audio codec
   - Check server audio quality settings

### Plugin-Specific Troubleshooting

#### Plugin Won't Install
1. **Run TeamSpeak as Administrator** when installing
2. **Check file integrity** of downloaded plugin
3. **Temporarily disable antivirus** during installation
4. **Ensure TeamSpeak is fully closed** before installation

#### Plugin Crashes TeamSpeak
1. **Update TeamSpeak** to latest version
2. **Disable other plugins** temporarily to test for conflicts
3. **Reinstall the plugin** with a fresh download

## FAQ

### General Questions

**Q: Do I need to manually tune radio frequencies?**
A: No, the plugin automatically assigns frequencies based on your in-game role and unit.

**Q: Can I talk to enemy factions?**
A: No, unless specifically configured by the mission maker, factions cannot communicate with each other.

**Q: What happens if I die in-game?**
A: You'll be moved to spectator channels where you can communicate with other spectators but not interfere with live players.

**Q: Do I need a specific microphone or headset?**
A: Any microphone will work, but a headset is recommended for the best experience and to prevent audio feedback.

### Technical Questions

**Q: Does the plugin work with other TeamSpeak plugins?**
A: The CVON plugin is designed to be compatible with most other TeamSpeak plugins, but conflicts may occur with other voice modification plugins.

**Q: Can I use voice activation instead of push-to-talk?**
A: Yes, but push-to-talk is strongly recommended for tactical gameplay to avoid accidental transmissions.

**Q: What if the server doesn't support CVON?**
A: The plugin will remain inactive, and you can use TeamSpeak normally without any interference.

**Q: Is the plugin safe to use?**
A: Yes, the plugin is developed specifically for Coalition events and contains no malicious code.

### Gameplay Questions

**Q: How do I know which radio frequency I'm on?**
A: Check your in-game radio display or ask your squad leader. The plugin handles frequency assignment automatically.

**Q: Can squad leaders hear all squad communications?**
A: Yes, squad leaders typically have access to their squad's frequency plus command networks.

**Q: What if I'm not hearing anyone but others can hear me?**
A: Check your TeamSpeak playback settings and ensure you're connected to the correct server with proper permissions.

## Getting Help

If you continue to experience issues:

1. **Check the Coalition Discord** for community support
2. **Review console output** in both Arma Reforger and TeamSpeak for error messages
3. **Verify all software is up to date** (TeamSpeak, Arma Reforger, plugin)
4. **Contact Coalition technical support** through official channels

## Version Information

- **Plugin Version**: 1.8
- **Compatible with**: TeamSpeak 3 Client
- **Last Updated**: Check Coalition website for latest version
- **Download Link**: https://coalitiongroup.net/files/Coalition_Teamspeak_PluginV1.9.ts3_plugin

---

*For the most up-to-date information and support, visit the official Coalition Group Discord server.*
