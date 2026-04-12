# Instructions

- Following Playwright test failed.
- Explain why, be concise, respect Playwright best practices.
- Provide a snippet of code with the fix, if possible.

# Test info

- Name: tests\ux-audit.spec.js >> UX Audit: Theme mocha
- Location: tests\ux-audit.spec.js:26:3

# Error details

```
TimeoutError: page.click: Timeout 5000ms exceeded.
Call log:
  - waiting for locator('text=Mocha (Dark)')
    - locator resolved to <div data-dropdown-option="true" class="n-dropdown-option-body__label">Mocha (Dark)</div>
  - attempting click action
    2 × waiting for element to be visible, enabled and stable
      - element is not stable
    - retrying click action
    - waiting 20ms
    - waiting for element to be visible, enabled and stable
    - element is not stable
  - retrying click action
    - waiting 100ms
    - waiting for element to be visible, enabled and stable
  - element was detached from the DOM, retrying

```

# Test source

```ts
  1   | const { _electron: electron } = require('@playwright/test');
  2   | const { test, expect } = require('@playwright/test');
  3   | const path = require('path');
  4   | 
  5   | const flavors = ['latte', 'frappe', 'macchiato', 'mocha'];
  6   | 
  7   | function getLuminance(rgb) {
  8   |   const parts = rgb.match(/\d+/g);
  9   |   if (!parts) return 0;
  10  |   const [r, g, b] = parts.map(Number).map(v => {
  11  |     v /= 255;
  12  |     return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4);
  13  |   });
  14  |   return 0.2126 * r + 0.7152 * g + 0.0722 * b;
  15  | }
  16  | 
  17  | function getContrastRatio(rgb1, rgb2) {
  18  |   const l1 = getLuminance(rgb1);
  19  |   const l2 = getLuminance(rgb2);
  20  |   const brightest = Math.max(l1, l2);
  21  |   const darkest = Math.min(l1, l2);
  22  |   return (brightest + 0.05) / (darkest + 0.05);
  23  | }
  24  | 
  25  | for (const flavor of flavors) {
  26  |   test(`UX Audit: Theme ${flavor}`, async () => {
  27  |     // Total test timeout
  28  |     test.setTimeout(60000);
  29  |     
  30  |     const electronApp = await electron.launch({ 
  31  |       args: ['.', '--no-sandbox', '--disable-gpu'],
  32  |       cwd: path.join(__dirname, '..'),
  33  |       env: { ...process.env, E2E_TEST: 'true' }
  34  |     });
  35  | 
  36  |     try {
  37  |       const window = await electronApp.firstWindow();
  38  |       
  39  |       console.log(`[TEST] Waiting for shell...`);
  40  |       await window.waitForSelector('.editor-shell', { timeout: 15000 });
  41  |       
  42  |       // We do NOT wait for "Live" here. Proceed with UI audit regardless of engine state.
  43  |       await window.waitForTimeout(1000);
  44  | 
  45  |       // 1. Switch theme
  46  |       console.log(`[TEST] Switching theme to: ${flavor}`);
  47  |       await window.click('text=Editor', { timeout: 5000 });
  48  |       await window.hover('text=Theme', { timeout: 5000 });
  49  |       
  50  |       let flavorText = '';
  51  |       if (flavor === 'latte') flavorText = 'Latte (Light)';
  52  |       else if (flavor === 'mocha') flavorText = 'Mocha (Dark)';
  53  |       else if (flavor === 'frappe') flavorText = 'Frappé';
  54  |       else if (flavor === 'macchiato') flavorText = 'Macchiato';
  55  |       
> 56  |       await window.click(`text=${flavorText}`, { timeout: 5000 });
      |                    ^ TimeoutError: page.click: Timeout 5000ms exceeded.
  57  |       await window.waitForTimeout(1500); 
  58  | 
  59  |       // 2. Trigger Simulation via State (Proves modal visibility)
  60  |       console.log(`[TEST] Triggering state simulation...`);
  61  |       await window.evaluate(() => {
  62  |         const { ipcRenderer } = require('electron');
  63  |         ipcRenderer.emit('engine-message', {}, { 
  64  |           type: 'IMPORT_PROGRESS', 
  65  |           name: 'E2E_Test_Asset.obj', 
  66  |           progress: 42 
  67  |         });
  68  |       });
  69  |       
  70  |       // Wait for modal card to appear
  71  |       const modalSelector = '.n-card.n-modal';
  72  |       await window.waitForSelector(modalSelector, { state: 'attached', timeout: 5000 });
  73  |       
  74  |       // 3. Contrast Check
  75  |       console.log(`[TEST] Auditing contrast...`);
  76  |       const auditSelectors = [
  77  |         '.n-menu-item-content-header', 
  78  |         '.n-form-item-label__text',    
  79  |         '.n-tag__content',
  80  |         '.n-card-header__main'
  81  |       ];
  82  | 
  83  |       for (const selector of auditSelectors) {
  84  |         const stats = await window.evaluate((sel) => {
  85  |           const el = document.querySelector(sel);
  86  |           if (!el) return null;
  87  | 
  88  |           const getRecursiveBg = (element) => {
  89  |             const bg = getComputedStyle(element).backgroundColor;
  90  |             if (bg !== 'rgba(0, 0, 0, 0)' && bg !== 'transparent' && element.parentElement) {
  91  |               return bg;
  92  |             }
  93  |             return element.parentElement ? getRecursiveBg(element.parentElement) : 'rgb(255, 255, 255)';
  94  |           };
  95  | 
  96  |           return {
  97  |             color: getComputedStyle(el).color,
  98  |             bg: getRecursiveBg(el)
  99  |           };
  100 |         }, selector);
  101 | 
  102 |         if (stats) {
  103 |           const ratio = getContrastRatio(stats.color, stats.bg);
  104 |           console.log(`[TEST] ${selector} -> Ratio: ${ratio.toFixed(2)}:1`);
  105 |         }
  106 |       }
  107 | 
  108 |       await window.screenshot({ 
  109 |         path: `test-results/ux-audit-${flavor}-fix.png`,
  110 |         fullPage: true 
  111 |       });
  112 | 
  113 |     } finally {
  114 |       console.log(`[TEST] Closing app...`);
  115 |       await electronApp.close().catch(() => {});
  116 |     }
  117 |   });
  118 | }
  119 | 
```