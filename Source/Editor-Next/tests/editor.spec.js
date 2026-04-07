const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

test('editor launches and connects to engine', async () => {
  test.setTimeout(180000);
  const electronApp = await electron.launch({ 
    args: [
      '.', 
      '--engine=Main',
      '--no-sandbox',
      '--disable-gpu',
      '--disable-dev-shm-usage'
    ],
    cwd: path.join(__dirname, '..'),
    env: { ...process.env, E2E_TEST: 'true' }
  });

  electronApp.on('console', msg => {
    console.log(`[ELECTRON] ${msg.text()}`);
  });

  const window = await electronApp.firstWindow();
  
  // Wait for the UI to be loaded
  log('Waiting for UI shell to be ready...');
  await window.waitForSelector('.editor-shell', { timeout: 30000 });

  // 1. Wait for connection (status becomes "Live")
  log('Waiting for engine connection (status: Live)...');

  try {
    // Naive UI tag content
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });
    log('Connection confirmed via UI status');
  } catch (e) {
    log('Failed to find connected status tag, checking DOM...');
    const html = await window.innerHTML('.header-right');
    log(`Header Right HTML: ${html}`);
    throw e;
  }

  // 2. Verify panels are present
  try {
    log('Waiting for .hierarchy-panel to be attached...');
    await window.waitForSelector('.hierarchy-panel', { state: 'attached', timeout: 15000 });
    log('.hierarchy-panel attached');
  } catch (e) {
    log('FAILED to find .hierarchy-panel.');
    throw e;
  }
  
  // 3. Verify entities are loaded into the hierarchy
  log('Waiting for hierarchy to populate (entities to arrive)...');
  
  try {
    const entitySelector = '.entity-item';
    await window.waitForSelector(entitySelector, { timeout: 30000 });
    log('Entities found in hierarchy (using selector)');
    
    // Now verify a known entity from the UnitTest scene exists
    const cameraText = 'Main Camera/';
    const cameraLocator = window.locator(`text=${cameraText}`);
    
    log(`Checking for '${cameraText}' in hierarchy...`);
    expect(await cameraLocator.count()).toBeGreaterThan(0);
    log(`Entity '${cameraText}' found in hierarchy`);

    // 4. Test Entity Selection and Property Population
    log(`Selecting entity '${cameraText}'...`);
    await cameraLocator.click();
    
    // Give it a moment to request and render
    await window.waitForTimeout(1000);

    log('Waiting for properties panel to populate...');
    // The properties panel should now have the entity name
    const propertyPanelNameSelector = '.properties-content h3';
    await window.waitForSelector(propertyPanelNameSelector, { timeout: 10000 });
    
    const displayedName = await window.textContent(propertyPanelNameSelector);
    log(`Properties panel is showing details for: ${displayedName}`);
    expect(displayedName).toContain('Main Camera');

    // 5. Test Property Update
    log('Testing property update...');
    // In Naive UI, n-input-number uses an input with class n-input-number-input or within n-input
    const xInputSelector = '.n-input-number input';
    await window.waitForSelector(xInputSelector, { timeout: 5000 });
    
    const xInput = window.locator(xInputSelector).first();
    await xInput.fill('123.45');
    await xInput.press('Enter');
    log('Property update value filled.');

    log('E2E verification complete, editor stack is healthy');

    log('Waiting for UI to stabilize before screenshot...');
    await window.waitForTimeout(3000);

    log('Capturing full-window visual regression screenshot...');
    try {
      // Take screenshot of the healthy, connected state BEFORE stopping the engine
      await expect(window).toHaveScreenshot('editor-layout.png', {
        fullPage: true,
        maxDiffPixelRatio: 0.01,
        timeout: 60000
      });
      log('Visual regression test passed');
    } catch (err) {
      log('Visual regression failed, taking manual debug screenshot...');
      const fs = require('fs');
      const dir = path.join(__dirname, '../test-results');
      if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
      await window.screenshot({ path: 'test-results/debug-failure.png', timeout: 60000 });
      throw err;
    }

    // 6. Test Engine Stop & UI Reset
    log('Testing engine stop and UI reset...');
    await window.click('button[title="Stop Engine sidecar"]');
    
    // Wait for disconnected state in tag
    await window.waitForSelector('.n-tag__content:has-text("Disconnected")', { timeout: 10000 });
    log('UI Tag changed to Disconnected');

    // Verify hierarchy is empty
    const entityCountAfterStop = await window.locator('.entity-item').count();
    log(`Entities in hierarchy after stop: ${entityCountAfterStop}`);
    expect(entityCountAfterStop).toBe(0);

  } catch (e) {
    log('Hierarchy/Properties/Stop verification failed.');
    const html = await window.innerHTML('body');
    log('Full Body HTML snippet:');
    log(html.substring(0, 1000));
    throw e;
  }

  await electronApp.close();
});

function log(msg) {
  console.log(`[TEST] ${msg}`);
}
