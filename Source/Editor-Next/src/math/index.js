import { eulerDegToQuat, quatToEulerDeg } from './quatEuler'

export { eulerDegToQuat, quatToEulerDeg }

// Same hook pattern as window.__innoStores: the flat-bundled production
// build hides module paths, so Playwright reaches math helpers through
// this global instead of dynamic import.
if (typeof window !== 'undefined') {
  window.__innoMath = { eulerDegToQuat, quatToEulerDeg }
}
