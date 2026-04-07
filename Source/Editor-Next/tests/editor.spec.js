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

  // 1. Wait for connection (status becomes "● Connected")
  const statusSelector = '.status-dot.connected';
  log('Waiting for engine connection (status: Connected)...');
  
  try {
    await window.waitForSelector(statusSelector, { timeout: 120000 });
    log('Connection confirmed via UI status');
  } catch (e) {
    log('Failed to find connected status selector, checking if element exists...');
    const statusExists = await window.locator('.status-dot').count();
    if (statusExists > 0) {
      log('Status dot element found in DOM but not connected.');
    } else {
      log('Status dot element NOT found in DOM!');
    }
    throw e;
  }

  // 2. Verify panels are present
  // We skip .dv-pane-content as dockview internal classes might vary.
  // Instead we verify our actual component is mounted.
  try {
    log('Waiting for .hierarchy-panel to be attached...');
    await window.waitForSelector('.hierarchy-panel', { state: 'attached', timeout: 15000 });
    log('.hierarchy-panel attached');
  } catch (e) {
    log('FAILED to find .hierarchy-panel. Printing body HTML:');
    const html = await window.innerHTML('body');
    log(html); // Print full HTML
    throw e;
  }
  
  // 3. Verify entities are loaded into the hierarchy
  log('Waiting for hierarchy to populate (entities to arrive)...');
  
  try {
    const entitySelector = '.entity-item';
    await window.waitForSelector(entitySelector, { timeout: 30000 });
    log('Entities found in hierarchy (using selector)');
    
    // Now verify a known entity from the UnitTest scene exists
    const cameraExists = await window.locator('text=Main Camera/').count();
    log(`Entity 'Main Camera/' found: ${cameraExists > 0}`);
    expect(cameraExists).toBeGreaterThan(0);
  } catch (e) {
    log('Hierarchy failed to populate or expected entity not found.');
    const html = await window.innerHTML('.hierarchy-panel');
    log('Hierarchy panel HTML:');
    log(html);
    throw e;
  }

  log('E2E verification complete, editor stack is healthy');

  log('Waiting for UI to stabilize before screenshot...');
  // A small hard sleep to ensure any Vue transitions or initial layouts have settled
  await window.waitForTimeout(2000);

  log('Capturing full-window visual regression screenshot (masking viewport)...');
  await expect(window).toHaveScreenshot('editor-layout.png', {
    mask: [window.locator('.viewport-canvas')],
    fullPage: true,
    maxDiffPixelRatio: 0.01 // Lower threshold to catch real regressions
  });

  log('Visual regression test passed');

  await electronApp.close();
});

function log(msg) {
  console.log(`[TEST] ${msg}`);
}
