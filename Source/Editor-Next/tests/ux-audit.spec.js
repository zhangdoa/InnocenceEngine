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
    test.setTimeout(180000);
    const electronApp = await electron.launch({ 
      args: ['.', '--no-sandbox', '--disable-gpu'],
      cwd: path.join(__dirname, '..'),
      env: { ...process.env, E2E_TEST: 'true' }
    });

    const window = await electronApp.firstWindow();
    await window.waitForSelector('.editor-shell', { timeout: 30000 });
    await window.waitForSelector('.n-tag__content:has-text("Live")', { timeout: 120000 });

    // 1. Switch theme
    await window.click('text=Editor');
    await window.hover('text=Theme');
    
    let flavorText = '';
    if (flavor === 'latte') flavorText = 'Latte (Light)';
    else if (flavor === 'mocha') flavorText = 'Mocha (Dark)';
    else if (flavor === 'frappe') flavorText = 'Frappé';
    else if (flavor === 'macchiato') flavorText = 'Macchiato';
    
    await window.click(`text=${flavorText}`);
    await window.waitForTimeout(2000); // Wait longer for variables to settle

    // 2. Select entity to show labels
    await window.waitForSelector('.entity-item', { timeout: 30000 });
    await window.click('.entity-item >> nth=0');
    await window.waitForTimeout(1000);

    // 3. Contrast Check with Recursive BG search
    console.log(`[TEST] Checking contrast for theme: ${flavor}`);
    
    const auditSelectors = [
      '.n-menu-item-content-header', 
      '.n-form-item-label__text',    
      '.n-tag__content'
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
        console.log(`[TEST] ${selector} -> Color: ${stats.color}, BG: ${stats.bg}, Ratio: ${ratio.toFixed(2)}:1`);
        // We warn instead of failing for now to collect all data
        if (ratio < 3.0) {
          console.error(`[FAIL] ${selector} has poor contrast!`);
        }
      }
    }

    await window.screenshot({ 
      path: `test-results/ux-audit-${flavor}-diagnostic.png`,
      fullPage: true 
    });

    await electronApp.close();
  });
}
