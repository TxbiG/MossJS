# Lesson 7 — HUD and Game Feedback

## Goal

Communicate health, score, current wave and enemy count with a lightweight HUD. The sample uses Moss's font API rather than assuming a ready-made gameplay UI widget layer.

## 1. Load a font

```cpp
Moss_Font* hud_font = Moss_FontLoad("assets/fonts/YourFont.ttf", 22.0f);
if (hud_font != nullptr && !Moss_FontUploadAtlas(renderer, hud_font)) {
    Moss_FontDestroy(hud_font);
    hud_font = nullptr;
}
```

`Moss_FontLoad` and `Moss_FontUploadAtlas` are declared in the public renderer header. Check whether the active GPU backend supports fonts using the renderer's backend capability query before relying on text for critical gameplay information.

## 2. Draw a line of HUD text

```cpp
void drawText(Moss_Renderer* renderer, Moss_Font* font,
              const std::string& value, float x, float y)
{
    if (font == nullptr)
        return;

    Moss_DrawTextDesc desc{};
    desc.font = font;
    desc.text = value.c_str();
    desc.position = Float2(x, y);
    desc.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    desc.scale = 1.0f;
    Moss_RendererDrawText(renderer, &desc);
}
```

Build strings from current game state, then call the helper while the string remains alive. For example, `Health: 80   Score: 120   Wave: 3`.

## 3. Prioritise feedback

Health should be visible without blocking the centre of the arena. A new wave should be announced briefly. Damage should produce an obvious but short response. A buff should show its remaining duration or a clear active-state icon. Use contrast and shape in addition to colour so the interface remains readable for more players.

## 4. Do not bury rules in UI code

The HUD is a read-only view of `Player3D`, `GameProgress`, `WaveState` and active buffs. It does not decide whether a wave is complete or whether an enemy has been defeated.

## 5. Resource lifetime

Destroy the font before destroying the renderer. Handle font failure gracefully so the game can still be tested with debug output or another temporary presentation method.

## Checkpoint

The HUD updates when health changes, when an enemy is defeated and when a new wave begins. It remains readable at the game's target resolution.
