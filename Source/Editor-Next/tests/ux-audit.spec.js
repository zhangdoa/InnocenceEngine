const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

const flavors = ['latte', 'frappe', 'macchiato', 'mocha'];

for (const flavor of flavors) {
  test(`UX Audit: Theme ${flavor}`, async () => {
    test.setTimeout(180000);
    const electronApp = await electron.launch({ 
      args: ['.', '--no-sandbox', '--disable-gpu'],
      cwd: path.join(__dirname, '..'),
      env: { ...process.env, E2E_TEST: 'true' }
    });

    const window = await electronApp.firstWindow();
    await window.waitForSelector('.editor-shell', { timeout: 30000 });

    // Wait for connection
    console.log(`[TEST] Waiting for engine connection...`);
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });

    // 1. Switch to the flavor
    console.log(`[TEST] Switching to theme: ${flavor}`);
    await window.click('text=Editor');
    await window.hover('text=Theme');
    
    let flavorText = '';
    if (flavor === 'latte') flavorText = 'Latte (Light)';
    else if (flavor === 'mocha') flavorText = 'Mocha (Dark)';
    else if (flavor === 'frappe') flavorText = 'Frappé';
    else if (flavor === 'macchiato') flavorText = 'Macchiato';
    
    await window.click(`text=${flavorText}`);
    await window.waitForTimeout(1000);

    // 2. Mock an import in progress to check UX
    console.log(`[TEST] Mocking import progress...`);
    await window.evaluate(() => {
      // Access the reactive store via the global window object if exposed, 
      // or just dispatch the events that AppLayout listens to
      const msg = {
        type: 'IMPORT_PROGRESS',
        name: 'Sponza_Atrium.fbx',
        progress: 45
      };
      // We can use ipcRenderer to simulate an engine message
      const { ipcRenderer } = require('electron');
      ipcRenderer.emit('engine-message', {}, msg);
    });
    
    await window.waitForSelector('text=Processing Assets', { timeout: 5000 });
    await window.waitForTimeout(500);

    // 3. Take screenshot of Import Modal
    console.log(`[TEST] Capturing import screenshot for ${flavor}...`);
    await window.screenshot({ 
      path: `test-results/ux-audit-${flavor}-import.png`,
      fullPage: true 
    });

    // 4. Close modal and select entity
    await window.evaluate(() => {
      const { ipcRenderer } = require('electron');
      ipcRenderer.emit('engine-message', {}, { type: 'IMPORT_FINISHED', success: true, name: 'Sponza_Atrium.fbx' });
    });
    await window.waitForTimeout(500);

    await window.waitForSelector('.entity-item', { timeout: 30000 });
    await window.click('.entity-item >> nth=0');
    await window.waitForTimeout(1000);

    // 5. Take final screenshot
    console.log(`[TEST] Capturing final screenshot for ${flavor}...`);
    await window.screenshot({ 
      path: `test-results/ux-audit-${flavor}.png`,
      fullPage: true 
    });

    await electronApp.close();
  });
}
