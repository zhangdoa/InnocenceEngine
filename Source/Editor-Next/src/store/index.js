import { uiStore } from './uiStore'
import { connectionStore } from './connectionStore'
import { sceneStore } from './sceneStore'
import { assetStore } from './assetStore'
import { devToggleStore } from './devToggleStore'
import { renderTargetStore } from './renderTargetStore'
import { taskGraphStore } from './taskGraphStore'

export {
  uiStore,
  connectionStore,
  sceneStore,
  assetStore,
  devToggleStore,
  renderTargetStore,
  taskGraphStore,
}

// Expose store singletons on window so Playwright scene-vertical / contract
// tests can drive them without re-resolving module paths against the Vite
// bundle (which is flat-bundled in production). Same pattern as useIpc's
// __innoIpc hook. Zero production code path reads __innoStores.
if (typeof window !== 'undefined') {
  window.__innoStores = {
    ui: uiStore,
    connection: connectionStore,
    scene: sceneStore,
    asset: assetStore,
    devToggle: devToggleStore,
    renderTarget: renderTargetStore,
    taskGraph: taskGraphStore,
  }
}
