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
  });

  electronApp.on('console', msg => {
    console.log(`[ELECTRON] ${msg.text()}`);
  });

  const window = await electronApp.firstWindow();
  
  // Wait for the UI to be loaded
  log('Waiting for UI shell to be ready...');
  await window.waitForSelector('.editor-shell', { timeout: 30000 });

  // 1. Wait for connection (status becomes "● Connected")
  const statusSelector = '.engine-status.connected';
  log('Waiting for engine connection (status: ● Connected)...');
  
  try {
    await window.waitForSelector(statusSelector, { timeout: 120000 });
    log('Connection confirmed via UI status');
  } catch (e) {
    log('Failed to find connected status selector, checking if element exists...');
    const statusExists = await window.locator('.engine-status').count();
    if (statusExists > 0) {
      const content = await window.textContent('.engine-status');
      log(`Current status text: "${content.trim()}"`);
    } else {
      log('Engine status element NOT found in DOM!');
    }
    throw e;
  }
  
  const statusText = await window.textContent('.engine-status');
  expect(statusText).toContain('● Connected');

  // 2. Verify panels are present
  await window.waitForSelector('.dv-pane-content', { timeout: 10000 });
  
  // 3. Verify entities are loaded into the hierarchy
  log('Waiting for hierarchy to populate (entities to arrive)...');
  const entitySelector = '.entity-item';
  
  try {
    await window.waitForSelector(entitySelector, { timeout: 30000 });
    const entityCount = await window.locator(entitySelector).count();
    log(`Found ${entityCount} entities in hierarchy`);
    expect(entityCount).toBeGreaterThan(0);

    const firstEntityName = await window.locator(`${entitySelector} .name`).first().textContent();
    log(`First entity: ${firstEntityName}`);
    expect(firstEntityName.length).toBeGreaterThan(0);
  } catch (e) {
    log('Hierarchy failed to populate in time.');
    // Log the whole page content for debugging
    // const body = await window.innerHTML('body');
    // log(`Body content sample: ${body.substring(0, 500)}`);
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
    maxDiffPixelRatio: 0.05 // Allow a tiny bit of variance for font rendering differences
  });

  log('Visual regression test passed');

  await electronApp.close();
});

function log(msg) {
  console.log(`[TEST] ${msg}`);
}
