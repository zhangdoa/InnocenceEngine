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

  try {
    electronApp.on('console', msg => {
      console.log(`[ELECTRON] ${msg.text()}`);
    });

    const window = await electronApp.firstWindow();
    
    // Wait for the UI to be loaded
    log('Waiting for UI shell to be ready...');
    await window.waitForSelector('.editor-shell', { timeout: 30000 });

    // 1. Wait for connection (status becomes "Live")
    log('Waiting for engine connection (status: Live)...');
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });
    log('Connection confirmed via UI status');

    // 2. Verify panels are present
    log('Waiting for .hierarchy-panel to be attached...');
    await window.waitForSelector('.hierarchy-panel', { state: 'attached', timeout: 15000 });
    log('.hierarchy-panel attached');
    
    // 3. Verify entities are loaded
    log('Waiting for hierarchy to populate...');
    const entitySelector = '.entity-item';
    await window.waitForSelector(entitySelector, { timeout: 30000 });
    log('Entities found in hierarchy');
    
    const mockEntityText = 'Main Camera/';
    const entityLocator = window.locator(`text=${mockEntityText}`);
    expect(await entityLocator.count()).toBeGreaterThan(0);
    log(`Entity '${mockEntityText}' found`);

    // 4. Test Selection
    await entityLocator.first().click();
    await window.waitForSelector('.properties-content', { timeout: 10000 });
    log('Properties panel populated');

    // 5. Test Engine Stop
    log('Testing engine stop...');
    await window.click('button:has-text("Stop")');
    
    // Wait for "Offline" state
    await window.waitForSelector('.n-tag__content:has-text("Offline")', { timeout: 15000 });
    log('UI Tag changed to Offline');

    // Verify hierarchy reset
    const count = await window.locator('.entity-item').count();
    expect(count).toBe(0);
    log('Hierarchy cleared successfully');

  } finally {
    await electronApp.close().catch(() => {});
  }
});

function log(msg) {
  console.log(`[TEST] ${msg}`);
}
