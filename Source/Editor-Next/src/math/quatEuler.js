/**
 * Quaternion ↔ Euler-degree conversion, XYZ intrinsic (Tait-Bryan) convention.
 *
 *   composed quaternion q = qx * qy * qz
 *   rotations applied to a vector in order Z, then Y, then X in the body frame
 *
 * Quat shape: [x, y, z, w] — matches the engine's Vec4 wire format.
 * Euler shape: [degX, degY, degZ].
 */

const DEG = Math.PI / 180
const RAD = 180 / Math.PI

export function eulerDegToQuat([dx, dy, dz]) {
  const hx = (dx * DEG) * 0.5
  const hy = (dy * DEG) * 0.5
  const hz = (dz * DEG) * 0.5
  const sx = Math.sin(hx), cx = Math.cos(hx)
  const sy = Math.sin(hy), cy = Math.cos(hy)
  const sz = Math.sin(hz), cz = Math.cos(hz)
  const x = sx * cy * cz + cx * sy * sz
  const y = cx * sy * cz - sx * cy * sz
  const z = cx * cy * sz + sx * sy * cz
  const w = cx * cy * cz - sx * sy * sz
  return [x, y, z, w]
}

export function quatToEulerDeg([x, y, z, w]) {
  // β = asin(2(xz + wy)); clamped to survive slight denormalization.
  const sinBeta = 2 * (x * z + w * y)
  const clamped = Math.max(-1, Math.min(1, sinBeta))
  const beta = Math.asin(clamped)

  // Gimbal-lock band: cos(β) ≈ 0. Use degenerate solution — pin γ, fold into α.
  const GIMBAL = 1 - 1e-6
  let alpha, gamma
  if (Math.abs(clamped) < GIMBAL) {
    alpha = Math.atan2(2 * (w * x - y * z), 1 - 2 * (x * x + y * y))
    gamma = Math.atan2(2 * (w * z - x * y), 1 - 2 * (y * y + z * z))
  } else {
    alpha = Math.atan2(2 * (w * x + y * z), 1 - 2 * (x * x + z * z))
    gamma = 0
  }
  return [alpha * RAD, beta * RAD, gamma * RAD]
}
