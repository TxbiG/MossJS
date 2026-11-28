# Lesson 7 — Scene Flow and Game States

## Goal

Separate the game lifecycle from the player's state. The player can be falling while the game is still in the `Playing` state; these are different layers of state.

## 1. Define game-level states

```cpp
enum class GameState {
    Title,
    Playing,
    Paused,
    PlayerDied,
    LevelComplete
};
```

Use one owner for state transitions. For example, only the game controller changes `Playing` to `Paused`, `PlayerDied` or `LevelComplete`. This avoids every entity deciding independently whether the game is active.

## 2. Use a small Scene interface

A scene manager is application code. Start with the minimum interface you need:

```cpp
class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() = 0;
    virtual void handleInput() = 0;
    virtual void update(float fixed_dt) = 0;
    virtual void render(Moss_Renderer* renderer) = 0;
    virtual void onExit() = 0;
};
```

A `TitleScene`, `LevelScene` and `ResultsScene` can implement this interface. Keep the window, renderer and camera owned by the application rather than destroying and recreating them on every scene transition.

## 3. Pause simulation without pausing platform events

When paused, keep calling `Moss_PollEvents()` so the application remains responsive. Skip gameplay simulation while paused, but continue rendering the pause overlay. The outer application loop remains active until `Moss_ShouldWindowClose(window)` becomes true.

A safe transition order is:

1. Finish the current update tick.
2. Call `onExit()` on the old scene.
3. Release scene-owned objects and resources.
4. Construct or activate the next scene.
5. Call `onEnter()`.

Do not delete a scene while one of its callbacks is still running.

## 4. Make restart deterministic

A level reset should restore player position and velocity, reset hazard state, reactivate pickups, clear temporary buffs, reset timers and restore the intended score policy. Decide whether score persists between levels or resets on death. Keep this logic in one `resetLevel()` method.

## Checkpoint

The game has a title screen, enters play, pauses, resumes, handles death and displays a level-complete state. Test every transition, including repeated pause/resume and restarting after death.

## Extension

Add a two-level progression sequence. Store level definitions separately from scene code so a new level is primarily data rather than another copy of the gameplay implementation.
