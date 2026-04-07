# Spec: Electron-Vue Editor Visual Regression Testing (Screenshot E2E)

## Objective
Establish a reliable, automated visual regression testing suite for the Electron-Vue editor using Playwright's screenshot comparison capabilities (`expect(page).toHaveScreenshot()`). The goal is to ensure UI changes (Naive UI updates, Dockview layout modifications) do not introduce unintended visual regressions, while explicitly ignoring the 3D Engine Viewport to avoid flakiness caused by GPU/driver rendering differences.

## Key Features

1. **Full Application Screenshotting:** The test suite will capture the entire Electron application window to verify the overall layout, themes, and panel placements.
2. **Viewport Masking:** The `<canvas>` element containing the 3D Engine Viewport (class `.viewport-canvas`) will be explicitly masked (blacked out or ignored) during the screenshot comparison. This guarantees that pixel-level differences in engine rendering (e.g., antialiasing, lighting calculations) across different hardware do not cause false test failures.
3. **Automated Baseline Generation:** Playwright will automatically generate and update baseline screenshots when run with the `--update-snapshots` flag.
4. **Integration with Existing E2E:** The visual regression tests will be integrated into the existing `editor.spec.js` file, executing after the application has fully loaded, connected to the engine, and populated the initial data (e.g., the Scene Hierarchy).

## Implementation Plan

### 1. Update Playwright Configuration
- Ensure Playwright is configured to handle screenshots. While the default configuration often works, we may need to specify a `testMatch` or `snapshotDir` if the defaults aren't suitable. Given the current setup, we can likely proceed without a dedicated `playwright.config.js` and rely on default behavior.

### 2. Modify `editor.spec.js`
- **Wait for Stability:** After the existing checks (connection confirmed, hierarchy populated), add explicit waits to ensure the UI is completely stable (e.g., waiting for animations to finish, fonts to load).
- **Take Screenshot with Masking:** Implement the screenshot assertion:
  ```javascript
  await expect(window).toHaveScreenshot('editor-layout.png', {
    mask: [window.locator('.viewport-canvas')],
    fullPage: true // Ensure the whole window is captured
  });
  ```
  *(Note: Playwright's `mask` option covers the specified locators with a solid color, effectively ignoring them during the pixel comparison).*

### 3. Workflow for Updating Baselines
- Document the command required to generate or update the baseline screenshots:
  ```bash
  npx playwright test --update-snapshots
  ```
- This command should be run whenever intentional UI changes are made to establish the new "source of truth."

## Trade-offs Considered
- **Masking vs. Component-Level Snapshots:** We chose to mask the viewport in a full-window snapshot (Option 2) rather than taking isolated snapshots of individual panels (Option 3). This provides better coverage of the overall layout, ensuring that panels don't overlap or render incorrectly relative to each other, while still avoiding the flakiness of the 3D rendering.

## Testing & Verification
1. Run the test once with `--update-snapshots` to generate the initial baseline.
2. Run the test again normally to ensure it passes against the new baseline.
3. Make a deliberate (temporary) CSS change (e.g., change a background color) and run the test to verify that the visual regression is caught.
4. Revert the CSS change and ensure the test passes again.
