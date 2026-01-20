# Enfusion Replication Quick Reference Cheat Sheet

## Core Principles

### Authority Model
```
SERVER = Authority (source of truth)
CLIENT = Proxy (follows server)
```

### Data Flow
```
Server → Clients  ✅ (via replication)
Clients → Server  ✅ (via RPCs only)
Client → Client   ❌ (forbidden)
```

---

## Common Patterns

### ✅ CORRECT: Safe RplId Access
```c
// Using helper
RplId rplId;
if (!CRF_ReplicationHelpers.GetRplId(entity, rplId))
{
    Print("Entity has no RplComponent", LogLevel.ERROR);
    return;
}
// Use rplId safely

// Manual method
RplComponent rplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
if (!rplComp)
    return;
RplId rplId = rplComp.Id();
```

### ❌ WRONG: Direct Access (Crashes!)
```c
// This WILL crash if entity has no RplComponent!
RplId rplId = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
```

---

### ✅ CORRECT: Safe Entity Lookup
```c
// Using helper
IEntity entity = CRF_ReplicationHelpers.GetEntityFromRplId(rplId);
if (!entity)
{
    Print("Entity not found or streamed out", LogLevel.WARNING);
    return;
}
// Use entity safely

// Manual method
RplComponent rplComp = RplComponent.Cast(Replication.FindItem(rplId));
if (!rplComp)
    return;
IEntity entity = rplComp.GetEntity();
if (!entity)
    return;
```

### ❌ WRONG: Direct Access (Crashes!)
```c
// This WILL crash if item doesn't exist!
IEntity entity = RplComponent.Cast(Replication.FindItem(rplId)).GetEntity();
```

---

### ✅ CORRECT: Modifying Replicated State
```c
void UpdateValue(int newValue)
{
    // Only authority should modify
    if (!Replication.IsServer())
        return;
        
    m_ReplicatedValue = newValue;
    Replication.BumpMe(); // Signal replication system
}

// Or use helper
void UpdateValue(int newValue)
{
    m_ReplicatedValue = newValue;
    CRF_ReplicationHelpers.SafeBumpMe(); // Only calls on server
}
```

### ❌ WRONG: No Authority Check
```c
void UpdateValue(int newValue)
{
    m_ReplicatedValue = newValue;
    Replication.BumpMe(); // ⚠️ Client could call this!
}
```

---

### ✅ CORRECT: RplProp Callback
```c
[RplProp(onRplName: "OnValueChanged")]
int m_SomeValue;

void OnValueChanged()
{
    // Callback fires on PROXIES when they receive updates
    // Only authority should broadcast changes
    if (!Replication.IsServer())
        return;
        
    BroadcastUpdate(); // Safe
}
```

### ❌ WRONG: Callback Without Check
```c
void OnValueChanged()
{
    // This runs on BOTH server AND clients!
    BroadcastUpdate(); // ⚠️ Clients will try to send RPCs!
}
```

---

### ✅ CORRECT: Authority Check
```c
// Use Replication.IsServer() for authority logic
if (!Replication.IsServer())
    return;
    
// Modify replicated state
m_Value = newValue;
Replication.BumpMe();
```

### ❌ WRONG: Session Mode Check
```c
// Don't use RplSession.Mode() for authority logic!
if (RplSession.Mode() == RplMode.Client)
    return; // ⚠️ Broken on listen servers!
```

---

## RPC Patterns

### Client → Server RPC
```c
// Client-side: Send request
void RequestAction()
{
    Rpc(RpcAsk_DoAction, param1, param2);
}

// Server-side: Handle request
[RplRpc(RplChannel.Reliable, RplRcver.Server)]
protected void RpcAsk_DoAction(int param1, string param2)
{
    // Validate request
    // Perform server-side action
    // Broadcast to clients if needed
}
```

### Server → All Clients RPC
```c
// Server-side: Broadcast to all
void BroadcastEvent()
{
    if (!Replication.IsServer())
        return;
        
    Rpc(RpcDo_ShowEvent, eventData);
}

// Client-side: Handle broadcast
[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
protected void RpcDo_ShowEvent(string eventData)
{
    // Update UI
    // Play effects
}
```

### Server → Owner RPC
```c
[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
protected void RpcDo_UpdateOwner(int data)
{
    // Only the owning client receives this
}
```

---

## Entity Hierarchy & RplComponent

### ✅ CORRECT: Prefab Structure
```
Entity (Root) [HAS RplComponent]
├── Component1 [Replicated]
├── Component2 [Not replicated]
└── ChildEntity [HAS RplComponent if needs to be detachable]
    ├── ChildComponent [Replicated]
    └── ...
```

### ❌ WRONG: Missing RplComponent
```
Entity (Root) [NO RplComponent] ⚠️
└── ChildEntity [HAS RplComponent]
    └── ... ⚠️ JIP will fail!
```

**Rule**: If an entity or component uses `[RplProp]` or `[RplRpc]`, the entity MUST have RplComponent (or a parent must have it).

---

## Common Errors & Solutions

### Error: Null Pointer Exception
```
RplComponent.Cast(...) returned null
```
**Solution**: Always check for null before accessing!
```c
RplComponent rplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
if (!rplComp)
{
    Print("Entity missing RplComponent!", LogLevel.ERROR);
    return;
}
```

### Error: Item Not Found
```
Replication.FindItem(rplId) returned null
```
**Causes**:
- Item was deleted
- Item hasn't been streamed to client yet
- Invalid RplId

**Solution**: Always validate lookup results
```c
IEntity entity = CRF_ReplicationHelpers.GetEntityFromRplId(rplId);
if (!entity)
{
    Print("Entity not found or streamed out", LogLevel.WARNING);
    return;
}
```

### Error: JIP Desync
```
Join-in-progress client has wrong state
```
**Causes**:
- Missing RplComponent on prefab root
- Spawning runtime entities without prefab
- Entity hierarchy doesn't match replication hierarchy

**Solution**: 
- Add RplComponent to all prefab roots that need replication
- Use prefabs for runtime spawning
- Check hierarchy structure

---

## Testing Scenarios

### Must Test On:
- ✅ **Single Player** - Baseline functionality
- ✅ **Listen Server** - Host + remote client
- ✅ **Dedicated Server** - Headless server + clients
- ✅ **Join-In-Progress** - Client joining mid-game
- ✅ **Entity Lifecycle** - Spawn, stream, delete
- ✅ **Long Sessions** - Memory leaks, cleanup

### Red Flags:
- ⚠️ Crashes only in multiplayer
- ⚠️ Different behavior on listen vs dedicated server
- ⚠️ JIP clients see wrong state
- ⚠️ Entities disappear unexpectedly
- ⚠️ Null reference errors in logs

---

## Helper Functions Reference

```c
// Safe RplId retrieval
RplId id;
if (CRF_ReplicationHelpers.GetRplId(entity, id))
    UseId(id);

// Safe entity lookup
IEntity entity = CRF_ReplicationHelpers.GetEntityFromRplId(id);
if (entity)
    UseEntity(entity);

// Safe BumpMe
CRF_ReplicationHelpers.SafeBumpMe();

// Batch entity lookup
array<IEntity> entities = {};
CRF_ReplicationHelpers.GetEntitiesFromRplIds(rplIdArray, entities);

// Batch RplId retrieval
array<RplId> rplIds = {};
CRF_ReplicationHelpers.GetRplIdsFromEntities(entityArray, rplIds);

// Validation (debugging)
if (!CRF_ReplicationHelpers.ValidateReplicationSetup(entity, true))
    Print("Invalid setup!");
```

---

## Quick Diagnostics

### Is my entity replicated?
```c
RplComponent rplComp = CRF_ReplicationHelpers.GetRplComponent(entity);
if (rplComp)
{
    Print("Entity IS replicated");
    Print("RplId: " + rplComp.Id());
    Print("Is Authority: " + !rplComp.IsProxy());
}
else
{
    Print("Entity NOT replicated");
}
```

### Am I authority or proxy?
```c
if (Replication.IsServer())
    Print("I am AUTHORITY");
else
    Print("I am PROXY");
```

### What's my session mode?
```c
// For information only - don't use for logic!
switch (RplSession.Mode())
{
    case RplMode.Dedicated: Print("Dedicated server"); break;
    case RplMode.Client: Print("Client"); break;
    default: Print("Listen server or other"); break;
}
```

---

## Remember

1. **NULL CHECK EVERYTHING** - RplComponent, entities, RplIds
2. **AUTHORITY FIRST** - Only server modifies replicated state
3. **USE HELPERS** - Avoid repeating null check patterns
4. **TEST MULTIPLAYER** - Issues often only appear in MP
5. **READ THE DOCS** - When in doubt, check official documentation

---

**Quick Links**:
- Analysis: `REPLICATION_ISSUES_ANALYSIS.md`
- Fixes Guide: `REPLICATION_FIXES_GUIDE.md`
- Summary: `REPLICATION_REVIEW_SUMMARY.md`
- Helpers: `scripts/Game/Systems/Core/CRF_ReplicationHelpers.c`
