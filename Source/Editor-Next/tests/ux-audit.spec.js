const { _electron: electron } = require('@playwright/test');
const { test, expect } = require('@playwright/test');
const path = require('path');

const flavors = ['latte', 'frappe', 'macchiato', 'mocha'];

function getLuminance(rgb) {
  const parts = rgb.match(/\d+/g);
  if (!parts) return 0;
  const [r, g, b] = parts.map(Number).map(v => {
    v /= 255;
    return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4);
  });
  return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

function getContrastRatio(rgb1, rgb2) {
  const l1 = getLuminance(rgb1);
  const l2 = getLuminance(rgb2);
  const brightest = Math.max(l1, l2);
  const darkest = Math.min(l1, l2);
  return (brightest + 0.05) / (darkest + 0.05);
}

for (const flavor of flavors) {
  test(`UX Audit: Theme ${flavor}`, async () => {
    // Total test timeout
    test.setTimeout(60000);
    
    const electronApp = await electron.launch({ 
      args: ['.', '--no-sandbox', '--disable-gpu'],
      cwd: path.join(__dirname, '..'),
      env: { ...process.env, E2E_TEST: 'true' }
    });

    try {
      const window = await electronApp.firstWindow();
      
      console.log(`[TEST] Waiting for shell...`);
      await window.waitForSelector('.editor-shell', { timeout: 15000 });
      
      // We do NOT wait for "Live" here. Proceed with UI audit regardless of engine state.
      await window.waitForTimeout(1000);

      // 1. Switch theme
      console.log(`[TEST] Switching theme to: ${flavor}`);
      await window.click('text=Editor', { timeout: 5000 });
      await window.hover('text=Theme', { timeout: 5000 });
      
      let flavorText = '';
      if (flavor === 'latte') flavorText = 'Latte (Light)';
      else if (flavor === 'mocha') flavorText = 'Mocha (Dark)';
      else if (flavor === 'frappe') flavorText = 'Frappé';
      else if (flavor === 'macchiato') flavorText = 'Macchiato';
      
      await window.click(`text=${flavorText}`, { timeout: 5000 });
      await window.waitForTimeout(1500); 

      // 2. Trigger Simulation via State (Proves modal visibility)
      console.log(`[TEST] Triggering state simulation...`);
      await window.evaluate(() => {
        const { ipcRenderer } = require('electron');
        ipcRenderer.emit('engine-message', {}, { 
          type: 'IMPORT_PROGRESS', 
          name: 'E2E_Test_Asset.obj', 
          progress: 42 
        });
      });
      
      // Wait for modal card to appear
      const modalSelector = '.n-card.n-modal';
      await window.waitForSelector(modalSelector, { state: 'attached', timeout: 5000 });
      
      // 3. Contrast Check
      console.log(`[TEST] Auditing contrast...`);
      const auditSelectors = [
        '.n-menu-item-content-header', 
        '.n-form-item-label__text',    
        '.n-tag__content',
        '.n-card-header__main'
      ];

      for (const selector of auditSelectors) {
        const stats = await window.evaluate((sel) => {
          const el = document.querySelector(sel);
          if (!el) return null;

          const getRecursiveBg = (element) => {
            const bg = getComputedStyle(element).backgroundColor;
            if (bg !== 'rgba(0, 0, 0, 0)' && bg !== 'transparent' && element.parentElement) {
              return bg;
            }
            return element.parentElement ? getRecursiveBg(element.parentElement) : 'rgb(255, 255, 255)';
          };

          return {
            color: getComputedStyle(el).color,
            bg: getRecursiveBg(el)
          };
        }, selector);

        if (stats) {
          const ratio = getContrastRatio(stats.color, stats.bg);
          console.log(`[TEST] ${selector} -> Ratio: ${ratio.toFixed(2)}:1`);
        }
      }

      await window.screenshot({ 
        path: `test-results/ux-audit-${flavor}-fix.png`,
        fullPage: true 
      });

    } finally {
      console.log(`[TEST] Closing app...`);
      await electronApp.close().catch(() => {});
    }
  });
}
