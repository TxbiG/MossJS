# Lesson 8 — HUD and Feedback

## Goal

Show score, lives and buff timers while keeping gameplay logic independent of the presentation layer.

## 1. Load and upload a font

The renderer header declares `Moss_FontLoad`, `Moss_FontUploadAtlas`, `Moss_FontDestroy` and `Moss_RendererDrawText`. Check the return values before using the font, and check whether the selected backend supports font rendering in the current build.

```cpp
Moss_Font* hud_font = Moss_FontLoad("assets/fonts/YourFont.ttf", 22.0f);
if (hud_font != nullptr) {
    if (!Moss_FontUploadAtlas(renderer, hud_font)) {
        Moss_FontDestroy(hud_font);
        hud_font = nullptr;
    }
}
```

Use a font file that you are licensed to distribute with your game. Keep font paths relative to a known asset root rather than relying on the process working directory.

## 2. Render current values

```cpp
void drawHud(Moss_Renderer* renderer, Moss_Font* font,
             int score, int lives)
{
    if (font == nullptr)
        return;

    const std::string line = "Score: " + std::to_string(score) + "  Lives: " + std::to_string(lives);

    Moss_DrawTextDesc text{};
    text.font = font;
    text.text = line.c_str();
    text.position = Float2(24.0f, 24.0f);
    text.color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    text.scale = 1.0f;
    Moss_RendererDrawText(renderer, &text);
}
```

Include `<string>`. The text descriptor references the string while the draw call is being recorded, so keep the string alive through that call. Draw the HUD after the world so it appears over the playfield according to the renderer's ordering rules.

## 3. Separate feedback from score logic

The pickup system changes the score. The HUD reads the score. A small event or feedback queue can request a popup, sound, particle or brief screen flash after a pickup. This prevents UI code from becoming the place where game rules are stored.

Use feedback sparingly: a brief colour pulse on a pickup, a short score popup or a distinct low-health cue can communicate useful state without obscuring the player.

## 4. Resource ownership

Destroy the font before destroying the renderer. If font upload fails, report the asset path and selected backend in your log. Do not crash because an optional HUD asset is missing; use a fallback diagnostic or disable the HUD text while keeping the game playable.

## Checkpoint

The score and lives update without restarting the game. Confirm that the text remains within the visible screen area at the target window size.

## Extension

Add a compact buff timer line. Format time as seconds with one decimal place, and show only buffs that are currently active.
