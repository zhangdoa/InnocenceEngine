# Electron-Vue Editor Visual Regression Testing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish a reliable, automated visual regression testing suite for the Electron-Vue editor using Playwright, explicitly masking the 3D Engine Viewport to avoid flakiness.

**Architecture:** We will modify the existing Playwright E2E test (`editor.spec.js`) to capture a full-page screenshot after the UI is stable and connected. We will instruct Playwright to mask the `.viewport-canvas` element with a solid color during the visual comparison to isolate UI layout changes from GPU rendering differences.

**Tech Stack:** Playwright, Electron, Node.js

---

### Task 1: Update Playwright Test for Visual Regression

**Files:**
- Modify: `Source/Editor-Next/tests/editor.spec.js`

- [ ] **Step 1: Modify the E2E test to include screenshot assertion**

Update the existing test file to add the `toHaveScreenshot` assertion at the end of the successful connection and data population flow. Ensure the `.viewport-canvas` is masked.

```javascript
// Append this block to the end of the test('editor launches and connects to engine', ...) block in Source/Editor-Next/tests/editor.spec.js, just before `await electronApp.close();`

  log('Waiting for UI to stabilize before screenshot...');
  // A small hard sleep to ensure any Vue transitions or initial layouts have settled
  await window.waitForTimeout(2000);

  log('Capturing full-window visual regression screenshot (masking viewport)...');
  await expect(window).toHaveScreenshot('editor-layout.png', {
    mask: [window.locator('.viewport-canvas')],
    fullPage: true,
    maxDiffPixelRatio: 0.05 // Allow a tiny bit of variance for font rendering differences
  });

  log('Visual regression test passed');
```

- [ ] **Step 2: Verify the modification**

Run the following command to check if the file was modified correctly.

Run: `Get-Content Source/Editor-Next/tests/editor.spec.js | Select-String "toHaveScreenshot"`
Expected: Output showing the `toHaveScreenshot` line.

- [ ] **Step 3: Commit**

```bash
git add Source/Editor-Next/tests/editor.spec.js
git commit -m "test: add visual regression screenshot assertion with viewport masking"
```

### Task 2: Generate Baseline Screenshots

**Files:**
- N/A (Operation only)

- [ ] **Step 1: Run the test to generate the initial baseline**

Since this is the first time the visual regression test will run, it will fail because there is no baseline. We need to run it with the flag to generate the baseline.

Run: `cd Source/Editor-Next && npx playwright test --update-snapshots`
Expected: Test passes, and a new file `tests/editor.spec.js-snapshots/editor-layout-chromium-win32.png` (or similar depending on platform) is created.

- [ ] **Step 2: Commit the baseline screenshot**

```bash
git add "Source/Editor-Next/tests/*snapshots/*"
git commit -m "test: add initial visual regression baseline screenshot"
```

### Task 3: Verify Visual Regression Failure (Sanity Check)

**Files:**
- Modify: `Source/Editor-Next/src/components/AppLayout.vue`

- [ ] **Step 1: Introduce a deliberate CSS change**

Change the background color of the header to force a visual failure.

```javascript
// Change in Source/Editor-Next/src/components/AppLayout.vue
// Under <style scoped>
.editor-header {
  height: 40px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 10px;
  user-select: none;
  background: red; /* ADD THIS LINE */
}
```

- [ ] **Step 2: Run the test and verify it FAILS**

Run: `cd Source/Editor-Next && npm run build && npx playwright test`
Expected: Test FAILS, citing a screenshot mismatch. Playwright will generate diff images in `test-results/`.

- [ ] **Step 3: Revert the deliberate change**

Remove the `background: red;` line from `AppLayout.vue`.

- [ ] **Step 4: Re-run the test and verify it PASSES**

Run: `cd Source/Editor-Next && npm run build && npx playwright test`
Expected: Test PASSES.

- [ ] **Step 5: Clean up test results**
Run: `rm -Recurse -Force Source/Editor-Next/test-results`
Expected: Directory removed.
