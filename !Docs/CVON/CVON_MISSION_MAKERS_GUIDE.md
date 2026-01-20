# MISSION MAKER GUIDE TO CVON
## STARTING OUT
- **CRF_Lobby** Ensure this has the CVON_VONGamemodeComponent inside of it.

### **CVON_VONGamemodeComponent**
- **Use Faction Encryption**: This just means are frequencies global or tied to a faction. When teamspeak checks if you can hear a radio frequency it checks for faction key. Turning this off means it only checks for frequency.
- **Teamspeak Checks**: More of a sanity thing as it turns of the Teamspeak warning you get when you first load into a world with Teamspeak.
- **Shared Frequencies**: This lets you define faction Ids and the frequency they share, it's an array so you can have multiple. All this does on the teamspeak side is merge the faction keys together to create a unique factionkey only people using specific radios can here.
- **Freq Config**: I go into this later but this is essentially a master reference for what element gets assigned what radio frequencies. Unless you are making a CCE/CCO you don't need to touch this.

### **SCR_Faction**
If you look just below where you define callsigns you will now see "Group Frequency Overrides".
Look here as well if you see a warning in the console that your element is missing a frequency configuration. You can fix it by adding it in here.
#### **Group Frequency Override**
- This lets you define groups outside of the typical structure later talked about in Freq Config.
- **Group Names**: This is the name of the group we are looking to override, MMG 1 for example.
- **SR Frequencies**: This is an array because there can be multiple SRs assigned to one person. If you leave this blank the SR channel name will just be the name of the group.
- **LR Frequencies**: This is an array because there can be multiple LRs assigned to one person. The first one should ALWAYS be the element they are directly attached to. If theres any more LR radios that don't have a frequency defined here they get set to the highest headquarters frequency.

## **Freq Config**
- Same concept as group frequency overrides. However this uses a normalized name to find groups.
- SR frequencies if left blank will be assigned as the name of the group in game.
- This means if you define a frequency container with a name of 1-1 it will go to 1-1.
- However if you define a container with a name of Armor, then Armor 1, Armor 2, etc... will get assigned that frequency.
- You can see this in the base CRF config where Armor on gets assigned the Armor LR net and that is it.
- Many leaders have two LR radios, if an LR frequency is left undefined or for instance if the first LR took the first frequency and there is no second it will default to the highest headquarters.
- Once again this is how I define, TH, ARMOR, MED, etc... so they will always be assigned to the highest headquarters and an LR on their respective, MEDIC, FIRES, AIR nets.

## **CRF Frequency Config Explained**
- **Platoon Linked**: This means that the number at the beginning or end of the element will represent it's platoon. They will always be assigned the platoon corrosponding to their number unless you use the overrides.
- **Highest HQ Linked**: This means that whatever that very first element at the top of slotting, whether it be COY or PLT, will be the highest HQ and these elements leaders will have their second LR tuned to it unless you use the overrides.
- **Example**: In my mission Ruha Salient I a Company for blufor, so all my gun trucks where being automatically assigned to COY, I used the overrides so I can manually assign them to platoons as in this mission I specifically wanted vehicles in the platoons.
### **Platoon Linked**
- Assume everything bellow is for 1PLT, 2PLT and 3PLT
- 1PLT
- 1-1
- 1-2
- 1-3
- 1-4
- MMG 1
- HMG 1
- HAT 1
- MAT 1
- AA 1

### **Highest HQ Linked**
- Engineers
- Mortars
- Vehicles
- Helicopters
- Sniper Team
- Medic